kernelsu-objs := ksu.o

kernelsu-objs += policy/allowlist.o
kernelsu-objs += policy/app_profile.o
kernelsu-objs += policy/feature.o

kernelsu-objs += manager/apk_sign.o
kernelsu-objs += manager/throne_tracker.o
kernelsu-objs += manager/pkg_observer.o

kernelsu-objs += supercall/perm.o
kernelsu-objs += supercall/dispatch.o
kernelsu-objs += supercall/supercall.o

kernelsu-objs += infra/su_mount_ns.o
kernelsu-objs += infra/file_wrapper.o
kernelsu-objs += infra/event_queue.o
kernelsu-objs += infra/kernel_compat.o

kernelsu-objs += feature/kernel_umount.o
kernelsu-objs += feature/sucompat.o
kernelsu-objs += feature/sulog.o
kernelsu-objs += feature/uts_spoof.o
kernelsu-$(CONFIG_KSU_FEATURE_ADBROOT) += feature/adb_root.o
kernelsu-$(CONFIG_KSU_FEATURE_SELINUX_HIDE_5_4) += feature/selinux_hide_5_4.o

kernelsu-objs += runtime/ksud.o
kernelsu-objs += sulog/event.o
kernelsu-objs += sulog/fd.o
kernelsu-objs += hook/lsm_hook.o
kernelsu-objs += selinux/selinux.o
kernelsu-objs += selinux/sepolicy.o
kernelsu-objs += selinux/rules.o

obj-$(CONFIG_KPM) += kpm/kpm.o
obj-$(CONFIG_KPM) += kpm/compact.o
obj-$(CONFIG_KPM) += kpm/super_access.o

obj-$(CONFIG_KSU) += kernelsu.o

ccflags-y += -I$(srctree)/security/selinux -I$(srctree)/security/selinux/include
ccflags-y += -I$(objtree)/security/selinux -include $(srctree)/include/uapi/asm-generic/errno.h

ifeq ($(filter /%,$(src)),)
KSU_KERNEL_DIR := $(srctree)/$(src)
else
KSU_KERNEL_DIR := $(src)
endif

ifeq ($(CONFIG_KSU_SUSFS), y)
## As we are building gki only, no KBUILD_EXTMOD is needed
## Since there is still a build error for 6.12 as $(src) returns absolute path, so here we just hardcode it
ccflags-y += -I$(srctree)/drivers/kernelsu -I$(srctree)/drivers/kernelsu/include
else
ccflags-y += -I$(KSU_KERNEL_DIR) -I$(KSU_KERNEL_DIR)/include
endif

# Frozen from SukiSU-Ultra builtin commit b1d534bc41941b2c818d7a1a1dac341e4aabfc2d.
# Do not derive these values from the surrounding kernel Git repository or network state.
REPO_NAME := SukiSU-Ultra
KSU_VERSION := 40856
KSU_VERSION_FULL := v4.1.3-b1d534bc@builtin
$(info -- $(REPO_NAME) version: $(KSU_VERSION) [$(KSU_VERSION_FULL)])

ccflags-y += -DKSU_VERSION=$(KSU_VERSION)
ccflags-y += -DKSU_VERSION_FULL=\"$(KSU_VERSION_FULL)\"

KERNEL_VERSION := $(VERSION).$(PATCHLEVEL)
KERNEL_TYPE := W200DS Non-GKI builtin
$(info -- KERNEL_VERSION: $(KERNEL_VERSION))
$(info -- KERNEL_TYPE: $(KERNEL_TYPE))

ifeq ($(CONFIG_KSU_SUSFS), y)
$(info -- $(REPO_NAME): using SUSFS_INLINE_HOOK)
else
$(info -- $(REPO_NAME): using Unknown)
endif

ifeq ($(CONFIG_KPM), y)
$(info -- KPM is enabled)
else
$(info -- KPM is disabled)
endif

# Check new vfs_getattr()
ifeq ($(shell grep -A1 "^int vfs_getattr" $(srctree)/fs/stat.c | grep -q "query_flags" ; echo $$?),0)
ccflags-y += -DKSU_HAS_NEW_VFS_GETATTR
endif

ifeq ($(shell grep -q " current_sid(void)" $(srctree)/security/selinux/include/objsec.h; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_CURRENT_SID
endif

ifeq ($(shell grep -q "struct selinux_state " $(srctree)/security/selinux/include/security.h; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_SELINUX_STATE
endif

ifeq ($(shell grep -q "strncpy_from_user_nofault" $(srctree)/include/linux/uaccess.h; echo $$?),0)
ccflags-y += -DKSU_HAS_STRNCPY_FROM_USER_NOFAULT
endif

ifeq ($(shell grep -q "atomic_t\s\+filter_count" $(srctree)/include/linux/seccomp.h; echo $$?),0)
ccflags-y += -DKSU_OPTIONAL_SECCOMP_FILTER_CNT
endif

# some old kernel backport this, check if put_seccomp_filter still exist
ifneq ($(shell grep -wq "put_seccomp_filter" $(srctree)/kernel/seccomp.c $(srctree)/include/linux/seccomp.h; echo $$?),0)
ifeq ($(shell grep -wq "seccomp_filter_release" $(srctree)/kernel/seccomp.c $(srctree)/include/linux/seccomp.h; echo $$?),0)
ccflags-y += -DKSU_OPTIONAL_SECCOMP_FILTER_RELEASE
endif
endif

ifeq ($(shell grep -q "^DEFINE_RWLOCK(policy_rwlock);" $(srctree)/security/selinux/ss/services.c; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_EXPORTED_POLICY_RWLOCK
endif

ifeq ($(shell grep -q "cpus_ptr;" $(srctree)/include/linux/sched.h; echo $$?),0)
ccflags-y += -DKSU_COMPAT_HAS_BACKPORTED_CPUS_PTR
endif

ifeq ($(shell grep -q "selinux_inode" $(srctree)/security/selinux/include/objsec.h; echo $$?),0)
ccflags-y += -DKSU_HAS_SELINUX_INODE
endif

ifeq ($(shell grep -q "selinux_cred" $(srctree)/security/selinux/include/objsec.h; echo $$?),0)
ccflags-y += -DKSU_HAS_SELINUX_CRED
endif

ifeq ($(shell grep -q "struct type_datum \*\*type_val_to_struct;" $(srctree)/security/selinux/ss/policydb.h; echo $$?),0)
ccflags-y += -DKSU_TYPE_VAL_TO_STRUCT
endif

# half-assed-backport from 5.1
ifeq ($(shell grep -q "struct type_datum \*\*type_val_to_struct_array;" $(srctree)/security/selinux/ss/policydb.h; echo $$?),0)
ccflags-y += -DKSU_TYPE_VAL_TO_STRUCT_ARRAY
endif

# https://github.com/torvalds/linux/commit/11276d5306b8e5b438a36bbff855fe792d7eaa61
ifeq ($(shell grep -q "DEFINE_STATIC_KEY_TRUE" $(srctree)/include/linux/jump_label.h 2>/dev/null; echo $$?),0)
$(info -- $(REPO_NAME)/compat: modern static_key_interface found)
ccflags-y += -DKSU_HAS_MODERN_STATIC_KEY_INTERFACE
endif

ifdef KSU_MANAGER_PACKAGE
ccflags-y += -DKSU_MANAGER_PACKAGE=\"$(KSU_MANAGER_PACKAGE)\"
$(info -- KernelSU Manager package name: $(KSU_MANAGER_PACKAGE))
endif

## For susfs stuff ##
ifeq ($(shell test -e $(srctree)/fs/susfs.c; echo $$?),0)
$(eval SUSFS_VERSION=$(shell cat $(srctree)/include/linux/susfs.h | grep -E '^#define SUSFS_VERSION' | cut -d' ' -f3 | sed 's/"//g'))
$(info )
$(info -- SUSFS_VERSION: $(SUSFS_VERSION))
else
$(info -- You have not integrated susfs in your kernel yet.)
$(info -- Read: https://gitlab.com/simonpunk/susfs4ksu)
endif

ccflags-y += -Wno-strict-prototypes -Wno-int-conversion -Wno-gcc-compat -Wno-missing-prototypes
ccflags-y += -Wno-declaration-after-statement -Wno-unused-function

# Keep a new line here!! Because someone may append config
