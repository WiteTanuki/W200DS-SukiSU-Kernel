#include <linux/version.h>
#include <linux/cred.h>
#include <linux/build_bug.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/seq_file.h>
#include <linux/printk.h>
#include <linux/namei.h>
#include <linux/list.h>
#include <linux/init_task.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/stat.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/fdtable.h>
#include <linux/statfs.h>
#include <linux/jump_label.h>
#include <linux/kthread.h>
#include <linux/workqueue.h>
#include <linux/fsnotify_backend.h>
#include <linux/srcu.h>
#include <linux/magic.h>
#include <linux/susfs.h>
#include "mount.h"

#ifndef FUSE_SUPER_MAGIC
#define FUSE_SUPER_MAGIC 0x65735546
#endif

static spinlock_t susfs_spin_lock;

extern bool susfs_is_current_ksu_domain(void);
extern void setup_selinux(const char *domain, struct cred *cred);
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
extern void ksu_try_umount(const char *mnt, bool check_mnt, int flags, uid_t uid);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
extern struct cred *ksu_cred;
#endif

#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
bool susfs_is_log_enabled __read_mostly = false;
#define SUSFS_LOGI(fmt, ...) if (READ_ONCE(susfs_is_log_enabled)) pr_info("susfs:[%u][%d][%s] " fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#define SUSFS_LOGE(fmt, ...) if (READ_ONCE(susfs_is_log_enabled)) pr_err("susfs:[%u][%d][%s]" fmt, current_uid().val, current->pid, __func__, ##__VA_ARGS__)
#else
#define SUSFS_LOGI(fmt, ...)
#define SUSFS_LOGE(fmt, ...)
#endif

/* sus_path */
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
static DEFINE_MUTEX(susfs_sus_path_mutex);
DEFINE_STATIC_SRCU(susfs_sus_path_srcu);
static LIST_HEAD(susfs_sus_path_loop_list);
/* Upstream v2.2 negative-lookup sentinel; a real entry defeats hiding. */
const struct qstr susfs_fake_qstr_name = QSTR_INIT("..5.u.S", 7);

static int susfs_mark_sus_path(const char *target_pathname)
{
	struct path p;
	struct inode *inode;
	int err;

	if (!target_pathname[0])
		return -EINVAL;
	if (strnlen(target_pathname, SUSFS_MAX_LEN_PATHNAME) ==
			SUSFS_MAX_LEN_PATHNAME)
		return -ENAMETOOLONG;

	err = kern_path(target_pathname, LOOKUP_FOLLOW, &p);
	if (err) {
		SUSFS_LOGE("Failed opening file '%s'\n", target_pathname);
		return err;
	}

	inode = d_backing_inode(p.dentry);
	if (!inode || !inode->i_mapping) {
		SUSFS_LOGE("inode or inode mapping is NULL\n");
		err = -ENOENT;
		goto out_path_put;
	}

	set_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags);
	err = 0;

out_path_put:
	path_put(&p);
	return err;
}

void susfs_add_sus_path(void __user **user_info)
{
	struct st_susfs_sus_path info = { 0 };

	if (!user_info || !*user_info)
		return;
	if (copy_from_user(&info, *user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	info.err = susfs_mark_sus_path(info.target_pathname);
out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_path __user *)*user_info)->err,
			 &info.err, sizeof(info.err)))
		SUSFS_LOGE("failed copying result to userspace\n");
}

void susfs_add_sus_path_loop(void __user **user_info)
{
	struct st_susfs_sus_path info = { 0 };
	struct st_susfs_sus_path_list *entry;
	struct st_susfs_sus_path_list *new_entry = NULL;

	if (!user_info || !*user_info)
		return;
	if (copy_from_user(&info, *user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}
	if (!info.target_pathname[0]) {
		info.err = -EINVAL;
		goto out_copy_to_user;
	}
	if (strnlen(info.target_pathname, SUSFS_MAX_LEN_PATHNAME) ==
			SUSFS_MAX_LEN_PATHNAME) {
		info.err = -ENAMETOOLONG;
		goto out_copy_to_user;
	}

	new_entry = kzalloc(sizeof(*new_entry), GFP_KERNEL);
	if (!new_entry) {
		info.err = -ENOMEM;
		goto out_copy_to_user;
	}
	strscpy(new_entry->target_pathname, info.target_pathname,
		SUSFS_MAX_LEN_PATHNAME);

	mutex_lock(&susfs_sus_path_mutex);
	list_for_each_entry(entry, &susfs_sus_path_loop_list, list) {
		if (!strcmp(entry->target_pathname, new_entry->target_pathname)) {
			mutex_unlock(&susfs_sus_path_mutex);
			kfree(new_entry);
			new_entry = NULL;
			info.err = 0;
			goto out_copy_to_user;
		}
	}
	list_add_tail_rcu(&new_entry->list, &susfs_sus_path_loop_list);
	mutex_unlock(&susfs_sus_path_mutex);
	new_entry = NULL;
	info.err = 0;

out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_sus_path __user *)*user_info)->err,
			 &info.err, sizeof(info.err)))
		SUSFS_LOGE("failed copying loop result to userspace\n");
}

static void susfs_sus_path_loop_workfn(struct work_struct *work)
{
	struct st_susfs_sus_path_list *entry;
	const struct cred *saved;
	int srcu_idx;

	if (!READ_ONCE(ksu_cred))
		return;

	saved = override_creds(ksu_cred);
	srcu_idx = srcu_read_lock(&susfs_sus_path_srcu);
	list_for_each_entry_rcu(entry, &susfs_sus_path_loop_list, list)
		susfs_mark_sus_path(entry->target_pathname);
	srcu_read_unlock(&susfs_sus_path_srcu, srcu_idx);
	revert_creds(saved);
}

static DECLARE_WORK(susfs_sus_path_loop_work, susfs_sus_path_loop_workfn);
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_PATH

/* sus_mount */
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
DEFINE_STATIC_KEY_FALSE(susfs_is_hide_sus_mnts_for_non_su_procs_enabled);
DEFINE_STATIC_KEY_TRUE(susfs_is_sdcard_android_data_not_decrypted);
static LIST_HEAD(LH_SUS_MOUNT);

void susfs_set_hide_sus_mnts_for_non_su_procs(void __user **user_info)
{
	struct st_susfs_hide_sus_mnts_for_non_su_procs info = { 0 };

	if (!user_info || !*user_info)
		return;
	if (copy_from_user(&info, *user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	if (info.enabled)
		static_branch_enable(&susfs_is_hide_sus_mnts_for_non_su_procs_enabled);
	else
		static_branch_disable(&susfs_is_hide_sus_mnts_for_non_su_procs_enabled);
	info.err = 0;

out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_hide_sus_mnts_for_non_su_procs __user *)*user_info)->err,
			 &info.err, sizeof(info.err)))
		pr_warn("susfs: failed to report mount hiding state\n");
}

#define SDCARD_ANDROID_PATH "/data/media/0/Android"

struct susfs_watch_dir {
	const char *path;
	u32 mask;
	struct path kpath;
	struct inode *inode;
	struct fsnotify_mark *mark;
};

static struct fsnotify_group *susfs_sdcard_group;
static struct susfs_watch_dir susfs_sdcard_watch = {
	.path = "/data/media/0",
	.mask = FS_EVENT_ON_CHILD | FS_ISDIR | FS_OPEN_PERM,
};
static unsigned long susfs_sdcard_cleanup_scheduled;
static struct delayed_work susfs_sdcard_cleanup_work;

static void susfs_sdcard_cleanup_fn(struct work_struct *work)
{
	struct fsnotify_group *group;
	struct inode *inode;

	if (static_key_enabled(&susfs_is_sdcard_android_data_not_decrypted))
		static_branch_disable(&susfs_is_sdcard_android_data_not_decrypted);
	SUSFS_LOGI("/sdcard is decrypted\n");

	group = xchg(&susfs_sdcard_group, NULL);
	if (group) {
		susfs_sdcard_watch.mark = NULL;
		fsnotify_destroy_group(group);
	}
	inode = xchg(&susfs_sdcard_watch.inode, NULL);
	if (inode)
		iput(inode);
	if (susfs_sdcard_watch.kpath.mnt) {
		path_put(&susfs_sdcard_watch.kpath);
		memset(&susfs_sdcard_watch.kpath, 0,
		       sizeof(susfs_sdcard_watch.kpath));
	}
}

static int susfs_handle_sdcard_event(struct fsnotify_group *group,
				     struct inode *inode, u32 mask,
				     const void *data, int data_type,
				     const struct qstr *file_name, u32 cookie,
				     struct fsnotify_iter_info *iter_info)
{
	if (!file_name || file_name->len != 7 ||
	    memcmp(file_name->name, "Android", 7))
		return 0;
	if (test_and_set_bit(0, &susfs_sdcard_cleanup_scheduled))
		return 0;

	SUSFS_LOGI("'%s' detected, mask: 0x%x\n", SDCARD_ANDROID_PATH, mask);
	queue_delayed_work(system_unbound_wq, &susfs_sdcard_cleanup_work, 5 * HZ);
	return 0;
}

static void susfs_free_sdcard_mark(struct fsnotify_mark *mark)
{
	kfree(mark);
}

static const struct fsnotify_ops susfs_sdcard_ops = {
	.handle_event = susfs_handle_sdcard_event,
	.free_mark = susfs_free_sdcard_mark,
};

static int susfs_watch_sdcard_dir(void)
{
	struct fsnotify_mark *mark;
	int ret;

	ret = kern_path(susfs_sdcard_watch.path, LOOKUP_FOLLOW,
			&susfs_sdcard_watch.kpath);
	if (ret)
		return ret;
	susfs_sdcard_watch.inode = d_backing_inode(susfs_sdcard_watch.kpath.dentry);
	if (!susfs_sdcard_watch.inode) {
		path_put(&susfs_sdcard_watch.kpath);
		return -ENOENT;
	}
	ihold(susfs_sdcard_watch.inode);

	mark = kzalloc(sizeof(*mark), GFP_KERNEL);
	if (!mark) {
		ret = -ENOMEM;
		goto out_inode;
	}
	fsnotify_init_mark(mark, susfs_sdcard_group);
	mark->mask = susfs_sdcard_watch.mask;
	ret = fsnotify_add_inode_mark(mark, susfs_sdcard_watch.inode, 0);
	if (ret) {
		fsnotify_put_mark(mark);
		goto out_inode;
	}
	susfs_sdcard_watch.mark = mark;
	return 0;

out_inode:
	iput(susfs_sdcard_watch.inode);
	susfs_sdcard_watch.inode = NULL;
	path_put(&susfs_sdcard_watch.kpath);
	memset(&susfs_sdcard_watch.kpath, 0, sizeof(susfs_sdcard_watch.kpath));
	return ret;
}

static int susfs_sdcard_monitor_fn(void *data)
{
	struct cred *cred;
	int ret;

	cred = prepare_creds();
	if (!cred)
		return -ENOMEM;
	setup_selinux("u:r:ksu:s0", cred);
	commit_creds(cred);
	if (!susfs_is_current_ksu_domain()) {
		ret = -EINVAL;
		goto out_disable;
	}

	INIT_DELAYED_WORK(&susfs_sdcard_cleanup_work, susfs_sdcard_cleanup_fn);
	susfs_sdcard_group = fsnotify_alloc_group(&susfs_sdcard_ops);
	if (IS_ERR(susfs_sdcard_group)) {
		ret = PTR_ERR(susfs_sdcard_group);
		susfs_sdcard_group = NULL;
		goto out_disable;
	}
	ret = susfs_watch_sdcard_dir();
	if (!ret)
		return 0;
	fsnotify_destroy_group(susfs_sdcard_group);
	susfs_sdcard_group = NULL;

out_disable:
	if (static_key_enabled(&susfs_is_sdcard_android_data_not_decrypted))
		static_branch_disable(&susfs_is_sdcard_android_data_not_decrypted);
	return ret;
}

void susfs_start_sdcard_monitor_fn(void)
{
	if (IS_ERR(kthread_run(susfs_sdcard_monitor_fn, NULL,
			      "susfs_sdcard_monitor")) &&
	    static_key_enabled(&susfs_is_sdcard_android_data_not_decrypted))
		static_branch_disable(&susfs_is_sdcard_android_data_not_decrypted);
}

static void susfs_update_sus_mount_inode(char *target_pathname) {
	struct mount *mnt = NULL;
	struct path p;
	struct inode *inode = NULL;
	int err = 0;

	err = kern_path(target_pathname, LOOKUP_FOLLOW, &p);
	if (err) {
		SUSFS_LOGE("Failed opening file '%s'\n", target_pathname);
		return;
	}

	/* It is important to check if the mount has a legit peer group id, if so we cannot add them to sus_mount,
	 * since there are chances that the mount is a legit mountpoint, and it can be misued by other susfs functions in future.
	 * And by doing this it won't affect the sus_mount check as other susfs functions check by mnt->mnt_id
	 * instead of INODE_STATE_SUS_MOUNT.
	 */
	mnt = real_mount(p.mnt);
	if (mnt->mnt_group_id > 0 && // 0 means no peer group
		mnt->mnt_group_id < DEFAULT_SUS_MNT_GROUP_ID) {
		SUSFS_LOGE("skip setting SUS_MOUNT inode state for path '%s' since its source mount has a legit peer group id\n", target_pathname);
		return;
	}

	inode = d_inode(p.dentry);
	if (!inode) {
		path_put(&p);
		SUSFS_LOGE("inode is NULL\n");
		return;
	}

	if (!(inode->i_state & INODE_STATE_SUS_MOUNT)) {
		spin_lock(&inode->i_lock);
		inode->i_state |= INODE_STATE_SUS_MOUNT;
		spin_unlock(&inode->i_lock);
	}
	path_put(&p);
}

int susfs_add_sus_mount(struct st_susfs_sus_mount* __user user_info) {
	struct st_susfs_sus_mount_list *cursor = NULL, *temp = NULL;
	struct st_susfs_sus_mount_list *new_list = NULL;
	struct st_susfs_sus_mount info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	info.target_dev = new_decode_dev(info.target_dev);
#else
	info.target_dev = huge_decode_dev(info.target_dev);
#endif /* CONFIG_MIPS */
#else
	info.target_dev = old_decode_dev(info.target_dev);
#endif /* defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64) */

	list_for_each_entry_safe(cursor, temp, &LH_SUS_MOUNT, list) {
		if (unlikely(!strcmp(cursor->info.target_pathname, info.target_pathname))) {
			spin_lock(&susfs_spin_lock);
			memcpy(&cursor->info, &info, sizeof(info));
			susfs_update_sus_mount_inode(cursor->info.target_pathname);
			SUSFS_LOGI("target_pathname: '%s', target_dev: '%lu', is successfully updated to LH_SUS_MOUNT\n",
						cursor->info.target_pathname, cursor->info.target_dev);
			spin_unlock(&susfs_spin_lock);
			return 0;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_sus_mount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));
	susfs_update_sus_mount_inode(new_list->info.target_pathname);

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_SUS_MOUNT);
	SUSFS_LOGI("target_pathname: '%s', target_dev: '%lu', is successfully added to LH_SUS_MOUNT\n",
				new_list->info.target_pathname, new_list->info.target_dev);
	spin_unlock(&susfs_spin_lock);
	return 0;
}

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
int susfs_auto_add_sus_bind_mount(const char *pathname, struct path *path_target) {
	struct mount *mnt;
	struct inode *inode;

	mnt = real_mount(path_target->mnt);
	if (mnt->mnt_group_id > 0 && // 0 means no peer group
		mnt->mnt_group_id < DEFAULT_SUS_MNT_GROUP_ID) {
		SUSFS_LOGE("skip setting SUS_MOUNT inode state for path '%s' since its source mount has a legit peer group id\n", pathname);
		// return 0 here as we still want it to be added to try_umount list
		return 0;
	}
	inode = path_target->dentry->d_inode;
	if (!inode) return 1;
	if (!(inode->i_state & INODE_STATE_SUS_MOUNT)) {
		spin_lock(&inode->i_lock);
		inode->i_state |= INODE_STATE_SUS_MOUNT;
		spin_unlock(&inode->i_lock);
		SUSFS_LOGI("set SUS_MOUNT inode state for source bind mount path '%s'\n", pathname);
	}
	return 0;
}
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
void susfs_auto_add_sus_ksu_default_mount(const char __user *to_pathname) {
	char *pathname = NULL;
	struct path path;
	struct inode *inode;

	pathname = kmalloc(SUSFS_MAX_LEN_PATHNAME, GFP_KERNEL);
	if (!pathname) {
		SUSFS_LOGE("no enough memory\n");
		return;
	}
	// Here we need to re-retrieve the struct path as we want the new struct path, not the old one
	if (strncpy_from_user(pathname, to_pathname, SUSFS_MAX_LEN_PATHNAME-1) < 0) {
		SUSFS_LOGE("strncpy_from_user()\n");
		goto out_free_pathname;
		return;
	}
	if ((!strncmp(pathname, "/data/adb/modules", 17) ||
		 !strncmp(pathname, "/debug_ramdisk", 14) ||
		 !strncmp(pathname, "/system", 7) ||
		 !strncmp(pathname, "/system_ext", 11) ||
		 !strncmp(pathname, "/vendor", 7) ||
		 !strncmp(pathname, "/product", 8) ||
		 !strncmp(pathname, "/odm", 4)) &&
		 !kern_path(pathname, LOOKUP_FOLLOW, &path)) {
		goto set_inode_sus_mount;
	}
	goto out_free_pathname;
set_inode_sus_mount:
	inode = path.dentry->d_inode;
	if (!inode) {
		goto out_path_put;
		return;
	}
	if (!(inode->i_state & INODE_STATE_SUS_MOUNT)) {
		spin_lock(&inode->i_lock);
		inode->i_state |= INODE_STATE_SUS_MOUNT;
		spin_unlock(&inode->i_lock);
		SUSFS_LOGI("set SUS_MOUNT inode state for default KSU mount path '%s'\n", pathname);
	}
out_path_put:
	path_put(&path);
out_free_pathname:
	kfree(pathname);
}
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_MOUNT

/* sus_kstat */
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
static DEFINE_HASHTABLE(SUS_KSTAT_HLIST, 10);
static DEFINE_MUTEX(susfs_mutex_lock_sus_kstat);
DEFINE_SRCU(susfs_srcu_sus_kstat);
static int susfs_mark_inode_sus_kstat(const char *pathname,
				      struct st_susfs_sus_kstat_hlist *entry)
{
	struct path path;
	struct inode *inode;
	int err;

	err = kern_path(pathname, LOOKUP_FOLLOW, &path);
	if (err)
		return err;
	inode = d_backing_inode(path.dentry);
	if (!inode || !inode->i_mapping) {
		err = -ENOENT;
		goto out;
	}

	set_bit(AS_FLAGS_SUS_KSTAT, &inode->i_mapping->flags);
	entry->target_ino = inode->i_ino;
	entry->target_dev = inode->i_sb->s_dev;
	entry->info.target_ino = inode->i_ino;
out:
	path_put(&path);
	return err;
}

static void susfs_decode_spoofed_dev(struct st_susfs_sus_kstat *info)
{
#if defined(__ARCH_WANT_STAT64) || defined(__ARCH_WANT_COMPAT_STAT64)
#ifdef CONFIG_MIPS
	info->spoofed_dev = new_decode_dev(info->spoofed_dev);
#else
	info->spoofed_dev = huge_decode_dev(info->spoofed_dev);
#endif
#else
	info->spoofed_dev = old_decode_dev(info->spoofed_dev);
#endif
}

int susfs_add_sus_kstat(struct st_susfs_sus_kstat __user *user_info)
{
	struct st_susfs_sus_kstat_hlist *entry, *old = NULL, *cursor;
	struct hlist_node *node;
	struct st_susfs_sus_kstat info;
	int bucket, err;

	if (copy_from_user(&info, user_info, sizeof(info)))
		return -EFAULT;
	info.target_pathname[SUSFS_MAX_LEN_PATHNAME - 1] = '\0';
	if (!info.target_pathname[0])
		return -EINVAL;

	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;
	susfs_decode_spoofed_dev(&info);
	memcpy(&entry->info, &info, sizeof(info));
	err = susfs_mark_inode_sus_kstat(entry->info.target_pathname, entry);
	if (err) {
		kfree(entry);
		return err;
	}

	mutex_lock(&susfs_mutex_lock_sus_kstat);
	hash_for_each_safe(SUS_KSTAT_HLIST, bucket, node, cursor, node) {
		if (!strcmp(cursor->info.target_pathname,
			    entry->info.target_pathname)) {
			hash_del_rcu(&cursor->node);
			old = cursor;
			break;
		}
	}
	hash_add_rcu(SUS_KSTAT_HLIST, &entry->node, entry->target_ino);
	mutex_unlock(&susfs_mutex_lock_sus_kstat);
	if (old) {
		synchronize_srcu(&susfs_srcu_sus_kstat);
		kfree(old);
	}
	SUSFS_LOGI("added kstat path '%s', dev %u, ino %lu, flags %#x\n",
		   entry->info.target_pathname, entry->target_dev,
		   entry->target_ino, entry->info.flags);
	return 0;
}

int susfs_update_sus_kstat(struct st_susfs_sus_kstat __user *user_info)
{
	struct st_susfs_sus_kstat_hlist *entry, *old = NULL, *cursor;
	struct hlist_node *node;
	struct st_susfs_sus_kstat info;
	int bucket, err = -ENOENT;

	if (copy_from_user(&info, user_info, sizeof(info)))
		return -EFAULT;
	info.target_pathname[SUSFS_MAX_LEN_PATHNAME - 1] = '\0';
	if (!info.target_pathname[0])
		return -EINVAL;
	entry = kzalloc(sizeof(*entry), GFP_KERNEL);
	if (!entry)
		return -ENOMEM;

	mutex_lock(&susfs_mutex_lock_sus_kstat);
	hash_for_each_safe(SUS_KSTAT_HLIST, bucket, node, cursor, node) {
		if (strcmp(cursor->info.target_pathname, info.target_pathname))
			continue;
		memcpy(&entry->info, &cursor->info, sizeof(entry->info));
		entry->info.flags = info.flags;
		err = susfs_mark_inode_sus_kstat(entry->info.target_pathname,
						 entry);
		if (err)
			break;
		hash_del_rcu(&cursor->node);
		hash_add_rcu(SUS_KSTAT_HLIST, &entry->node, entry->target_ino);
		old = cursor;
		break;
	}
	mutex_unlock(&susfs_mutex_lock_sus_kstat);
	if (old) {
		synchronize_srcu(&susfs_srcu_sus_kstat);
		kfree(old);
		SUSFS_LOGI("updated kstat path '%s', dev %u, ino %lu, flags %#x\n",
			   entry->info.target_pathname, entry->target_dev,
			   entry->target_ino, entry->info.flags);
	} else {
		kfree(entry);
	}
	return err;
}

void susfs_sus_kstat_spoof_generic_fillattr(struct inode *inode,
					    struct kstat *stat)
{
	struct st_susfs_sus_kstat_hlist *entry;
	int index;

	if (!inode || !inode->i_mapping ||
	    !test_bit(AS_FLAGS_SUS_KSTAT, &inode->i_mapping->flags) ||
	    !susfs_is_current_proc_umounted_app())
		return;

	index = srcu_read_lock(&susfs_srcu_sus_kstat);
	hash_for_each_possible_rcu(SUS_KSTAT_HLIST, entry, node, inode->i_ino) {
		if (entry->target_ino != inode->i_ino ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		if (entry->info.flags & KSTAT_SPOOF_INO)
			stat->ino = entry->info.spoofed_ino;
		if (entry->info.flags & KSTAT_SPOOF_DEV)
			stat->dev = entry->info.spoofed_dev;
		if (entry->info.flags & KSTAT_SPOOF_NLINK)
			stat->nlink = entry->info.spoofed_nlink;
		if (entry->info.flags & KSTAT_SPOOF_SIZE)
			stat->size = entry->info.spoofed_size;
		if (entry->info.flags & KSTAT_SPOOF_ATIME_TV_SEC)
			stat->atime.tv_sec = entry->info.spoofed_atime_tv_sec;
		if (entry->info.flags & KSTAT_SPOOF_ATIME_TV_NSEC)
			stat->atime.tv_nsec = entry->info.spoofed_atime_tv_nsec;
		if (entry->info.flags & KSTAT_SPOOF_MTIME_TV_SEC)
			stat->mtime.tv_sec = entry->info.spoofed_mtime_tv_sec;
		if (entry->info.flags & KSTAT_SPOOF_MTIME_TV_NSEC)
			stat->mtime.tv_nsec = entry->info.spoofed_mtime_tv_nsec;
		if (entry->info.flags & KSTAT_SPOOF_CTIME_TV_SEC)
			stat->ctime.tv_sec = entry->info.spoofed_ctime_tv_sec;
		if (entry->info.flags & KSTAT_SPOOF_CTIME_TV_NSEC)
			stat->ctime.tv_nsec = entry->info.spoofed_ctime_tv_nsec;
		if (entry->info.flags & KSTAT_SPOOF_BLOCKS)
			stat->blocks = entry->info.spoofed_blocks;
		if (entry->info.flags & KSTAT_SPOOF_BLKSIZE)
			stat->blksize = entry->info.spoofed_blksize;
		break;
	}
	srcu_read_unlock(&susfs_srcu_sus_kstat, index);
}

void susfs_sus_kstat_spoof_show_map_vma(struct inode *inode, dev_t *out_dev,
					unsigned long *out_ino)
{
	struct st_susfs_sus_kstat_hlist *entry;
	int index;

	if (!inode || !inode->i_mapping ||
	    !test_bit(AS_FLAGS_SUS_KSTAT, &inode->i_mapping->flags) ||
	    !susfs_is_current_proc_umounted_app())
		return;
	index = srcu_read_lock(&susfs_srcu_sus_kstat);
	hash_for_each_possible_rcu(SUS_KSTAT_HLIST, entry, node, inode->i_ino) {
		if (entry->target_ino != inode->i_ino ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		if (entry->info.flags & KSTAT_SPOOF_DEV)
			*out_dev = entry->info.spoofed_dev;
		if (entry->info.flags & KSTAT_SPOOF_INO)
			*out_ino = entry->info.spoofed_ino;
		break;
	}
	srcu_read_unlock(&susfs_srcu_sus_kstat, index);
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_KSTAT

/* try_umount */
#ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT
static LIST_HEAD(LH_TRY_UMOUNT_PATH);
int susfs_add_try_umount(struct st_susfs_try_umount* __user user_info) {
	struct st_susfs_try_umount_list *cursor = NULL, *temp = NULL;
	struct st_susfs_try_umount_list *new_list = NULL;
	struct st_susfs_try_umount info;

	if (copy_from_user(&info, user_info, sizeof(info))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
		if (unlikely(!strcmp(info.target_pathname, cursor->info.target_pathname))) {
			SUSFS_LOGE("target_pathname: '%s' is already created in LH_TRY_UMOUNT_PATH\n", info.target_pathname);
			return 1;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_try_umount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		return 1;
	}

	memcpy(&new_list->info, &info, sizeof(info));

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_TRY_UMOUNT_PATH);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', mnt_mode: %d, is successfully added to LH_TRY_UMOUNT_PATH\n", new_list->info.target_pathname, new_list->info.mnt_mode);
	return 0;
}

void susfs_try_umount(uid_t target_uid) {
	struct st_susfs_try_umount_list *cursor = NULL;

	// We should umount in reversed order
	list_for_each_entry_reverse(cursor, &LH_TRY_UMOUNT_PATH, list) {
		if (cursor->info.mnt_mode == TRY_UMOUNT_DEFAULT) {
			ksu_try_umount(cursor->info.target_pathname, false, 0, target_uid);
		} else if (cursor->info.mnt_mode == TRY_UMOUNT_DETACH) {
			ksu_try_umount(cursor->info.target_pathname, false, MNT_DETACH, target_uid);
		} else {
			SUSFS_LOGE("failed umounting '%s' for uid: %d, mnt_mode '%d' not supported\n",
							cursor->info.target_pathname, target_uid, cursor->info.mnt_mode);
		}
	}
}

#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
void susfs_auto_add_try_umount_for_bind_mount(struct path *path) {
	struct st_susfs_try_umount_list *cursor = NULL, *temp = NULL;
	struct st_susfs_try_umount_list *new_list = NULL;
	char *pathname = NULL, *dpath = NULL;
#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
	bool is_magic_mount_path = false;
#endif

#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	if (path->dentry->d_inode->i_mapping &&
	    test_bit(AS_FLAGS_SUS_KSTAT,
		     &path->dentry->d_inode->i_mapping->flags)) {
		SUSFS_LOGI("skip try_umount: mapping has AS_FLAGS_SUS_KSTAT\n");
		return;
	}
#endif

	pathname = kmalloc(PAGE_SIZE, GFP_KERNEL);
	if (!pathname) {
		SUSFS_LOGE("no enough memory\n");
		return;
	}

	dpath = d_path(path, pathname, PAGE_SIZE);
	if (!dpath) {
		SUSFS_LOGE("dpath is NULL\n");
		goto out_free_pathname;
	}

#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
	if (strstr(dpath, MAGIC_MOUNT_WORKDIR)) {
		is_magic_mount_path = true;
	}
#endif

	list_for_each_entry_safe(cursor, temp, &LH_TRY_UMOUNT_PATH, list) {
#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
		if (is_magic_mount_path && strstr(dpath, cursor->info.target_pathname)) {
			goto out_free_pathname;
		}
#endif
		if (unlikely(!strcmp(dpath, cursor->info.target_pathname))) {
			SUSFS_LOGE("target_pathname: '%s', ino: %lu, is already created in LH_TRY_UMOUNT_PATH\n",
							dpath, path->dentry->d_inode->i_ino);
			goto out_free_pathname;
		}
	}

	new_list = kmalloc(sizeof(struct st_susfs_try_umount_list), GFP_KERNEL);
	if (!new_list) {
		SUSFS_LOGE("no enough memory\n");
		goto out_free_pathname;
	}

#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
	if (is_magic_mount_path) {
		strncpy(new_list->info.target_pathname, dpath + strlen(MAGIC_MOUNT_WORKDIR), SUSFS_MAX_LEN_PATHNAME-1);
		goto out_add_to_list;
	}
#endif
	strncpy(new_list->info.target_pathname, dpath, SUSFS_MAX_LEN_PATHNAME-1);

#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
out_add_to_list:
#endif

	new_list->info.mnt_mode = TRY_UMOUNT_DETACH;

	INIT_LIST_HEAD(&new_list->list);
	spin_lock(&susfs_spin_lock);
	list_add_tail(&new_list->list, &LH_TRY_UMOUNT_PATH);
	spin_unlock(&susfs_spin_lock);
	SUSFS_LOGI("target_pathname: '%s', ino: %lu, mnt_mode: %d, is successfully added to LH_TRY_UMOUNT_PATH\n",
					new_list->info.target_pathname, path->dentry->d_inode->i_ino, new_list->info.mnt_mode);
out_free_pathname:
	kfree(pathname);
}
#endif // #ifdef CONFIG_KSU_SUSFS_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT
#endif // #ifdef CONFIG_KSU_SUSFS_TRY_UMOUNT

/* spoof_uname */
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
static DEFINE_MUTEX(susfs_mutex_lock_uname);
static struct st_susfs_uname my_uname;
static void susfs_my_uname_init(void) {
	memset(&my_uname, 0, sizeof(my_uname));
}

int susfs_set_uname_kernel(const char *release, const char *version)
{
	mutex_lock(&susfs_mutex_lock_uname);
	if (release && release[0] != '\0') {
		if (!strcmp(release, "default"))
			strncpy(my_uname.release, utsname()->release, __NEW_UTS_LEN);
		else
			strncpy(my_uname.release, release, __NEW_UTS_LEN);
		my_uname.release[__NEW_UTS_LEN] = '\0';
	}
	if (version && version[0] != '\0') {
		if (!strcmp(version, "default"))
			strncpy(my_uname.version, utsname()->version, __NEW_UTS_LEN);
		else
			strncpy(my_uname.version, version, __NEW_UTS_LEN);
		my_uname.version[__NEW_UTS_LEN] = '\0';
	}
	SUSFS_LOGI("setting spoofed release: '%s', version: '%s'\n",
			my_uname.release, my_uname.version);
	mutex_unlock(&susfs_mutex_lock_uname);
	return 0;
}

int susfs_set_uname(struct st_susfs_uname* __user user_info) {
	struct st_susfs_uname info;

	if (copy_from_user(&info, user_info, sizeof(struct st_susfs_uname))) {
		SUSFS_LOGE("failed copying from userspace.\n");
		return 1;
	}
	info.release[__NEW_UTS_LEN] = '\0';
	info.version[__NEW_UTS_LEN] = '\0';

	return susfs_set_uname_kernel(info.release, info.version);
}

void susfs_spoof_uname(struct new_utsname* tmp) {
	mutex_lock(&susfs_mutex_lock_uname);
	if (likely(my_uname.release[0] != '\0')) {
		strncpy(tmp->release, my_uname.release, __NEW_UTS_LEN);
		strncpy(tmp->version, my_uname.version, __NEW_UTS_LEN);
	}
	mutex_unlock(&susfs_mutex_lock_uname);
}
#endif // #ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME

/* set_log */
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
void susfs_set_log(bool enabled) {
	WRITE_ONCE(susfs_is_log_enabled, enabled);
	if (enabled) {
		pr_info("susfs: enable logging to kernel");
	} else {
		pr_info("susfs: disable logging to kernel");
	}
}

void susfs_enable_log(void __user **user_info)
{
	struct st_susfs_log info = { 0 };

	if (!user_info || !*user_info)
		return;
	if (copy_from_user(&info, *user_info, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy_to_user;
	}

	susfs_set_log(info.enabled);
	info.err = 0;

out_copy_to_user:
	if (copy_to_user(&((struct st_susfs_log __user *)*user_info)->err,
			 &info.err, sizeof(info.err)))
		pr_warn("susfs: failed to report logging state\n");
}
#endif // #ifdef CONFIG_KSU_SUSFS_ENABLE_LOG

/* spoof_cmdline_or_bootconfig */
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
static char *fake_cmdline_or_bootconfig = NULL;
static DEFINE_MUTEX(susfs_mutex_lock_cmdline_or_bootconfig);
int susfs_set_cmdline_or_bootconfig(char* __user user_fake_cmdline_or_bootconfig) {
	char *new_cmdline_or_bootconfig;
	char *old_cmdline_or_bootconfig;
	int res;

	new_cmdline_or_bootconfig = kzalloc(SUSFS_FAKE_CMDLINE_OR_BOOTCONFIG_SIZE, GFP_KERNEL);
	if (!new_cmdline_or_bootconfig) {
		SUSFS_LOGE("no enough memory\n");
		return -ENOMEM;
	}

	res = strncpy_from_user(new_cmdline_or_bootconfig, user_fake_cmdline_or_bootconfig,
					SUSFS_FAKE_CMDLINE_OR_BOOTCONFIG_SIZE - 1);

	if (res > 0) {
		mutex_lock(&susfs_mutex_lock_cmdline_or_bootconfig);
		old_cmdline_or_bootconfig = fake_cmdline_or_bootconfig;
		fake_cmdline_or_bootconfig = new_cmdline_or_bootconfig;
		mutex_unlock(&susfs_mutex_lock_cmdline_or_bootconfig);
		kfree(old_cmdline_or_bootconfig);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,1,0)
		SUSFS_LOGI("fake_cmdline_or_bootconfig is set, length of string: %lu\n", (unsigned long)res);
#else
		SUSFS_LOGI("fake_cmdline_or_bootconfig is set, length of string: %u\n", (unsigned int)res);
#endif
		return 0;
	}
	kfree(new_cmdline_or_bootconfig);
	SUSFS_LOGI("failed setting fake_cmdline_or_bootconfig\n");
	return res;
}

int susfs_spoof_cmdline_or_bootconfig(struct seq_file *m) {
	int ret = 1;

	mutex_lock(&susfs_mutex_lock_cmdline_or_bootconfig);
	if (fake_cmdline_or_bootconfig != NULL) {
		seq_puts(m, fake_cmdline_or_bootconfig);
		ret = 0;
	}
	mutex_unlock(&susfs_mutex_lock_cmdline_or_bootconfig);
	return ret;
}
#endif

/* open_redirect */
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
static DEFINE_HASHTABLE(OPEN_REDIRECT_HLIST, 10);
static DEFINE_MUTEX(susfs_mutex_lock_open_redirect);
DEFINE_SRCU(susfs_srcu_open_redirect);
static bool susfs_open_redirect_pair(const struct st_susfs_open_redirect_hlist *a,
				     const struct st_susfs_open_redirect_hlist *b)
{
	return a->target_ino == b->redirected_ino &&
		a->target_dev == b->redirected_dev &&
		a->redirected_ino == b->target_ino &&
		a->redirected_dev == b->target_dev;
}

int susfs_add_open_redirect(struct st_susfs_open_redirect __user *user_info)
{
	struct st_susfs_open_redirect_hlist *new_target, *new_redirected;
	struct st_susfs_open_redirect_hlist *entry, *old_target = NULL;
	struct st_susfs_open_redirect_hlist *old_redirected = NULL;
	struct hlist_node *tmp;
	struct st_susfs_open_redirect info = { 0 };
	struct path target_path, redirected_path;
	struct inode *target_inode, *redirected_inode;
	int bkt, err;

	if (copy_from_user(&info, user_info, sizeof(info)))
		return -EFAULT;
	info.target_pathname[SUSFS_MAX_LEN_PATHNAME - 1] = '\0';
	info.redirected_pathname[SUSFS_MAX_LEN_PATHNAME - 1] = '\0';
	if (!info.target_pathname[0] || !info.redirected_pathname[0])
		return -EINVAL;
	if (info.uid_scheme > UID_UMOUNTED_PROC)
		return -EINVAL;

	err = kern_path(info.redirected_pathname, 0, &redirected_path);
	if (err)
		return err;
	err = kern_path(info.target_pathname, 0, &target_path);
	if (err)
		goto out_redirected_path;
	target_inode = d_backing_inode(target_path.dentry);
	redirected_inode = d_backing_inode(redirected_path.dentry);
	if (!target_inode || !target_inode->i_mapping || !redirected_inode ||
	    !redirected_inode->i_mapping) {
		err = -ENOENT;
		goto out_target_path;
	}
	if (target_inode == redirected_inode &&
	    target_inode->i_sb->s_dev == redirected_inode->i_sb->s_dev) {
		err = -EINVAL;
		goto out_target_path;
	}
	if (target_inode->i_sb->s_magic == FUSE_SUPER_MAGIC ||
	    redirected_inode->i_sb->s_magic == FUSE_SUPER_MAGIC) {
		err = -EINVAL;
		goto out_target_path;
	}

	new_target = kzalloc(sizeof(*new_target), GFP_KERNEL);
	new_redirected = kzalloc(sizeof(*new_redirected), GFP_KERNEL);
	if (!new_target || !new_redirected) {
		err = -ENOMEM;
		goto out_free_new;
	}
	new_target->target_ino = target_inode->i_ino;
	new_target->target_dev = target_inode->i_sb->s_dev;
	new_target->redirected_ino = redirected_inode->i_ino;
	new_target->redirected_dev = redirected_inode->i_sb->s_dev;
	new_target->spoofed_mnt_id = real_mount(target_path.mnt)->mnt_id;
	new_target->info = info;
	err = vfs_statfs(&target_path, &new_target->spoofed_kstatfs);
	if (err)
		goto out_free_new;

	new_redirected->target_ino = redirected_inode->i_ino;
	new_redirected->target_dev = redirected_inode->i_sb->s_dev;
	new_redirected->redirected_ino = target_inode->i_ino;
	new_redirected->redirected_dev = target_inode->i_sb->s_dev;
	new_redirected->spoofed_mnt_id = new_target->spoofed_mnt_id;
	new_redirected->spoofed_kstatfs = new_target->spoofed_kstatfs;
	new_redirected->info = info;
	strscpy(new_redirected->info.target_pathname,
		info.redirected_pathname, SUSFS_MAX_LEN_PATHNAME);
	strscpy(new_redirected->info.redirected_pathname,
		info.target_pathname, SUSFS_MAX_LEN_PATHNAME);
	new_redirected->reversed_lookup_only = true;

	mutex_lock(&susfs_mutex_lock_open_redirect);
	hash_for_each_possible_safe(OPEN_REDIRECT_HLIST, entry, tmp, node,
				    target_inode->i_ino) {
		if (entry->target_dev != target_inode->i_sb->s_dev)
			continue;
		if (entry->reversed_lookup_only) {
			err = -EEXIST;
			goto out_unlock;
		}
		old_target = entry;
		break;
	}
	if (old_target) {
		hash_for_each_safe(OPEN_REDIRECT_HLIST, bkt, tmp, entry, node) {
			if (entry->reversed_lookup_only &&
			    susfs_open_redirect_pair(old_target, entry)) {
				old_redirected = entry;
				break;
			}
		}
	}
	hash_for_each_possible_safe(OPEN_REDIRECT_HLIST, entry, tmp, node,
				    redirected_inode->i_ino) {
		if (entry->target_dev != redirected_inode->i_sb->s_dev)
			continue;
		if (entry == old_redirected)
			continue;
		err = -EEXIST;
		goto out_unlock;
	}
	if (old_target)
		hash_del_rcu(&old_target->node);
	if (old_redirected)
		hash_del_rcu(&old_redirected->node);
	hash_add_rcu(OPEN_REDIRECT_HLIST, &new_target->node,
		     new_target->target_ino);
	hash_add_rcu(OPEN_REDIRECT_HLIST, &new_redirected->node,
		     new_redirected->target_ino);
	set_bit(AS_FLAGS_OPEN_REDIRECT, &target_inode->i_mapping->flags);
	set_bit(AS_FLAGS_OPEN_REDIRECT, &redirected_inode->i_mapping->flags);
	mutex_unlock(&susfs_mutex_lock_open_redirect);
	if (old_target || old_redirected) {
		synchronize_srcu(&susfs_srcu_open_redirect);
		kfree(old_target);
		kfree(old_redirected);
	}
	err = 0;
	goto out_target_path;

out_unlock:
	mutex_unlock(&susfs_mutex_lock_open_redirect);
out_free_new:
	kfree(new_redirected);
	kfree(new_target);
out_target_path:
	path_put(&target_path);
out_redirected_path:
	path_put(&redirected_path);
	return err;
}

static bool susfs_open_redirect_applies(u32 uid_scheme)
{
	switch (uid_scheme) {
	case UID_NON_APP_PROC:
		return current_uid().val % 100000 < 10000;
	case UID_ROOT_PROC_EXCEPT_SU_PROC:
		return current_uid().val == 0 && !susfs_is_current_ksu_domain();
	case UID_NON_SU_PROC:
		return !susfs_is_current_ksu_domain();
	case UID_UMOUNTED_APP_PROC:
		return susfs_is_current_proc_umounted_app();
	case UID_UMOUNTED_PROC:
		return susfs_is_current_proc_umounted();
	default:
		return false;
	}
}

bool susfs_open_redirect_should_redirect(struct inode *inode)
{
	struct st_susfs_open_redirect_hlist *entry;
	bool redirect = false;
	int idx = srcu_read_lock(&susfs_srcu_open_redirect);

	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		if (!entry->reversed_lookup_only &&
		    entry->target_dev == inode->i_sb->s_dev &&
		    susfs_open_redirect_applies(entry->info.uid_scheme)) {
			redirect = true;
			break;
		}
	}
	srcu_read_unlock(&susfs_srcu_open_redirect, idx);
	return redirect;
}

struct filename *susfs_open_redirect_spoof_do_filp_open(struct inode *inode)
{
	struct st_susfs_open_redirect_hlist *entry;
	struct filename *filename = NULL;
	int idx = srcu_read_lock(&susfs_srcu_open_redirect);

	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		if (entry->reversed_lookup_only ||
		    entry->target_dev != inode->i_sb->s_dev ||
		    !susfs_open_redirect_applies(entry->info.uid_scheme))
			continue;
		filename = getname_kernel(entry->info.redirected_pathname);
		break;
	}
	srcu_read_unlock(&susfs_srcu_open_redirect, idx);
	return filename;
}

int susfs_spoof_vfs_readlink(struct inode *inode, char __user *buffer,
			     int buflen)
{
	struct st_susfs_open_redirect_hlist *entry;
	int ret = -ENOENT;
	int idx = srcu_read_lock(&susfs_srcu_open_redirect);

	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		size_t len;

		if (!entry->reversed_lookup_only ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		len = strlen(entry->info.redirected_pathname);
		if (len > buflen) {
			ret = -ENAMETOOLONG;
			break;
		}
		ret = copy_to_user(buffer, entry->info.redirected_pathname, len) ?
			-EFAULT : len;
		break;
	}
	srcu_read_unlock(&susfs_srcu_open_redirect, idx);
	return ret;
}

int susfs_open_redirect_spoof_do_proc_readlink(struct inode *inode,
					       char *tmp_buf, int buflen)
{
	struct st_susfs_open_redirect_hlist *entry;
	int ret = -ENOENT;
	int idx = srcu_read_lock(&susfs_srcu_open_redirect);

	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		if (!entry->reversed_lookup_only ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		ret = strscpy(tmp_buf, entry->info.redirected_pathname, buflen);
		if (ret >= 0)
			ret = 0;
		break;
	}
	srcu_read_unlock(&susfs_srcu_open_redirect, idx);
	return ret;
}

int susfs_open_redirect_spoof_vfs_statfs(struct inode *inode,
					 struct kstatfs *buf)
{
	struct st_susfs_open_redirect_hlist *entry;
	int ret = -ENOENT;
	int idx = srcu_read_lock(&susfs_srcu_open_redirect);

	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		if (!entry->reversed_lookup_only ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		*buf = entry->spoofed_kstatfs;
		ret = 0;
		break;
	}
	srcu_read_unlock(&susfs_srcu_open_redirect, idx);
	return ret;
}

int susfs_open_redirect_spoof_seq_show(struct inode *inode, int *out_mnt_id,
				       unsigned long *out_ino)
{
	struct st_susfs_open_redirect_hlist *entry;
	int ret = -ENOENT;
	int idx = srcu_read_lock(&susfs_srcu_open_redirect);

	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		if (!entry->reversed_lookup_only ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		*out_mnt_id = entry->spoofed_mnt_id;
		*out_ino = entry->redirected_ino;
		ret = 0;
		break;
	}
	srcu_read_unlock(&susfs_srcu_open_redirect, idx);
	return ret;
}

int susfs_spoof_map_srcu(struct inode *inode,
			 struct susfs_open_redirect_map_spoof *out)
{
	struct st_susfs_open_redirect_hlist *entry;

	if (!out || out->name)
		return -EINVAL;
	hash_for_each_possible_rcu(OPEN_REDIRECT_HLIST, entry, node,
				   inode->i_ino) {
		if (!entry->reversed_lookup_only ||
		    entry->target_dev != inode->i_sb->s_dev)
			continue;
		out->ino = entry->redirected_ino;
		out->dev = entry->redirected_dev;
		out->name = entry->info.redirected_pathname;
		return 0;
	}
	return -ENOENT;
}
#endif // #ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT

/* sus_map */
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
void susfs_add_sus_map(void __user **user_info)
{
	struct st_susfs_sus_map info = { 0 };
	struct st_susfs_sus_map __user *user_info_ptr;
	struct path path;
	struct inode *inode;

	if (!user_info || !*user_info)
		return;
	user_info_ptr = *user_info;
	if (copy_from_user(&info, user_info_ptr, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy;
	}
	info.target_pathname[SUSFS_MAX_LEN_PATHNAME - 1] = '\0';
	if (!info.target_pathname[0]) {
		info.err = -EINVAL;
		goto out_copy;
	}
	info.err = kern_path(info.target_pathname, LOOKUP_FOLLOW, &path);
	if (info.err)
		goto out_copy;
	inode = d_backing_inode(path.dentry);
	if (!inode || !inode->i_mapping) {
		info.err = -ENOENT;
		goto out_path;
	}
	set_bit(AS_FLAGS_SUS_MAP, &inode->i_mapping->flags);
	info.err = 0;
out_path:
	path_put(&path);
out_copy:
	if (copy_to_user(&user_info_ptr->err, &info.err, sizeof(info.err)))
		SUSFS_LOGE("failed copying SUS_MAP result to userspace\n");
}
#endif

/* sus_su */
#ifdef CONFIG_KSU_SUSFS_SUS_SU
bool susfs_is_sus_su_hooks_enabled __read_mostly = false;
static int susfs_sus_su_working_mode = 0;
extern void ksu_susfs_enable_sus_su(void);
extern void ksu_susfs_disable_sus_su(void);

int susfs_get_sus_su_working_mode(void) {
	return susfs_sus_su_working_mode;
}

int susfs_sus_su(struct st_sus_su* __user user_info) {
	struct st_sus_su info;
	int last_working_mode = susfs_sus_su_working_mode;

	if (copy_from_user(&info, user_info, sizeof(struct st_sus_su))) {
		SUSFS_LOGE("failed copying from userspace\n");
		return 1;
	}

	if (info.mode == SUS_SU_WITH_HOOKS) {
		if (last_working_mode == SUS_SU_WITH_HOOKS) {
			SUSFS_LOGE("current sus_su mode is already %d\n", SUS_SU_WITH_HOOKS);
			return 1;
		}
		if (last_working_mode != SUS_SU_DISABLED) {
			SUSFS_LOGE("please make sure the current sus_su mode is %d first\n", SUS_SU_DISABLED);
			return 2;
		}
		ksu_susfs_enable_sus_su();
		susfs_sus_su_working_mode = SUS_SU_WITH_HOOKS;
		susfs_is_sus_su_hooks_enabled = true;
		SUSFS_LOGI("core kprobe hooks for ksu are disabled!\n");
		SUSFS_LOGI("non-kprobe hook sus_su is enabled!\n");
		SUSFS_LOGI("sus_su mode: %d\n", SUS_SU_WITH_HOOKS);
		return 0;
	} else if (info.mode == SUS_SU_DISABLED) {
		if (last_working_mode == SUS_SU_DISABLED) {
			SUSFS_LOGE("current sus_su mode is already %d\n", SUS_SU_DISABLED);
			return 1;
		}
		susfs_is_sus_su_hooks_enabled = false;
		ksu_susfs_disable_sus_su();
		susfs_sus_su_working_mode = SUS_SU_DISABLED;
		if (last_working_mode == SUS_SU_WITH_HOOKS) {
			SUSFS_LOGI("core kprobe hooks for ksu are enabled!\n");
			goto out;
		}
out:
		if (copy_to_user(user_info, &info, sizeof(info)))
			SUSFS_LOGE("copy_to_user() failed\n");
		return 0;
	} else if (info.mode == SUS_SU_WITH_OVERLAY) {
		SUSFS_LOGE("sus_su mode %d is deprecated\n", SUS_SU_WITH_OVERLAY);
		return 1;
	}
	return 1;
}
#endif // #ifdef CONFIG_KSU_SUSFS_SUS_SU

/* SukiSU control-plane compatibility. */
void susfs_set_current_proc_umounted(void)
{
	set_thread_flag(TIF_PROC_UMOUNTED);
	current->susfs_task_state |= TASK_STRUCT_NON_ROOT_USER_APP_PROC;
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	schedule_work(&susfs_sus_path_loop_work);
#endif
}

DEFINE_STATIC_KEY_FALSE(susfs_is_avc_log_spoofing_enabled);

void susfs_set_avc_log_spoofing(void __user **user_info)
{
	struct st_susfs_avc_log_spoofing info = { 0 };
	struct st_susfs_avc_log_spoofing __user *user_info_ptr;

	if (!user_info || !*user_info)
		return;
	user_info_ptr = *user_info;
	if (copy_from_user(&info, user_info_ptr, sizeof(info))) {
		info.err = -EFAULT;
		goto out_copy;
	}
	if (info.enabled)
		static_branch_enable(&susfs_is_avc_log_spoofing_enabled);
	else
		static_branch_disable(&susfs_is_avc_log_spoofing_enabled);
	info.err = 0;
out_copy:
	if (copy_to_user(&user_info_ptr->err, &info.err, sizeof(info.err)))
		SUSFS_LOGE("failed copying AVC log spoofing result to userspace\n");
}

static void susfs_append_enabled_feature(char *features, const char *feature)
{
	strlcat(features, feature, SUSFS_ENABLED_FEATURES_SIZE);
	strlcat(features, "\n", SUSFS_ENABLED_FEATURES_SIZE);
}

void susfs_get_enabled_features(void __user **user_info)
{
	struct st_susfs_enabled_features *info;
	struct st_susfs_enabled_features __user *user_info_ptr;
	int err;

	if (!user_info || !*user_info)
		return;

	user_info_ptr = *user_info;
	info = kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		err = -ENOMEM;
		copy_to_user(&user_info_ptr->err, &err, sizeof(err));
		return;
	}

	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS");
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_SUS_PATH");
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_SUS_MOUNT");
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT");
#endif
#ifdef CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_AUTO_ADD_SUS_BIND_MOUNT");
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_SUS_KSTAT");
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_SPOOF_UNAME");
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_ENABLE_LOG");
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_SPOOF_CMDLINE_OR_BOOTCONFIG");
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_OPEN_REDIRECT");
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_SUS_MAP");
#endif
#ifdef CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT
	susfs_append_enabled_feature(info->enabled_features, "CONFIG_KSU_SUSFS_HAS_MAGIC_MOUNT");
#endif

	info->err = 0;
	if (copy_to_user(user_info_ptr, info, sizeof(*info)))
		pr_warn("susfs: failed to report enabled features\n");
	kfree(info);
}

void susfs_show_variant(void __user **user_info)
{
	struct st_susfs_variant info = { 0 };

	if (!user_info || !*user_info)
		return;
	strlcpy(info.susfs_variant, SUSFS_VARIANT, sizeof(info.susfs_variant));
	if (copy_to_user(*user_info, &info, sizeof(info)))
		pr_warn("susfs: failed to report variant\n");
}

void susfs_show_version(void __user **user_info)
{
	struct st_susfs_version info = { 0 };

	if (!user_info || !*user_info)
		return;
	strlcpy(info.susfs_version, SUSFS_VERSION, sizeof(info.susfs_version));
	if (copy_to_user(*user_info, &info, sizeof(info)))
		pr_warn("susfs: failed to report version\n");
}

/* susfs_init */
void susfs_init(void) {
#ifdef CONFIG_KSU_SUSFS_SUS_PATH
	BUILD_BUG_ON(TIF_PROC_UMOUNTED >= BITS_PER_LONG);
	BUILD_BUG_ON(sizeof(struct st_susfs_sus_path) != 260);
	BUILD_BUG_ON(offsetof(struct st_susfs_sus_path, err) != 256);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MOUNT
	BUILD_BUG_ON(sizeof(struct st_susfs_sus_mount) != 264);
	BUILD_BUG_ON(sizeof(struct st_susfs_hide_sus_mnts_for_non_su_procs) != 8);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_KSTAT
	BUILD_BUG_ON(sizeof(struct st_susfs_sus_kstat) != 376);
#endif
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	BUILD_BUG_ON(sizeof(struct st_susfs_uname) != 136);
#endif
#ifdef CONFIG_KSU_SUSFS_ENABLE_LOG
	BUILD_BUG_ON(sizeof(struct st_susfs_log) != 8);
#endif
#ifdef CONFIG_KSU_SUSFS_OPEN_REDIRECT
	BUILD_BUG_ON(sizeof(struct st_susfs_open_redirect) != 520);
#endif
#ifdef CONFIG_KSU_SUSFS_SUS_MAP
	BUILD_BUG_ON(AS_FLAGS_SUS_MAP >= BITS_PER_LONG);
	BUILD_BUG_ON(sizeof(struct st_susfs_sus_map) != 260);
	BUILD_BUG_ON(offsetof(struct st_susfs_sus_map, err) != 256);
#endif
	BUILD_BUG_ON(sizeof(struct st_susfs_avc_log_spoofing) != 8);
	BUILD_BUG_ON(sizeof(struct st_susfs_enabled_features) != 8196);
	BUILD_BUG_ON(sizeof(struct st_susfs_variant) != 20);
	BUILD_BUG_ON(sizeof(struct st_susfs_version) != 20);
	spin_lock_init(&susfs_spin_lock);
#ifdef CONFIG_KSU_SUSFS_SPOOF_UNAME
	susfs_my_uname_init();
#endif
	SUSFS_LOGI("susfs is initialized! version: " SUSFS_VERSION " \n");
}

/* No module exit is needed becuase it should never be a loadable kernel module */
//void __init susfs_exit(void)
