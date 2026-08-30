#include <asm/current.h>
#include <linux/compiler_types.h>
#include <linux/cred.h>
#include <linux/fcntl.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/mm.h>
#include <linux/namei.h>
#include <linux/ptrace.h>
#include <linux/sched/task_stack.h>
#include <linux/thread_info.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/version.h>

#ifdef CONFIG_KSU_SUSFS
#include <linux/susfs.h>
#endif

#include "feature/adb_root.h"
#include "feature/sucompat.h"
#include "infra/kernel_compat.h"
#include "klog.h"
#include "ksu.h"
#include "policy/allowlist.h"
#include "policy/app_profile.h"
#include "policy/feature.h"
#include "runtime/ksud.h"
#include "selinux/selinux.h"
#include "sulog/event.h"
#include "uapi/feature.h"

#define SU_PATH "/system/bin/su"

bool ksu_su_compat_enabled __read_mostly = true;

static const char su_path[] = SU_PATH;
static const char ksud_path[] = KSUD_PATH;

static __always_inline bool sucompat_policy_allows(const void *target)
{
    if (!READ_ONCE(ksu_su_compat_enabled) || !target)
        return false;

    if (test_thread_flag(TIF_SECCOMP) || current_chrooted())
        return false;

    return ksu_is_allow_uid_for_current(current_uid().val);
}

static int su_compat_feature_get(u64 *value)
{
    *value = ksu_su_compat_enabled ? 1 : 0;
    return 0;
}

static int su_compat_feature_set(u64 value)
{
    bool enable = value != 0;
    ksu_su_compat_enabled = enable;
    pr_info("su_compat: set to %d\n", enable);
    return 0;
}

static const struct ksu_feature_handler su_compat_handler = {
    .feature_id = KSU_FEATURE_SU_COMPAT,
    .name = "su_compat",
    .get_handler = su_compat_feature_get,
    .set_handler = su_compat_feature_set,
};

static void __user *userspace_stack_buffer(const void *d, size_t len)
{
    // To avoid having to mmap a page in userspace, just write below the stack
    // pointer.
    char __user *p = (void __user *)current_user_stack_pointer() - len;

    return copy_to_user(p, d, len) ? NULL : p;
}

static char __user *ksud_user_path(void)
{
    return userspace_stack_buffer(ksud_path, sizeof(ksud_path));
}

static bool sucompat_user_path_is_su(const char __user *filename_user)
{
	char path[sizeof(su_path)] = { 0 };
	long len;

	if (!filename_user)
		return false;

	len = ksu_strncpy_from_user_nofault(path, filename_user, sizeof(path));
	return len == sizeof(su_path) &&
	       !memcmp(path, su_path, sizeof(su_path));
}

static const struct cred *sucompat_override_ksud_path(const char __user **filename_user)
{
    const struct cred *old_cred;
    struct path path;
    char __user *user_path;

    if (!sucompat_policy_allows(filename_user ? *filename_user : NULL) ||
        !sucompat_user_path_is_su(*filename_user) || !READ_ONCE(ksu_cred))
        return NULL;

    old_cred = override_creds(ksu_cred);
    if (kern_path(ksud_path, 0, &path)) {
        revert_creds(old_cred);
        return NULL;
    }
    path_put(&path);

    user_path = ksud_user_path();
    if (!user_path) {
        revert_creds(old_cred);
        return NULL;
    }

    *filename_user = user_path;
    return old_cred;
}

static int sucompat_open_ksud_fd(void)
{
    const struct cred *old_cred;
    struct file *file;
    int fd;

    if (!READ_ONCE(ksu_cred))
        return -ENOENT;

    fd = get_unused_fd_flags(O_CLOEXEC);
    if (fd < 0)
        return fd;

    old_cred = override_creds(ksu_cred);
    file = filp_open(KSUD_PATH, O_PATH, 0);
    revert_creds(old_cred);
    if (IS_ERR(file)) {
        put_unused_fd(fd);
        return PTR_ERR(file);
    }

    fd_install(fd, file);
    return fd;
}


#ifdef CONFIG_KSU_SUSFS
extern const char __user *get_user_arg_ptr(struct user_arg_ptr argv, int nr);
/*
 * return 0 -> No further checks should be required afterwards
 * return non-zero -> Further checks should be continued afterwards
 */
int ksu_handle_execveat_init(struct filename *filename, struct user_arg_ptr *argv_user, struct user_arg_ptr *envp_user) {
    int ret = 0;

    if (current->pid == 1)
        return -EINVAL;

    if (!is_init(get_current_cred()))
        return -EINVAL;

    if (unlikely(!strcmp(filename->name, KSUD_PATH))) {
        const char __user *argv_user_ptr = get_user_arg_ptr(*argv_user, 0);
        struct ksu_sulog_pending_event *pending_sucompat = NULL;

        pr_info("hook_manager: escape to root for init executing ksud: %d\n", current->pid);
        pending_sucompat = ksu_sulog_capture_sucompat(filename->name, argv_user, GFP_KERNEL);
		ret = escape_to_root_for_init();
        if (ret) {
            pr_err("escape_to_root_for_init() failed: %d\n", ret);
            return ret;
        }
        if (!argv_user_ptr || IS_ERR(argv_user_ptr)) {
            pr_err("!argv_user_ptr || IS_ERR(argv_user_ptr)\n");
            return 0;
        }
        ksu_sulog_emit_pending(pending_sucompat, ret, GFP_KERNEL);
        return 0;
    }

    if (likely(!strstr(filename->name, "/app_process") && !strstr(filename->name, "/adbd"))) {
        pr_info("susfs: mark no sucompat checks for pid: '%d', exec: '%s'\n", current->pid, filename->name);
        susfs_set_current_proc_umounted();
        return 0;
    }

#ifdef CONFIG_KSU_FEATURE_ADBROOT
#ifdef CONFIG_COMPAT
    if (unlikely(envp_user->is_compat))
        ret = ksu_adb_root_handle_execve(filename->name, (void ***)&envp_user->ptr.compat);
    else
        ret = ksu_adb_root_handle_execve(filename->name, (void ***)&envp_user->ptr.native);
#else
        ret = ksu_adb_root_handle_execve(filename->name, (void ***)&envp_user->ptr.native);
#endif
#endif

    if (ret)
        pr_err("adb root failed: %d\n", ret);

    return ret;
}

// the call from execve_handler_pre won't provided correct value for __never_use_argument, use them after fix execve_handler_pre, keeping them for consistence for manually patched code
int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr,
                 void *argv_user, void *envp_user,
                 int *__never_use_flags)
{
    struct filename *filename;
    struct ksu_sulog_pending_event *pending_sucompat = NULL;
    int ret, tmp_fd;

    if (unlikely(!filename_ptr))
        return -1;

    filename = *filename_ptr;
    if (IS_ERR(filename))
        return -1;

    if (!ksu_handle_execveat_init(filename, (struct user_arg_ptr*)argv_user, (struct user_arg_ptr*)envp_user))
        return -1;

    if (!sucompat_policy_allows(filename))
        return -1;

    if (likely(memcmp(filename->name, su_path, sizeof(su_path))))
        return -1;

    pr_info("ksu_handle_execveat_sucompat: su found\n");

    pending_sucompat = ksu_sulog_capture_sucompat(filename->name, (struct user_arg_ptr*)argv_user, GFP_KERNEL);

    tmp_fd = sucompat_open_ksud_fd();
    if (tmp_fd < 0) {
        pr_info("ksu_handle_execveat_sucompat: ksud unavailable: %d\n", tmp_fd);
        ksu_sulog_emit_pending(pending_sucompat, tmp_fd, GFP_KERNEL);
        return -1;
    }

    ret = escape_with_root_profile();
    if (ret) {
        pr_err("escape_with_root_profile() failed: %d\n", ret);
        close_fd(tmp_fd);
        ksu_sulog_emit_pending(pending_sucompat, ret, GFP_KERNEL);
        return -1;
    }

    ((char *)filename->name)[0] = '\0';
    *fd = tmp_fd;
    *__never_use_flags = AT_EMPTY_PATH;

    ksu_sulog_emit_pending(pending_sucompat, ret, GFP_KERNEL);

    const char __user *argv_user_ptr = get_user_arg_ptr(*((struct user_arg_ptr*)argv_user), 0);
    if (!argv_user_ptr || IS_ERR(argv_user_ptr)) {
        pr_err("!argv_user_ptr || IS_ERR(argv_user_ptr)\n");
        return tmp_fd;
    }

    return tmp_fd;
}

#ifdef KSU_COMPAT_USE_STATIC_KEY
extern struct static_key_true is_first_zygote;
#endif

int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv,
            void *envp, int *flags)
{
#ifdef KSU_COMPAT_USE_STATIC_KEY
    if (static_branch_unlikely(&is_first_zygote))
#else
    if (unlikely(first_zygote))
#endif
        (void)ksu_handle_execveat_ksud(fd, filename_ptr, argv, envp, flags);

    return ksu_handle_execveat_sucompat(fd, filename_ptr, argv, envp,
                        flags);
}

const struct cred *ksu_handle_faccessat(int *dfd,
                       const char __user **filename_user, int *mode,
                       int *__unused_flags)
{
    return sucompat_override_ksud_path(filename_user);
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)
const struct cred *ksu_handle_stat(int *dfd, struct filename **filename,
                      int *flags)
{
    const struct cred *old_cred;
    struct path path;

    if (unlikely(!filename || IS_ERR(*filename) || (*filename)->name == NULL))
        return NULL;

    if (!sucompat_policy_allows((*filename)->name))
        return NULL;

    if (likely(memcmp((*filename)->name, su_path, sizeof(su_path))))
        return NULL;

    if (!READ_ONCE(ksu_cred))
        return NULL;

    old_cred = override_creds(ksu_cred);
    if (kern_path(ksud_path, 0, &path)) {
        revert_creds(old_cred);
        return NULL;
    }
    path_put(&path);
    memcpy((void *)(*filename)->name, ksud_path, sizeof(ksud_path));
    return old_cred;
}
#else
const struct cred *ksu_handle_stat(int *dfd,
                  const char __user **filename_user, int *flags)
{
    return sucompat_override_ksud_path(filename_user);
}
#endif // #if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 1, 0)

#else
__attribute__((hot)) static __always_inline bool __is_su_allowed(const void **ptr_to_check)
{
    return ptr_to_check && sucompat_policy_allows(*ptr_to_check);
}
#define is_su_allowed(ptr) (__is_su_allowed((const void **)ptr))

const struct cred *ksu_handle_faccessat(int *dfd,
                       const char __user **filename_user, int *mode,
                       int *__unused_flags)
{
    return sucompat_override_ksud_path(filename_user);
}

const struct cred *ksu_handle_stat(int *dfd,
                  const char __user **filename_user, int *flags)
{
    return sucompat_override_ksud_path(filename_user);
}

int ksu_handle_execve_sucompat(int *fd, const char __user **filename_user, void *argv, void *__never_use_envp,
                               int *__never_use_flags)
{
    struct ksu_sulog_pending_event *pending_root_execve = NULL;
    int ret, tmp_fd;

    if (!is_su_allowed(filename_user))
        return -1;

    pending_root_execve =
        ksu_sulog_capture_sucompat(*filename_user, *((struct user_arg_ptr *)argv), GFP_KERNEL);

    if (!sucompat_user_path_is_su(*filename_user))
        return -1;

    tmp_fd = sucompat_open_ksud_fd();
    if (tmp_fd < 0) {
        ksu_sulog_emit_pending(pending_root_execve, tmp_fd, GFP_KERNEL);
        return -1;
    }

    ret = escape_with_root_profile();
    ksu_sulog_emit_pending(pending_root_execve, ret, GFP_KERNEL);
    if (ret) {
        close_fd(tmp_fd);
        return -1;
    }

    *fd = tmp_fd;
    *filename_user = userspace_stack_buffer("", sizeof(""));
    *__never_use_flags = AT_EMPTY_PATH;
    if (!*filename_user) {
        close_fd(tmp_fd);
        return -1;
    }
    return tmp_fd;
}

int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr, void *argv, void *__never_use_envp,
                                 int *__never_use_flags)
{
    struct ksu_sulog_pending_event *pending_root_execve = NULL;
    int ret, tmp_fd;

    if (!is_su_allowed(filename_ptr))
        return -1;

    if (likely(memcmp((void *)(*filename_ptr)->name, su_path, sizeof(su_path))))
        return -1;

    pending_root_execve =
        ksu_sulog_capture_sucompat((*filename_ptr)->name, *((struct user_arg_ptr *)argv), GFP_KERNEL);

    tmp_fd = sucompat_open_ksud_fd();
    if (tmp_fd < 0) {
        ksu_sulog_emit_pending(pending_root_execve, tmp_fd, GFP_KERNEL);
        return -1;
    }

    ret = escape_with_root_profile();
    ksu_sulog_emit_pending(pending_root_execve, ret, GFP_KERNEL);
    if (ret) {
        close_fd(tmp_fd);
        return -1;
    }

    ((char *)(*filename_ptr)->name)[0] = '\0';
    *fd = tmp_fd;
    *__never_use_flags = AT_EMPTY_PATH;
    return tmp_fd;
}

extern bool ksu_execveat_hook __read_mostly;
int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv, void *envp, int *flags)
{
#ifdef CONFIG_KSU_FEATURE_ADBROOT
    int ret = 0;
    if (current_uid().val != 1 && is_init(get_current_cred())) {
        ret = ksu_adb_root_handle_execve_manual((*filename_ptr)->name, (struct user_arg_ptr *)envp);
        if (ret) {
            pr_err("adb root failed: %d\n", (int)ret);
        }
    }
#endif

    if (unlikely(ksu_execveat_hook))
        (void)ksu_handle_execveat_ksud(fd, filename_ptr, argv, envp,
                          flags);

    return ksu_handle_execveat_sucompat(fd, filename_ptr, argv, envp, flags);
}

// dead code
int __maybe_unused ksu_handle_devpts(struct inode *inode)
{
    return 0;
}
#endif

// sucompat: permitted process can execute 'su' to gain root access.
void __init ksu_sucompat_init(void)
{
    if (ksu_register_feature_handler(&su_compat_handler)) {
        pr_err("Failed to register su_compat feature handler\n");
    }
}

void __exit ksu_sucompat_exit(void)
{
    ksu_unregister_feature_handler(KSU_FEATURE_SU_COMPAT);
}
