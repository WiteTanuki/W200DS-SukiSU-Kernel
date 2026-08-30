#include <linux/errno.h>

#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
extern int susfs_set_uname_kernel(const char *release, const char *version);
#endif

#include "feature/uts_spoof.h"

int ksu_set_spoof_version(const char *release, const char *version)
{
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
    return susfs_set_uname_kernel(release, version);
#else
    return -EOPNOTSUPP;
#endif
}
