#ifndef __SUKISU_KPM_H
#define __SUKISU_KPM_H

int sukisu_handle_kpm(unsigned long control_code, unsigned long arg3,
                      unsigned long arg4, unsigned long result_code);
int sukisu_is_kpm_control_code(unsigned long control_code);
int do_kpm(void __user *arg);

enum sukisu_kpm_state {
	SUKISU_KPM_DISABLED = 0,
	SUKISU_KPM_BRIDGE_ONLY,
	SUKISU_KPM_RUNTIME_READY,
};

enum sukisu_kpm_state sukisu_kpm_get_state(void);

/* KPM Control Code */
#define CMD_KPM_CONTROL 1
#define CMD_KPM_CONTROL_MAX 7

#endif
