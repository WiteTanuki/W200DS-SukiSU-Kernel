// SPDX-License-Identifier: GPL-2.0-only
/* Linux 5.4 clean SELinux query view for KernelSU. */
#include <linux/lockdep.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/uidgid.h>
#include <linux/vmalloc.h>

#include "context.h"
#include "ksu_selinux_hide_5_4.h"
#include "objsec.h"
#include "policydb.h"
#include "security.h"
#include "services.h"
#include "sidtab.h"

struct ksu_selinux_clean_view {
	struct selinux_ss ss;
	struct sidtab sidtab;
	struct selinux_state query_state;
	struct sidtab *source_sidtab;
	struct sidtab_convert_params convert;
	u32 source_generation;
	bool policydb_ready;
	bool sidtab_ready;
	bool convert_active;
};

static DEFINE_SPINLOCK(clean_view_lock);
static enum ksu_selinux_clean_view_state clean_view_state =
	KSU_SELINUX_CLEAN_EMPTY;
static struct ksu_selinux_clean_view *building_view;
static struct ksu_selinux_clean_view *ready_view;
static bool clean_view_enabled;

static int copy_context(struct context *old, struct context *new, void *unused)
{
	(void)unused;
	return context_cpy(new, old);
}

static void destroy_view(struct ksu_selinux_clean_view *view)
{
	if (!view)
		return;
	if (view->sidtab_ready)
		sidtab_destroy(&view->sidtab);
	if (view->policydb_ready)
		policydb_destroy(&view->ss.policydb);
	kfree(view);
}

static int serialize_live_policy(struct selinux_state *state, void **data,
				 size_t *len, u32 *generation,
				 struct sidtab **sidtab)
{
	struct policy_file fp;
	struct selinux_ss *ss = state->ss;
	int rc;

	read_lock(&ss->policy_rwlock);
	if (!state->initialized) {
		read_unlock(&ss->policy_rwlock);
		return -EINVAL;
	}
	*len = ss->policydb.len;
	*generation = ss->latest_granting;
	*sidtab = ss->sidtab;
	read_unlock(&ss->policy_rwlock);

	*data = vmalloc(*len);
	if (!*data)
		return -ENOMEM;

	fp.data = *data;
	fp.len = *len;
	read_lock(&ss->policy_rwlock);
	if (*len != ss->policydb.len ||
	    *generation != ss->latest_granting || *sidtab != ss->sidtab) {
		rc = -EAGAIN;
		goto out_unlock;
	}
	rc = policydb_write(&ss->policydb, &fp);
	if (!rc)
		*len = (unsigned long)fp.data - (unsigned long)*data;
out_unlock:
	read_unlock(&ss->policy_rwlock);
	if (rc) {
		vfree(*data);
		*data = NULL;
	}
	return rc;
}

static void fail_build(struct selinux_state *state,
			       struct ksu_selinux_clean_view *view)
{
	if (view && view->convert_active) {
		sidtab_convert_finish(view->source_sidtab, &view->convert);
		view->convert_active = false;
	}
	destroy_view(view);

	spin_lock(&clean_view_lock);
	building_view = NULL;
	clean_view_state = KSU_SELINUX_CLEAN_FAILED;
	spin_unlock(&clean_view_lock);
	mutex_unlock(&state->ss->policy_load_mutex);
}

int ksu_selinux_clean_view_prepare(struct selinux_state *state)
{
	struct ksu_selinux_clean_view *view = NULL;
	struct policy_file fp;
	void *data = NULL;
	size_t len;
	int rc;

	spin_lock(&clean_view_lock);
	if (clean_view_state != KSU_SELINUX_CLEAN_EMPTY) {
		spin_unlock(&clean_view_lock);
		return -EALREADY;
	}
	clean_view_state = KSU_SELINUX_CLEAN_BUILDING;
	spin_unlock(&clean_view_lock);

	/* Held until finish/abort so policy replacement cannot free our source. */
	mutex_lock(&state->ss->policy_load_mutex);
	view = kzalloc(sizeof(*view), GFP_KERNEL);
	if (!view) {
		rc = -ENOMEM;
		goto fail;
	}

	rc = serialize_live_policy(state, &data, &len,
				   &view->source_generation,
				   &view->source_sidtab);
	if (rc)
		goto fail;

	fp.data = data;
	fp.len = len;
	rc = policydb_read(&view->ss.policydb, &fp);
	if (rc)
		goto fail;
	view->policydb_ready = true;
	view->ss.policydb.len = len;

	rc = policydb_load_isids(&view->ss.policydb, &view->sidtab);
	if (rc)
		goto fail;
	view->sidtab_ready = true;
	view->ss.sidtab = &view->sidtab;
	rwlock_init(&view->ss.policy_rwlock);
	mutex_init(&view->ss.status_lock);
	view->ss.latest_granting = view->source_generation;

	view->query_state = *state;
	view->query_state.ss = &view->ss;
	view->query_state.avc = NULL;
	view->query_state.initialized = true;

	view->convert.func = copy_context;
	view->convert.target = &view->sidtab;
	rc = sidtab_convert(view->source_sidtab, &view->convert);
	if (rc)
		goto fail;
	view->convert_active = true;

	vfree(data);
	spin_lock(&clean_view_lock);
	building_view = view;
	spin_unlock(&clean_view_lock);
	return 0;

fail:
	vfree(data);
	fail_build(state, view);
	return rc;
}

int ksu_selinux_clean_view_publish_locked(struct selinux_state *state)
{
	struct ksu_selinux_clean_view *view;
	int rc = 0;

	lockdep_assert_held(&state->ss->policy_rwlock);
	spin_lock(&clean_view_lock);
	view = building_view;
	if (clean_view_state != KSU_SELINUX_CLEAN_BUILDING || !view) {
		rc = -EINVAL;
		goto out;
	}

	if (state->ss->sidtab != view->source_sidtab ||
	    state->ss->latest_granting != view->source_generation) {
		rc = -ESTALE;
		goto stop_convert;
	}

	rc = sidtab_convert_finish(view->source_sidtab, &view->convert);
	if (rc)
		goto stop_convert;
	view->convert_active = false;
	view->source_sidtab = NULL;
	building_view = NULL;
	ready_view = view;
	clean_view_state = KSU_SELINUX_CLEAN_READY;
	goto out;

stop_convert:
	if (view->convert_active) {
		sidtab_convert_finish(view->source_sidtab, &view->convert);
		view->convert_active = false;
	}
	clean_view_state = KSU_SELINUX_CLEAN_FAILED;
out:
	spin_unlock(&clean_view_lock);
	return rc;
}

void ksu_selinux_clean_view_finish(struct selinux_state *state)
{
	struct ksu_selinux_clean_view *view = NULL;

	spin_lock(&clean_view_lock);
	if (clean_view_state == KSU_SELINUX_CLEAN_FAILED) {
		view = building_view;
		building_view = NULL;
	}
	spin_unlock(&clean_view_lock);
	destroy_view(view);
	mutex_unlock(&state->ss->policy_load_mutex);
}

void ksu_selinux_clean_view_abort(struct selinux_state *state)
{
	struct ksu_selinux_clean_view *view;

	spin_lock(&clean_view_lock);
	view = building_view;
	building_view = NULL;
	clean_view_state = KSU_SELINUX_CLEAN_FAILED;
	spin_unlock(&clean_view_lock);

	if (view && view->convert_active) {
		sidtab_convert_finish(view->source_sidtab, &view->convert);
		view->convert_active = false;
	}
	destroy_view(view);
	mutex_unlock(&state->ss->policy_load_mutex);
}

enum ksu_selinux_clean_view_state ksu_selinux_clean_view_state(void)
{
	enum ksu_selinux_clean_view_state state;

	spin_lock(&clean_view_lock);
	state = clean_view_state;
	spin_unlock(&clean_view_lock);
	return state;
}

int ksu_selinux_clean_view_set_enabled(bool enabled)
{
	struct ksu_selinux_clean_view *view;
	struct selinux_kernel_status *status;
	int rc = 0;

	spin_lock(&clean_view_lock);
	view = clean_view_state == KSU_SELINUX_CLEAN_READY ? ready_view : NULL;
	if (!enabled) {
		clean_view_enabled = enabled;
		spin_unlock(&clean_view_lock);
		return 0;
	}
	spin_unlock(&clean_view_lock);
	if (!view)
		return -EAGAIN;

	mutex_lock(&view->ss.status_lock);
	if (!view->ss.status_page)
		view->ss.status_page = alloc_page(GFP_KERNEL | __GFP_ZERO);
	if (!view->ss.status_page) {
		rc = -ENOMEM;
		goto out_status;
	}
	status = page_address(view->ss.status_page);
	if (!status->version) {
		status->version = SELINUX_KERNEL_STATUS_VERSION;
		status->enforcing = enforcing_enabled(&selinux_state);
		status->deny_unknown = !view->ss.policydb.allow_unknown;
	}
out_status:
	mutex_unlock(&view->ss.status_lock);
	if (rc)
		return rc;

	spin_lock(&clean_view_lock);
	if (clean_view_state != KSU_SELINUX_CLEAN_READY || ready_view != view)
		rc = -EAGAIN;
	else
		clean_view_enabled = true;
	spin_unlock(&clean_view_lock);
	return rc;
}

bool ksu_selinux_clean_view_enabled(void)
{
	bool enabled;

	spin_lock(&clean_view_lock);
	enabled = clean_view_enabled;
	spin_unlock(&clean_view_lock);
	return enabled;
}

static bool current_is_app_zygote(struct selinux_state *live)
{
	struct context *context;
	const char *type = NULL;
	bool eligible = false;

	if (uid_eq(current_uid(), GLOBAL_ROOT_UID))
		return false;

	read_lock(&live->ss->policy_rwlock);
	context = sidtab_search(live->ss->sidtab, current_sid());
	if (context && context->type &&
	    context->type <= live->ss->policydb.p_types.nprim)
		type = sym_name(&live->ss->policydb, SYM_TYPES,
				context->type - 1);
	if (type && !strcmp(type, "app_zygote"))
		eligible = true;
	read_unlock(&live->ss->policy_rwlock);
	return eligible;
}

struct selinux_state *
ksu_selinux_clean_view_select(struct selinux_state *live,
			      enum ksu_selinux_query_surface surface)
{
	struct ksu_selinux_clean_view *view;

	if (surface != KSU_SELINUX_QUERY_ACCESS &&
	    surface != KSU_SELINUX_QUERY_CONTEXT)
		return live;

	spin_lock(&clean_view_lock);
	view = clean_view_enabled &&
	       clean_view_state == KSU_SELINUX_CLEAN_READY ? ready_view : NULL;
	spin_unlock(&clean_view_lock);
	if (!view || !current_is_app_zygote(live))
		return live;
	return &view->query_state;
}

int ksu_selinux_clean_view_validate_context(struct selinux_state *live,
					    const char *context,
					    u32 context_len)
{
	struct ksu_selinux_clean_view *view;
	u32 sid;

	spin_lock(&clean_view_lock);
	view = clean_view_enabled &&
	       clean_view_state == KSU_SELINUX_CLEAN_READY ? ready_view : NULL;
	spin_unlock(&clean_view_lock);
	if (!view || !current_is_app_zygote(live))
		return 0;

	return security_context_to_sid(&view->query_state, context, context_len,
				       &sid, GFP_KERNEL);
}

struct page *ksu_selinux_clean_view_status_page(struct selinux_state *live)
{
	struct ksu_selinux_clean_view *view;
	struct page *page = NULL;

	spin_lock(&clean_view_lock);
	view = clean_view_enabled &&
	       clean_view_state == KSU_SELINUX_CLEAN_READY ? ready_view : NULL;
	if (view)
		page = view->ss.status_page;
	spin_unlock(&clean_view_lock);
	if (!page || !current_is_app_zygote(live))
		return NULL;
	return page;
}

void ksu_selinux_clean_view_update_enforcing(int enforcing)
{
	struct ksu_selinux_clean_view *view;
	struct selinux_kernel_status *status;

	spin_lock(&clean_view_lock);
	/* Retired mappings remain valid until reboot and still report enforcement. */
	view = ready_view;
	spin_unlock(&clean_view_lock);
	if (!view)
		return;

	mutex_lock(&view->ss.status_lock);
	if (view->ss.status_page) {
		status = page_address(view->ss.status_page);
		status->sequence++;
		smp_wmb();
		status->enforcing = enforcing;
		smp_wmb();
		status->sequence++;
	}
	mutex_unlock(&view->ss.status_lock);
}

void ksu_selinux_clean_view_retire(void)
{
	spin_lock(&clean_view_lock);
	if (clean_view_state == KSU_SELINUX_CLEAN_READY) {
		clean_view_enabled = false;
		clean_view_state = KSU_SELINUX_CLEAN_RETIRED;
	}
	spin_unlock(&clean_view_lock);
}
