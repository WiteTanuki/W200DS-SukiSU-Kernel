// License-Identifier: GPL-2.0
/*
 * File:shub_algo_cmd_notify.c
 *
 * Copyright (C) 2023 ZTE.
 *
 */

#include <linux/notifier.h>
#include <linux/export.h>

static ATOMIC_NOTIFIER_HEAD(shub_algo_cmd_notifier_list);
static bool shub_algo_cmd_notifier_registerd = false;

int register_shub_algo_cmd_notifier(struct notifier_block *nb)
{
	int ret = 0;

	ret = atomic_notifier_chain_register(&shub_algo_cmd_notifier_list, nb);
	if (ret == 0) {
		shub_algo_cmd_notifier_registerd = true;
	}

	return ret;
}

int unregister_shub_algo_cmd_notifier(struct notifier_block *nb)
{
	int ret = 0;

	ret = atomic_notifier_chain_unregister(&shub_algo_cmd_notifier_list, nb);
	if (ret == 0) {
		shub_algo_cmd_notifier_registerd = false;
	}

	return ret;
}

int call_shub_algo_cmd_notifier_chain(unsigned long val, void *v)
{
	if (shub_algo_cmd_notifier_registerd == true)
		return atomic_notifier_call_chain(&shub_algo_cmd_notifier_list, val, v);
	else
		return 0;
}
EXPORT_SYMBOL(call_shub_algo_cmd_notifier_chain);
