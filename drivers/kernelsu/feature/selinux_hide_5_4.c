// SPDX-License-Identifier: GPL-2.0-only
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/version.h>

#include "feature/selinux_hide_5_4.h"
#include "klog.h"
#include "policy/feature.h"
#include "uapi/feature.h"

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 4, 0) || \
	LINUX_VERSION_CODE >= KERNEL_VERSION(5, 5, 0)
#error CONFIG_KSU_FEATURE_SELINUX_HIDE_5_4 requires Linux 5.4
#endif

static int selinux_hide_5_4_get(u64 *value)
{
	*value = 0;
	return 0;
}

static int selinux_hide_5_4_set(u64 value)
{
	/* SH1 exposes the contract but cannot enable it before the clean view exists. */
	return value ? -EAGAIN : 0;
}

static const struct ksu_feature_handler selinux_hide_5_4_handler = {
	.feature_id = KSU_FEATURE_SELINUX_HIDE,
	.name = "selinux_hide_5_4",
	.get_handler = selinux_hide_5_4_get,
	.set_handler = selinux_hide_5_4_set,
};

void ksu_selinux_hide_5_4_init(void)
{
	if (ksu_register_feature_handler(&selinux_hide_5_4_handler))
		pr_err("selinux_hide_5_4: feature registration failed\n");
}

void ksu_selinux_hide_5_4_exit(void)
{
	ksu_unregister_feature_handler(KSU_FEATURE_SELINUX_HIDE);
}
