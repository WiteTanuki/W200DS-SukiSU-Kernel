#ifndef KSU_SUSFS_DEF_H
#define KSU_SUSFS_DEF_H

#include <linux/bits.h>
#include <linux/cred.h>

/********/
/* ENUM */
/********/
/* shared with userspace ksu_susfs tool */
#define SUSFS_MAGIC 0xFAFAFAFA
#define CMD_SUSFS_ADD_SUS_PATH 0x55550
#define CMD_SUSFS_ADD_SUS_PATH_LOOP 0x55553
#define CMD_SUSFS_ADD_SUS_MOUNT 0x55560
#define CMD_SUSFS_HIDE_SUS_MNTS_FOR_NON_SU_PROCS 0x55561
#define CMD_SUSFS_ADD_SUS_KSTAT 0x55570
#define CMD_SUSFS_UPDATE_SUS_KSTAT 0x55571
#define CMD_SUSFS_ADD_SUS_KSTAT_STATICALLY 0x55572
#define CMD_SUSFS_ADD_TRY_UMOUNT 0x55580
#define CMD_SUSFS_SET_UNAME 0x55590
#define CMD_SUSFS_ENABLE_LOG 0x555a0
#define CMD_SUSFS_SET_CMDLINE_OR_BOOTCONFIG 0x555b0
#define CMD_SUSFS_ADD_OPEN_REDIRECT 0x555c0
#define CMD_SUSFS_RUN_UMOUNT_FOR_CURRENT_MNT_NS 0x555d0
#define CMD_SUSFS_SHOW_VERSION 0x555e1
#define CMD_SUSFS_SHOW_ENABLED_FEATURES 0x555e2
#define CMD_SUSFS_SHOW_VARIANT 0x555e3
#define CMD_SUSFS_SHOW_SUS_SU_WORKING_MODE 0x555e4
#define CMD_SUSFS_IS_SUS_SU_READY 0x555f0
#define CMD_SUSFS_SUS_SU 0x60000
#define CMD_SUSFS_ENABLE_AVC_LOG_SPOOFING 0x60010
#define CMD_SUSFS_ADD_SUS_MAP 0x60020

#define SUSFS_MAX_LEN_PATHNAME 256 // 256 should address many paths already unless you are doing some strange experimental stuff, then set your own desired length
#define SUSFS_FAKE_CMDLINE_OR_BOOTCONFIG_SIZE 4096
#define SUSFS_ENABLED_FEATURES_SIZE 8192
#define SUSFS_MAX_VERSION_BUFSIZE 16
#define SUSFS_MAX_VARIANT_BUFSIZE 16

#define TRY_UMOUNT_DEFAULT 0 /* used by susfs_try_umount() */
#define TRY_UMOUNT_DETACH 1 /* used by susfs_try_umount() */

#define SUS_SU_DISABLED 0
#define SUS_SU_WITH_OVERLAY 1 /* deprecated */
#define SUS_SU_WITH_HOOKS 2

#define DEFAULT_SUS_MNT_ID 100000 /* used by mount->mnt_id */
#define DEFAULT_SUS_MNT_ID_FOR_KSU_PROC_UNSHARE 1000000 /* used by vfsmount->susfs_mnt_id_backup */
#define DEFAULT_SUS_MNT_GROUP_ID 1000 /* used by mount->mnt_group_id */

/*
 * inode->i_mapping->flags => storing flag 'AS_FLAGS_'
 * mount->mnt.susfs_mnt_id_backup => storing original mnt_id of normal mounts or custom sus mnt_id of sus mounts
 * task_struct->susfs_last_fake_mnt_id => storing last valid fake mnt_id
 * task_struct->susfs_task_state => storing flag 'TASK_STRUCT_'
 * task_struct->thread_info.flags => storing flag 'TIF_'
 */

#define TIF_PROC_UMOUNTED 33

#define AS_FLAGS_SUS_PATH 33
#define AS_FLAGS_SUS_MOUNT 34
#define AS_FLAGS_SUS_KSTAT 35
#define AS_FLAGS_OPEN_REDIRECT 36
#define AS_FLAGS_SUS_MAP 39

#define INODE_STATE_SUS_MOUNT BIT(25)
#define TASK_STRUCT_NON_ROOT_USER_APP_PROC BIT(24)

static inline bool susfs_is_current_proc_umounted(void)
{
	return likely(test_thread_flag(TIF_PROC_UMOUNTED));
}

static inline bool susfs_is_current_proc_umounted_app(void)
{
	return likely(test_thread_flag(TIF_PROC_UMOUNTED)) &&
		likely(current_uid().val >= 10000);
}

static inline bool
susfs_is_inode_open_redirect_without_uid_check(struct inode *inode)
{
	return inode && inode->i_mapping &&
		unlikely(test_bit(AS_FLAGS_OPEN_REDIRECT,
				  &inode->i_mapping->flags));
}

#define SUSFS_IS_INODE_OPEN_REDIRECT_WITHOUT_UID_CHECK(inode) \
	susfs_is_inode_open_redirect_without_uid_check(inode)

#define SUSFS_IS_INODE_OPEN_REDIRECT(inode) \
	(SUSFS_IS_INODE_OPEN_REDIRECT_WITHOUT_UID_CHECK(inode) && \
	 susfs_is_current_proc_umounted_app())

static inline bool susfs_is_inode_sus_path(struct inode *inode)
{
	return inode && inode->i_mapping &&
		unlikely(test_bit(AS_FLAGS_SUS_PATH, &inode->i_mapping->flags)) &&
		susfs_is_current_proc_umounted_app() &&
		likely(current_uid().val != inode->i_uid.val);
}

#define SUSFS_IS_INODE_SUS_PATH(inode) susfs_is_inode_sus_path(inode)

static inline bool susfs_is_inode_sus_map(struct inode *inode)
{
	return inode && inode->i_mapping &&
		unlikely(test_bit(AS_FLAGS_SUS_MAP, &inode->i_mapping->flags)) &&
		susfs_is_current_proc_umounted_app();
}

static inline bool susfs_is_vma_sus_map(struct vm_area_struct *vma)
{
	return vma && vma->vm_file &&
		susfs_is_inode_sus_map(file_inode(vma->vm_file));
}

#define MAGIC_MOUNT_WORKDIR "/debug_ramdisk/workdir"
#define DATA_ADB_UMOUNT_FOR_ZYGOTE_SYSTEM_PROCESS "/data/adb/susfs_umount_for_zygote_system_process"
#define DATA_ADB_NO_AUTO_ADD_SUS_BIND_MOUNT "/data/adb/susfs_no_auto_add_sus_bind_mount"
#define DATA_ADB_NO_AUTO_ADD_SUS_KSU_DEFAULT_MOUNT "/data/adb/susfs_no_auto_add_sus_ksu_default_mount"
#define DATA_ADB_NO_AUTO_ADD_TRY_UMOUNT_FOR_BIND_MOUNT "/data/adb/susfs_no_auto_add_try_umount_for_bind_mount"

#endif // #ifndef KSU_SUSFS_DEF_H
