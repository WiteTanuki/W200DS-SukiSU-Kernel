#ifndef __KSU_H_ADB_ROOT
#define __KSU_H_ADB_ROOT

#ifdef CONFIG_KSU_FEATURE_ADBROOT
#ifdef CONFIG_KSU_SUSFS
long ksu_adb_root_handle_execve(const char *filename, void ***envp_user_ptr);
#else
struct user_arg_ptr;
long ksu_adb_root_handle_execve_manual(const char *filename,
                                       struct user_arg_ptr *envp);
#endif

void ksu_adb_root_init(void);
void ksu_adb_root_exit(void);
#else
static inline void ksu_adb_root_init(void) { }
static inline void ksu_adb_root_exit(void) { }
#endif

#endif
