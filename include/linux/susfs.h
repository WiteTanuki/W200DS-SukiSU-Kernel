#ifndef KSU_SUSFS_H
#define KSU_SUSFS_H

#include <linux/version.h>
#include <linux/types.h>
#include <linux/utsname.h>
#include <linux/hashtable.h>
#include <linux/path.h>
#include <linux/susfs_def.h>
#include <linux/statfs.h>
#include <linux/jump_label.h>

#define SUSFS_VERSION "v2.2.0"
/* W200DS is a builtin vendor integration; kernel version does not imply GKI. */
#define SUSFS_VARIANT "NON-GKI"

/*********/
/* MACRO */
/*********/
#define getname_safe(name) (name == NULL ? ERR_PTR(-EINVAL) : getname(name))
#define putname_safe(name) (IS_ERR(name) ? NULL : putname(name))

/**********/
/* STRUCT */
/**********/
enum susfs_uid_scheme {
	UID_NON_APP_PROC = 0,
	UID_ROOT_PROC_EXCEPT_SU_PROC,
	UID_NON_SU_PROC,
	UID_UMOUNTED_APP_PROC,
	UID_UMOUNTED_PROC,
};

/* sus_path */
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
struct st_susfs_sus_path {
	char                             target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int                              err;
};

struct st_susfs_sus_path_list {
	char                             target_pathname[SUSFS_MAX_LEN_PATHNAME];
	struct list_head                 list;
};
#endif

/* sus_mount */
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
#define SUSFS_HAS_SDCARD_MONITOR 1

struct st_susfs_sus_mount {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           target_dev;
};

struct st_susfs_sus_mount_list {
	struct list_head                        list;
	struct st_susfs_sus_mount               info;
};

struct st_susfs_hide_sus_mnts_for_non_su_procs {
	bool                                    enabled;
	int                                     err;
};
#endif

/* sus_kstat */
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
#define KSTAT_SPOOF_INO BIT(0)
#define KSTAT_SPOOF_DEV BIT(1)
#define KSTAT_SPOOF_NLINK BIT(2)
#define KSTAT_SPOOF_SIZE BIT(3)
#define KSTAT_SPOOF_ATIME_TV_SEC BIT(4)
#define KSTAT_SPOOF_ATIME_TV_NSEC BIT(5)
#define KSTAT_SPOOF_MTIME_TV_SEC BIT(6)
#define KSTAT_SPOOF_MTIME_TV_NSEC BIT(7)
#define KSTAT_SPOOF_CTIME_TV_SEC BIT(8)
#define KSTAT_SPOOF_CTIME_TV_NSEC BIT(9)
#define KSTAT_SPOOF_BLOCKS BIT(10)
#define KSTAT_SPOOF_BLKSIZE BIT(11)

struct st_susfs_sus_kstat {
	u32                     is_statically;
	unsigned long           target_ino; // the ino after bind mounted or overlayed
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long           spoofed_ino;
	unsigned long           spoofed_dev;
	unsigned int            spoofed_nlink;
	long long               spoofed_size;
	long                    spoofed_atime_tv_sec;
	unsigned long           spoofed_atime_tv_nsec;
	long                    spoofed_mtime_tv_sec;
	unsigned long           spoofed_mtime_tv_nsec;
	long                    spoofed_ctime_tv_sec;
	unsigned long           spoofed_ctime_tv_nsec;
	unsigned long long      spoofed_blocks;
	long                    spoofed_blksize;
	u32                     flags;
	int                     err;
};

struct st_susfs_sus_kstat_hlist {
	unsigned long                           target_ino;
	dev_t                                   target_dev;
	struct st_susfs_sus_kstat               info;
	struct hlist_node                       node;
};
#endif

/* try_umount */
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
struct st_susfs_try_umount {
	char                    target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int                     mnt_mode;
};

struct st_susfs_try_umount_list {
	struct list_head                        list;
	struct st_susfs_try_umount              info;
};
#endif

/* spoof_uname */
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
struct st_susfs_uname {
	char        release[__NEW_UTS_LEN+1];
	char        version[__NEW_UTS_LEN+1];
	int         err;
};
#endif

/* enable_log */
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
struct st_susfs_log {
	bool                                    enabled;
	int                                     err;
};
#endif

/* open_redirect */
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
struct st_susfs_open_redirect {
	char                             target_pathname[SUSFS_MAX_LEN_PATHNAME];
	char                             redirected_pathname[SUSFS_MAX_LEN_PATHNAME];
	u32                              uid_scheme;
	int                              err;
};

struct st_susfs_open_redirect_hlist {
	unsigned long                    target_ino;
	dev_t                            target_dev;
	unsigned long                    redirected_ino;
	dev_t                            redirected_dev;
	int                              spoofed_mnt_id;
	struct kstatfs                   spoofed_kstatfs;
	struct st_susfs_open_redirect    info;
	bool                             reversed_lookup_only;
	struct hlist_node                node;
};

struct susfs_open_redirect_map_spoof {
	unsigned long ino;
	dev_t dev;
	char *name;
};
#endif

/* sus_map */
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
struct st_susfs_sus_map {
	char target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int err;
};
#endif

/* sus_su */
#ifdef CONFIG_KSU_SUSFS_SUS_SU
struct st_sus_su {
	int         mode;
};
#endif

/* SukiSU supercall metadata ABI. */
struct st_susfs_avc_log_spoofing {
	bool enabled;
	int err;
};

struct st_susfs_enabled_features {
	char enabled_features[SUSFS_ENABLED_FEATURES_SIZE];
	int err;
};

struct st_susfs_variant {
	char susfs_variant[SUSFS_MAX_VARIANT_BUFSIZE];
	int err;
};

struct st_susfs_version {
	char susfs_version[SUSFS_MAX_VERSION_BUFSIZE];
	int err;
};

/***********************/
/* FORWARD DECLARATION */
/***********************/
/* sus_path */
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
void susfs_add_sus_path(void __user **user_info);
void susfs_add_sus_path_loop(void __user **user_info);
#endif
/* sus_mount */
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
int susfs_add_sus_mount(struct st_susfs_sus_mount* __user user_info);
void susfs_set_hide_sus_mnts_for_non_su_procs(void __user **user_info);
void susfs_start_sdcard_monitor_fn(void);
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
int susfs_auto_add_sus_bind_mount(const char *pathname, struct path *path_target);
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
void susfs_auto_add_sus_ksu_default_mount(const char __user *to_pathname);
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT

/* sus_kstat */
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
int susfs_add_sus_kstat(struct st_susfs_sus_kstat* __user user_info);
int susfs_update_sus_kstat(struct st_susfs_sus_kstat* __user user_info);
void susfs_sus_kstat_spoof_generic_fillattr(struct inode *inode,
					    struct kstat *stat);
void susfs_sus_kstat_spoof_show_map_vma(struct inode *inode,
					dev_t *out_dev,
					unsigned long *out_ino);
#endif
/* try_umount */
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info);
void susfs_try_umount(uid_t target_uid);
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
void susfs_auto_add_try_umount_for_bind_mount(struct path *path);
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
#endif // #ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
/* spoof_uname */
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
int susfs_set_uname(struct st_susfs_uname* __user user_info);
int susfs_set_uname_kernel(const char *release, const char *version);
void susfs_spoof_uname(struct new_utsname* tmp);
#endif
/* set_log */
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
void susfs_set_log(bool enabled);
void susfs_enable_log(void __user **user_info);
#endif
/* spoof_cmdline_or_bootconfig */
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
int susfs_set_cmdline_or_bootconfig(char* __user user_fake_boot_config);
int susfs_spoof_cmdline_or_bootconfig(struct seq_file *m);
#endif
/* open_redirect */
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
int susfs_add_open_redirect(struct st_susfs_open_redirect* __user user_info);
struct filename *susfs_open_redirect_spoof_do_filp_open(struct inode *inode);
bool susfs_open_redirect_should_redirect(struct inode *inode);
int susfs_spoof_vfs_readlink(struct inode *inode, char __user *buffer,
			     int buflen);
int susfs_open_redirect_spoof_do_proc_readlink(struct inode *inode,
					       char *tmp_buf, int buflen);
int susfs_open_redirect_spoof_vfs_statfs(struct inode *inode,
					 struct kstatfs *buf);
int susfs_open_redirect_spoof_seq_show(struct inode *inode, int *out_mnt_id,
				       unsigned long *out_ino);
int susfs_spoof_map_srcu(struct inode *inode,
			 struct susfs_open_redirect_map_spoof *out);
extern struct srcu_struct susfs_srcu_open_redirect;
#endif
/* sus_map */
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
void susfs_add_sus_map(void __user **user_info);
#endif
/* sus_su */
#ifdef CONFIG_KSU_SUSFS_SUS_SU
int susfs_get_sus_su_working_mode(void);
int susfs_sus_su(struct st_sus_su* __user user_info);
#endif
void susfs_set_current_proc_umounted(void);
extern u32 susfs_ksu_sid;
extern u32 susfs_priv_app_sid;
extern struct static_key_false susfs_is_avc_log_spoofing_enabled;
void susfs_set_avc_log_spoofing(void __user **user_info);
void susfs_get_enabled_features(void __user **user_info);
void susfs_show_variant(void __user **user_info);
void susfs_show_version(void __user **user_info);
/* susfs_init */
void susfs_init(void);

#endif
