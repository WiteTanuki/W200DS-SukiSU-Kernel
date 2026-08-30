/* SPDX-License-Identifier: GPL-2.0-or-later */
/* 
 * Copyright (C) 2025 Liankong (xhsw.new@outlook.com). All Rights Reserved.
 * 本代码由GPL-2授权
 * 
 * 适配KernelSU的KPM 内核模块加载器兼容实现
 * 
 * 集成了 ELF 解析、内存布局、符号处理、重定位（支持 ARM64 重定位类型）
 * 并参照KernelPatch的标准KPM格式实现加载和控制
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/kernfs.h>
#include <linux/file.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>
#include <linux/elf.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <linux/list.h>
#include <linux/spinlock.h>
#include <linux/rcupdate.h>
#include <asm/elf.h>
#include <linux/mm.h>
#include <linux/string.h>
#include <asm/cacheflush.h>
#include <linux/module.h>
#include <linux/set_memory.h>
#include <linux/export.h>

#include "kpm/kpm.h"
#include "uapi/supercall.h"
#include <linux/slab.h>
#include <asm/insn.h>
#include <linux/kprobes.h>
#include <linux/stacktrace.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0) && defined(CONFIG_MODULES)
#include <linux/moduleloader.h>
#endif

#define KPM_NAME_LEN 32
#define KPM_ARGS_LEN 1024
#define KPM_PATH_LEN 256
#define KPM_INFO_LEN 256
#define KPM_LIST_LEN 1024

#ifndef NO_OPTIMIZE
#if defined(__GNUC__) && !defined(__clang__)
#define NO_OPTIMIZE __attribute__((optimize("O0")))
#elif defined(__clang__)
#define NO_OPTIMIZE __attribute__((optnone))
#else
#define NO_OPTIMIZE
#endif
#endif

noinline NO_OPTIMIZE void sukisu_kpm_load_module_path(const char *path,
						      const char *args,
						      void *ptr, int *result)
{
	if (result)
		*result = -EOPNOTSUPP;

	__asm__ volatile("nop");
}
EXPORT_SYMBOL(sukisu_kpm_load_module_path);

noinline NO_OPTIMIZE void sukisu_kpm_unload_module(const char *name, void *ptr,
						   int *result)
{
	if (result)
		*result = -EOPNOTSUPP;

	__asm__ volatile("nop");
}
EXPORT_SYMBOL(sukisu_kpm_unload_module);

noinline NO_OPTIMIZE void sukisu_kpm_num(int *result)
{
	if (result)
		*result = -EOPNOTSUPP;

	__asm__ volatile("nop");
}
EXPORT_SYMBOL(sukisu_kpm_num);

noinline NO_OPTIMIZE void sukisu_kpm_info(const char *name, char *buf,
					  int bufferSize, int *size)
{
	if (size)
		*size = -EOPNOTSUPP;

	__asm__ volatile("nop");
}
EXPORT_SYMBOL(sukisu_kpm_info);

noinline NO_OPTIMIZE void sukisu_kpm_list(void *out, int bufferSize,
					  int *result)
{
	if (result)
		*result = -EOPNOTSUPP;
}
EXPORT_SYMBOL(sukisu_kpm_list);

noinline NO_OPTIMIZE void sukisu_kpm_control(const char *name, const char *args,
					     long arg_len, int *result)
{
	if (result)
		*result = -EOPNOTSUPP;

	__asm__ volatile("nop");
}
EXPORT_SYMBOL(sukisu_kpm_control);

noinline NO_OPTIMIZE void sukisu_kpm_version(char *buf, int bufferSize)
{
	if (buf && bufferSize > 0)
		buf[0] = '\0';
}
EXPORT_SYMBOL(sukisu_kpm_version);

enum sukisu_kpm_state sukisu_kpm_get_state(void)
{
	char version[32] = { 0 };

	sukisu_kpm_version(version, sizeof(version));
	return version[0] ? SUKISU_KPM_RUNTIME_READY : SUKISU_KPM_BRIDGE_ONLY;
}

static long kpm_copy_user_string(char *dst, size_t dst_size,
				 unsigned long user_ptr, bool allow_empty)
{
	long len;

	if (!user_ptr)
		return allow_empty ? 0 : -EINVAL;

	len = strncpy_from_user(dst, (const char __user *)user_ptr, dst_size);
	if (len < 0)
		return len;
	if (len == 0 && !allow_empty)
		return -EINVAL;
	if (len >= dst_size)
		return -ENAMETOOLONG;

	dst[len] = '\0';
	return len;
}

noinline int sukisu_handle_kpm(unsigned long control_code, unsigned long arg1,
			       unsigned long arg2, unsigned long result_code)
{
	int res = -EINVAL;

	if (control_code == SUKISU_KPM_LOAD) {
		char path[KPM_PATH_LEN] = { 0 };
		char args[KPM_PATH_LEN] = { 0 };

		res = kpm_copy_user_string(path, sizeof(path), arg1, false);
		if (res < 0)
			goto exit;
		res = kpm_copy_user_string(args, sizeof(args), arg2, true);
		if (res < 0)
			goto exit;

		sukisu_kpm_load_module_path(path, args, NULL, &res);
	} else if (control_code == SUKISU_KPM_UNLOAD) {
		char name[KPM_NAME_LEN + 1] = { 0 };

		res = kpm_copy_user_string(name, sizeof(name), arg1, false);
		if (res < 0)
			goto exit;
		sukisu_kpm_unload_module(name, NULL, &res);
	} else if (control_code == SUKISU_KPM_NUM) {
		sukisu_kpm_num(&res);
	} else if (control_code == SUKISU_KPM_INFO) {
		char name[KPM_NAME_LEN + 1] = { 0 };
		char buf[KPM_INFO_LEN] = { 0 };
		size_t copy_len;
		int size = 0;

		if (!arg2 || !access_ok(arg2, sizeof(buf))) {
			res = -EFAULT;
			goto exit;
		}
		res = kpm_copy_user_string(name, sizeof(name), arg1, false);
		if (res < 0)
			goto exit;

		sukisu_kpm_info(name, buf, sizeof(buf), &size);
		if (size < 0) {
			res = size;
			goto exit;
		}
		if (size > (int)sizeof(buf)) {
			res = -EOVERFLOW;
			goto exit;
		}

		buf[sizeof(buf) - 1] = '\0';
		copy_len = strnlen(buf, sizeof(buf) - 1) + 1;
		res = copy_to_user((void __user *)arg2, buf, copy_len) ? -EFAULT : 0;
	} else if (control_code == SUKISU_KPM_LIST) {
		char buf[KPM_LIST_LEN] = { 0 };
		size_t copy_len;
		int len = (int)arg2;

		if (!arg1 || len <= 0 || len > (int)sizeof(buf)) {
			res = -EINVAL;
			goto exit;
		}
		if (!access_ok(arg1, len)) {
			res = -EFAULT;
			goto exit;
		}

		sukisu_kpm_list(buf, len, &res);
		if (res < 0)
			goto exit;
		if (res > len) {
			res = -ENOBUFS;
			goto exit;
		}

		buf[len - 1] = '\0';
		copy_len = strnlen(buf, len - 1) + 1;
		if (copy_to_user((void __user *)arg1, buf, copy_len))
			res = -EFAULT;
	} else if (control_code == SUKISU_KPM_CONTROL) {
		char name[KPM_NAME_LEN + 1] = { 0 };
		char args[KPM_ARGS_LEN] = { 0 };
		long arg_len;

		res = kpm_copy_user_string(name, sizeof(name), arg1, false);
		if (res < 0)
			goto exit;
		arg_len = kpm_copy_user_string(args, sizeof(args), arg2, true);
		if (arg_len < 0) {
			res = arg_len;
			goto exit;
		}

		sukisu_kpm_control(name, args, arg_len, &res);
	} else if (control_code == SUKISU_KPM_VERSION) {
		char buffer[KPM_INFO_LEN] = { 0 };
		size_t len;
		unsigned int outlen = (unsigned int)arg2;

		if (!arg1 || outlen == 0) {
			res = -EINVAL;
			goto exit;
		}

		sukisu_kpm_version(buffer, sizeof(buffer));
		buffer[sizeof(buffer) - 1] = '\0';
		len = strnlen(buffer, sizeof(buffer) - 1);
		if (len >= outlen)
			len = outlen - 1;
		if (!access_ok(arg1, len + 1)) {
			res = -EFAULT;
			goto exit;
		}
		buffer[len] = '\0';
		res = copy_to_user((void __user *)arg1, buffer, len + 1) ? -EFAULT : 0;
	}

exit:
	if (!result_code ||
	    copy_to_user((void __user *)result_code, &res, sizeof(res)))
		return -EFAULT;
	return 0;
}
EXPORT_SYMBOL(sukisu_handle_kpm);

int sukisu_is_kpm_control_code(unsigned long control_code)
{
	return control_code >= CMD_KPM_CONTROL &&
	       control_code <= CMD_KPM_CONTROL_MAX;
}

int do_kpm(void __user *arg)
{
	struct ksu_kpm_cmd cmd;

	if (copy_from_user(&cmd, arg, sizeof(cmd))) {
		pr_err("kpm: copy_from_user failed\n");
		return -EFAULT;
	}

	if (!sukisu_is_kpm_control_code(cmd.control_code))
		return -EINVAL;

	if (!access_ok(cmd.result_code, sizeof(int))) {
		pr_err("kpm: invalid result_code pointer %p\n",
		       (void *)cmd.result_code);
		return -EFAULT;
	}

	return sukisu_handle_kpm(cmd.control_code, cmd.arg1, cmd.arg2,
			     cmd.result_code);
}
