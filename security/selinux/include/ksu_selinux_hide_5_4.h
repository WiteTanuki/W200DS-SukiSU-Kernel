/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef _SELINUX_KSU_SELINUX_HIDE_5_4_H_
#define _SELINUX_KSU_SELINUX_HIDE_5_4_H_

#include <linux/errno.h>
#include <linux/types.h>

struct selinux_state;
struct page;

enum ksu_selinux_query_surface {
	KSU_SELINUX_QUERY_ACCESS,
	KSU_SELINUX_QUERY_CONTEXT,
	KSU_SELINUX_QUERY_SETPROCATTR_CURRENT,
	KSU_SELINUX_QUERY_STATUS,
};

enum ksu_selinux_clean_view_state {
	KSU_SELINUX_CLEAN_EMPTY,
	KSU_SELINUX_CLEAN_BUILDING,
	KSU_SELINUX_CLEAN_READY,
	KSU_SELINUX_CLEAN_RETIRED,
	KSU_SELINUX_CLEAN_FAILED,
};

#ifdef CONFIG_KSU_FEATURE_SELINUX_HIDE_5_4
int ksu_selinux_clean_view_prepare(struct selinux_state *state);
int ksu_selinux_clean_view_publish_locked(struct selinux_state *state);
void ksu_selinux_clean_view_finish(struct selinux_state *state);
void ksu_selinux_clean_view_abort(struct selinux_state *state);
enum ksu_selinux_clean_view_state ksu_selinux_clean_view_state(void);
int ksu_selinux_clean_view_set_enabled(bool enabled);
bool ksu_selinux_clean_view_enabled(void);
struct selinux_state *
ksu_selinux_clean_view_select(struct selinux_state *live,
			      enum ksu_selinux_query_surface surface);
int ksu_selinux_clean_view_validate_context(struct selinux_state *live,
					    const char *context,
					    u32 context_len);
struct page *ksu_selinux_clean_view_status_page(struct selinux_state *live);
void ksu_selinux_clean_view_update_enforcing(int enforcing);
void ksu_selinux_clean_view_retire(void);
#else
static inline int ksu_selinux_clean_view_prepare(struct selinux_state *state)
{
	return -EOPNOTSUPP;
}

static inline int
ksu_selinux_clean_view_publish_locked(struct selinux_state *state)
{
	return -EOPNOTSUPP;
}

static inline void ksu_selinux_clean_view_finish(struct selinux_state *state) { }
static inline void ksu_selinux_clean_view_abort(struct selinux_state *state) { }
static inline enum ksu_selinux_clean_view_state
ksu_selinux_clean_view_state(void)
{
	return KSU_SELINUX_CLEAN_EMPTY;
}

static inline int ksu_selinux_clean_view_set_enabled(bool enabled)
{
	return enabled ? -EOPNOTSUPP : 0;
}

static inline bool ksu_selinux_clean_view_enabled(void)
{
	return false;
}

static inline struct selinux_state *
ksu_selinux_clean_view_select(struct selinux_state *live,
			      enum ksu_selinux_query_surface surface)
{
	return live;
}

static inline int
ksu_selinux_clean_view_validate_context(struct selinux_state *live,
					const char *context, u32 context_len)
{
	return 0;
}

static inline struct page *
ksu_selinux_clean_view_status_page(struct selinux_state *live)
{
	return NULL;
}

static inline void ksu_selinux_clean_view_update_enforcing(int enforcing) { }
static inline void ksu_selinux_clean_view_retire(void) { }
#endif

#endif /* _SELINUX_KSU_SELINUX_HIDE_5_4_H_ */
