// SPDX-License-Identifier: GPL-2.0-only
#define _GNU_SOURCE

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define KSU_INSTALL_MAGIC1 UINT32_C(0xDEADBEEF)
#define SUSFS_MAGIC UINT32_C(0xFAFAFAFA)
#define CMD_ADD_SUS_PATH UINT32_C(0x55550)
#define CMD_ADD_SUS_PATH_LOOP UINT32_C(0x55553)
#define CMD_ADD_SUS_KSTAT UINT32_C(0x55570)
#define CMD_UPDATE_SUS_KSTAT UINT32_C(0x55571)
#define CMD_ADD_SUS_KSTAT_STATICALLY UINT32_C(0x55572)
#define CMD_ADD_OPEN_REDIRECT UINT32_C(0x555c0)
#define CMD_SHOW_VERSION UINT32_C(0x555e1)
#define CMD_SHOW_ENABLED_FEATURES UINT32_C(0x555e2)
#define CMD_SHOW_VARIANT UINT32_C(0x555e3)
#define CMD_ENABLE_AVC_LOG_SPOOFING UINT32_C(0x60010)
#define CMD_ADD_SUS_MAP UINT32_C(0x60020)
#define ERR_CMD_NOT_SUPPORTED 126
#define SUSFS_MAX_LEN_PATHNAME 256
#define SUSFS_ENABLED_FEATURES_SIZE 8192

#define KSTAT_SPOOF_INO (UINT32_C(1) << 0)
#define KSTAT_SPOOF_DEV (UINT32_C(1) << 1)
#define KSTAT_SPOOF_NLINK (UINT32_C(1) << 2)
#define KSTAT_SPOOF_SIZE (UINT32_C(1) << 3)
#define KSTAT_SPOOF_ATIME_TV_SEC (UINT32_C(1) << 4)
#define KSTAT_SPOOF_ATIME_TV_NSEC (UINT32_C(1) << 5)
#define KSTAT_SPOOF_MTIME_TV_SEC (UINT32_C(1) << 6)
#define KSTAT_SPOOF_MTIME_TV_NSEC (UINT32_C(1) << 7)
#define KSTAT_SPOOF_CTIME_TV_SEC (UINT32_C(1) << 8)
#define KSTAT_SPOOF_CTIME_TV_NSEC (UINT32_C(1) << 9)
#define KSTAT_SPOOF_BLOCKS (UINT32_C(1) << 10)
#define KSTAT_SPOOF_BLKSIZE (UINT32_C(1) << 11)
#define KSTAT_AUTO_SPOOF (KSTAT_SPOOF_INO | KSTAT_SPOOF_DEV | \
	KSTAT_SPOOF_ATIME_TV_SEC | KSTAT_SPOOF_ATIME_TV_NSEC | \
	KSTAT_SPOOF_MTIME_TV_SEC | KSTAT_SPOOF_MTIME_TV_NSEC | \
	KSTAT_SPOOF_CTIME_TV_SEC | KSTAT_SPOOF_CTIME_TV_NSEC | \
	KSTAT_SPOOF_BLOCKS | KSTAT_SPOOF_BLKSIZE)
#define KSTAT_AUTO_SPOOF_FULL (KSTAT_AUTO_SPOOF | KSTAT_SPOOF_NLINK | \
	KSTAT_SPOOF_SIZE)

struct susfs_sus_path {
	char target_pathname[SUSFS_MAX_LEN_PATHNAME];
	int err;
};

struct susfs_sus_kstat {
	uint32_t is_statically;
	unsigned long target_ino;
	char target_pathname[SUSFS_MAX_LEN_PATHNAME];
	unsigned long spoofed_ino;
	unsigned long spoofed_dev;
	unsigned int spoofed_nlink;
	long long spoofed_size;
	long spoofed_atime_tv_sec;
	unsigned long spoofed_atime_tv_nsec;
	long spoofed_mtime_tv_sec;
	unsigned long spoofed_mtime_tv_nsec;
	long spoofed_ctime_tv_sec;
	unsigned long spoofed_ctime_tv_nsec;
	unsigned long long spoofed_blocks;
	long spoofed_blksize;
	uint32_t flags;
	int err;
};

struct susfs_open_redirect {
	char target_pathname[SUSFS_MAX_LEN_PATHNAME];
	char redirected_pathname[SUSFS_MAX_LEN_PATHNAME];
	uint32_t uid_scheme;
	int err;
};

struct susfs_avc_log_spoofing {
	bool enabled;
	int err;
};

struct susfs_enabled_features {
	char enabled_features[SUSFS_ENABLED_FEATURES_SIZE];
	int err;
};

struct susfs_text_result {
	char text[16];
	int err;
};

_Static_assert(sizeof(struct susfs_sus_path) == 260, "SUS_PATH size");
_Static_assert(offsetof(struct susfs_sus_path, err) == 256, "SUS_PATH err");
_Static_assert(sizeof(struct susfs_sus_kstat) == 376, "SUS_KSTAT size");
_Static_assert(offsetof(struct susfs_sus_kstat, flags) == 368,
	       "SUS_KSTAT flags");
_Static_assert(offsetof(struct susfs_sus_kstat, err) == 372,
	       "SUS_KSTAT err");
_Static_assert(sizeof(struct susfs_open_redirect) == 520,
	       "OPEN_REDIRECT size");
_Static_assert(offsetof(struct susfs_open_redirect, uid_scheme) == 512,
	       "OPEN_REDIRECT uid_scheme");
_Static_assert(offsetof(struct susfs_open_redirect, err) == 516,
	       "OPEN_REDIRECT err");
_Static_assert(sizeof(struct susfs_avc_log_spoofing) == 8,
	       "AVC_LOG_SPOOFING size");
_Static_assert(offsetof(struct susfs_avc_log_spoofing, err) == 4,
	       "AVC_LOG_SPOOFING err");
_Static_assert(sizeof(struct susfs_text_result) == 20, "text result size");

static void print_help(FILE *stream)
{
	fputs("usage:\n"
		"  ksu_susfs add_sus_path <path>\n"
		"  ksu_susfs add_sus_path_loop <path>\n"
		"  ksu_susfs add_sus_kstat <path>\n"
		"  ksu_susfs update_sus_kstat <path>\n"
		"  ksu_susfs update_sus_kstat_full_clone <path>\n",
		stream);
	fputs("  ksu_susfs add_sus_map <path-to-mapped-file>\n", stream);
	fputs("  ksu_susfs add_open_redirect <target> <redirected> <scheme:0-4>\n",
	      stream);
	fputs("  ksu_susfs set_avc_log_spoofing <0|1>\n", stream);
	fputs("  ksu_susfs show_version\n  ksu_susfs show_variant\n",
	      stream);
	fputs("  ksu_susfs show_enabled_features\n", stream);
	fputs("  ksu_susfs add_sus_kstat_statically <path> <ino> <dev>\n",
	      stream);
	fputs("    <nlink> <size> <atime> <atime_nsec> <mtime> <mtime_nsec>\n",
	      stream);
	fputs("    <ctime> <ctime_nsec> <blocks> <blksize>\n", stream);
	fputs("  Use 'default' for a field that should not be spoofed.\n",
	      stream);
}

static int copy_path(char destination[SUSFS_MAX_LEN_PATHNAME],
		     const char *source)
{
	size_t length = strlen(source);

	if (!length)
		return -EINVAL;
	if (length >= SUSFS_MAX_LEN_PATHNAME)
		return -ENAMETOOLONG;
	memcpy(destination, source, length + 1);
	return 0;
}

static int susfs_call(uint32_t command, void *request, int *result)
{
	*result = ERR_CMD_NOT_SUPPORTED;
	if (syscall(SYS_reboot, KSU_INSTALL_MAGIC1, SUSFS_MAGIC, command,
		    request) == -1 && errno != EINVAL)
		return -errno;
	if (*result == ERR_CMD_NOT_SUPPORTED)
		fprintf(stderr, "ksu_susfs: command 0x%x is not supported\n",
			command);
	return *result;
}

static int call_sus_path(uint32_t command, const char *path)
{
	struct susfs_sus_path info = { 0 };
	int err = copy_path(info.target_pathname, path);

	return err ? err : susfs_call(command, &info, &info.err);
}

static void stat_to_kstat(struct susfs_sus_kstat *info,
			  const struct stat *sb)
{
	info->target_ino = sb->st_ino;
	info->spoofed_ino = sb->st_ino;
	info->spoofed_dev = sb->st_dev;
	info->spoofed_nlink = sb->st_nlink;
	info->spoofed_size = sb->st_size;
	info->spoofed_atime_tv_sec = sb->st_atim.tv_sec;
	info->spoofed_atime_tv_nsec = sb->st_atim.tv_nsec;
	info->spoofed_mtime_tv_sec = sb->st_mtim.tv_sec;
	info->spoofed_mtime_tv_nsec = sb->st_mtim.tv_nsec;
	info->spoofed_ctime_tv_sec = sb->st_ctim.tv_sec;
	info->spoofed_ctime_tv_nsec = sb->st_ctim.tv_nsec;
	info->spoofed_blocks = sb->st_blocks;
	info->spoofed_blksize = sb->st_blksize;
}

static int prepare_kstat(struct susfs_sus_kstat *info, const char *argument,
			 char resolved[PATH_MAX])
{
	struct stat sb;

	if (!realpath(argument, resolved))
		return -errno;
	if (stat(resolved, &sb))
		return -errno;
	if (copy_path(info->target_pathname, resolved))
		return -ENAMETOOLONG;
	stat_to_kstat(info, &sb);
	return 0;
}

static int call_dynamic_kstat(uint32_t command, const char *path,
			      uint32_t flags)
{
	struct susfs_sus_kstat info = { .flags = flags };
	char resolved[PATH_MAX];
	int err = prepare_kstat(&info, path, resolved);

	return err ? err : susfs_call(command, &info, &info.err);
}

static int parse_unsigned(const char *text, unsigned long long *value)
{
	char *end;

	errno = 0;
	*value = strtoull(text, &end, 0);
	return errno || !text[0] || end[0] ? -EINVAL : 0;
}

static int parse_static_field(const char *text, uint32_t bit, uint32_t *flags,
			      unsigned long long *value, int *is_set)
{
	*is_set = strcmp(text, "default") != 0;
	if (!*is_set)
		return 0;
	if (parse_unsigned(text, value))
		return -EINVAL;
	*flags |= bit;
	return 0;
}

static int call_static_kstat(int argc, char **argv)
{
	struct susfs_sus_kstat info = { .is_statically = 1 };
	char resolved[PATH_MAX];
	unsigned long long value;
	int field_index;
	int is_set;
	int err;

	if (argc != 15)
		return -EINVAL;
	err = prepare_kstat(&info, argv[2], resolved);
	if (err)
		return err;
	for (field_index = 3; field_index < 15; field_index++) {
		uint32_t bit = UINT32_C(1) << (field_index - 3);

		err = parse_static_field(argv[field_index], bit, &info.flags,
					 &value, &is_set);
		if (err)
			return err;
		if (!is_set)
			continue;
		switch (field_index) {
		case 3:
			info.spoofed_ino = value;
			break;
		case 4:
			info.spoofed_dev = value;
			break;
		case 5:
			info.spoofed_nlink = value;
			break;
		case 6:
			info.spoofed_size = value;
			break;
		case 7:
			info.spoofed_atime_tv_sec = value;
			break;
		case 8:
			info.spoofed_atime_tv_nsec = value;
			break;
		case 9:
			info.spoofed_mtime_tv_sec = value;
			break;
		case 10:
			info.spoofed_mtime_tv_nsec = value;
			break;
		case 11:
			info.spoofed_ctime_tv_sec = value;
			break;
		case 12:
			info.spoofed_ctime_tv_nsec = value;
			break;
		case 13:
			info.spoofed_blocks = value;
			break;
		case 14:
			info.spoofed_blksize = value;
			break;
		}
	}
	return susfs_call(CMD_ADD_SUS_KSTAT_STATICALLY, &info, &info.err);
}

static int call_open_redirect(const char *target, const char *redirected,
			      const char *scheme_text)
{
	struct susfs_open_redirect info = { 0 };
	char target_path[PATH_MAX];
	char redirected_path[PATH_MAX];
	unsigned long long scheme;
	int err;

	if (!realpath(target, target_path))
		return -errno;
	if (!realpath(redirected, redirected_path))
		return -errno;
	err = parse_unsigned(scheme_text, &scheme);
	if (err || scheme > 4)
		return -EINVAL;
	err = copy_path(info.target_pathname, target_path);
	if (!err)
		err = copy_path(info.redirected_pathname, redirected_path);
	if (err)
		return err;
	info.uid_scheme = scheme;
	return susfs_call(CMD_ADD_OPEN_REDIRECT, &info, &info.err);
}

static int call_avc_log_spoofing(const char *enabled_text)
{
	struct susfs_avc_log_spoofing info = { 0 };
	unsigned long long enabled;

	if (parse_unsigned(enabled_text, &enabled) || enabled > 1)
		return -EINVAL;
	info.enabled = enabled;
	return susfs_call(CMD_ENABLE_AVC_LOG_SPOOFING, &info, &info.err);
}

static int show_text_result(uint32_t command)
{
	struct susfs_text_result info = { 0 };
	int err = susfs_call(command, &info, &info.err);

	if (!err)
		printf("%s\n", info.text);
	return err;
}

static int show_enabled_features(void)
{
	struct susfs_enabled_features info = { 0 };
	int err = susfs_call(CMD_SHOW_ENABLED_FEATURES, &info, &info.err);

	if (!err)
		fputs(info.enabled_features, stdout);
	return err;
}

int main(int argc, char **argv)
{
	char resolved[PATH_MAX];
	const char *command;
	const char *path;
	int result;

	if (getuid() != 0) {
		fprintf(stderr, "ksu_susfs: must run as root\n");
		return EXIT_FAILURE;
	}
	if (argc < 2) {
		print_help(stderr);
		return EXIT_FAILURE;
	}
	command = argv[1];
	if (!strcmp(command, "add_sus_path") && argc == 3) {
		path = realpath(argv[2], resolved);
		result = path ? call_sus_path(CMD_ADD_SUS_PATH, path) : -errno;
	} else if (!strcmp(command, "add_sus_path_loop") && argc == 3) {
		result = call_sus_path(CMD_ADD_SUS_PATH_LOOP, argv[2]);
	} else if (!strcmp(command, "add_sus_kstat") && argc == 3) {
		result = call_dynamic_kstat(CMD_ADD_SUS_KSTAT, argv[2],
					    KSTAT_AUTO_SPOOF);
	} else if (!strcmp(command, "update_sus_kstat") && argc == 3) {
		result = call_dynamic_kstat(CMD_UPDATE_SUS_KSTAT, argv[2],
					    KSTAT_AUTO_SPOOF);
	} else if (!strcmp(command, "update_sus_kstat_full_clone") && argc == 3) {
		result = call_dynamic_kstat(CMD_UPDATE_SUS_KSTAT, argv[2],
					    KSTAT_AUTO_SPOOF_FULL);
	} else if (!strcmp(command, "add_sus_map") && argc == 3) {
		path = realpath(argv[2], resolved);
		result = path ? call_sus_path(CMD_ADD_SUS_MAP, path) : -errno;
	} else if (!strcmp(command, "add_sus_kstat_statically")) {
		result = call_static_kstat(argc, argv);
	} else if (!strcmp(command, "add_open_redirect") && argc == 5) {
		result = call_open_redirect(argv[2], argv[3], argv[4]);
	} else if (!strcmp(command, "set_avc_log_spoofing") && argc == 3) {
		result = call_avc_log_spoofing(argv[2]);
	} else if (!strcmp(command, "show_version") && argc == 2) {
		result = show_text_result(CMD_SHOW_VERSION);
	} else if (!strcmp(command, "show_variant") && argc == 2) {
		result = show_text_result(CMD_SHOW_VARIANT);
	} else if (!strcmp(command, "show_enabled_features") && argc == 2) {
		result = show_enabled_features();
	} else {
		print_help(stderr);
		return EXIT_FAILURE;
	}
	if (result)
		fprintf(stderr, "ksu_susfs: %s failed: %s (%d)\n", command,
			strerror(result < 0 ? -result : result), result);
	return result < 0 ? -result : result;
}
