/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef __KSU_SELINUX_HIDE_5_4_H
#define __KSU_SELINUX_HIDE_5_4_H

#ifdef CONFIG_KSU_FEATURE_SELINUX_HIDE_5_4
void ksu_selinux_hide_5_4_init(void);
void ksu_selinux_hide_5_4_exit(void);
#else
static inline void ksu_selinux_hide_5_4_init(void) { }
static inline void ksu_selinux_hide_5_4_exit(void) { }
#endif

#endif
