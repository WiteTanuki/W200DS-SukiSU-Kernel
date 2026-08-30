// SPDX-License-Identifier: GPL-2.0-only
#include <linux/export.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/string.h>

#include "feature/kernel_umount.h"
#include "kpm/compact.h"
#include "manager/manager_identity.h"
#include "manager/throne_tracker.h"
#include "policy/allowlist.h"

static int sukisu_is_su_allow_uid(uid_t uid)
{
    return ksu_is_allow_uid_for_current(uid) ? 1 : 0;
}

static int sukisu_get_ap_mod_exclude(uid_t uid)
{
	(void)uid;
	return 0; /* Not supported */
}

static int sukisu_is_uid_should_umount(uid_t uid)
{
    return ksu_uid_should_umount(uid) ? 1 : 0;
}

static int sukisu_is_current_uid_manager(void)
{
    return is_manager();
}

static uid_t sukisu_get_manager_uid(void)
{
    return ksu_manager_appid;
}

static void sukisu_set_manager_uid(uid_t uid, int force)
{
    if (force || ksu_manager_appid == -1)
        ksu_manager_appid = uid;
}

struct compact_address_symbol {
    const char *symbol_name;
    void *addr;
};

unsigned long sukisu_compact_find_symbol(const char *name);

static const struct compact_address_symbol address_symbol[] = {
    { "kallsyms_lookup_name", &kallsyms_lookup_name },
    { "compact_find_symbol", &sukisu_compact_find_symbol },
    { "is_run_in_sukisu_ultra", (void *)1 },
    { "is_su_allow_uid", &sukisu_is_su_allow_uid },
    { "get_ap_mod_exclude", &sukisu_get_ap_mod_exclude },
    { "is_uid_should_umount", &sukisu_is_uid_should_umount },
    { "is_current_uid_manager", &sukisu_is_current_uid_manager },
    { "get_manager_uid", &sukisu_get_manager_uid },
    { "sukisu_set_manager_uid", &sukisu_set_manager_uid }
};

unsigned long sukisu_compact_find_symbol(const char *name)
{
    int i;
    unsigned long addr;

	for (i = 0; i < ARRAY_SIZE(address_symbol); i++) {
		const struct compact_address_symbol *symbol = &address_symbol[i];

        if (strcmp(name, symbol->symbol_name) == 0)
            return (unsigned long)symbol->addr;
    }

    addr = kallsyms_lookup_name(name);
    if (addr)
        return addr;

    return 0;
}
EXPORT_SYMBOL(sukisu_compact_find_symbol);
