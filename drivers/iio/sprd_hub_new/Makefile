#
# Makefile for sensor hub new drivers.
#
ifeq ($(CONFIG_VENDOR_ZTE_LOG_EXCEPTION),m)
	ccflags-y += -DCONFIG_VENDOR_ZTE_LOG_EXCEPTION
	ccflags-y += -I$(srctree)/include/vendor/comdef/
endif

obj-$(CONFIG_SPRD_SENSOR_HUB_NEW)    += sensorhub.o
sensorhub-objs := shub_core.o shub_protocol.o shub_comm.o

ifeq ($(CONFIG_VENDOR_ZTE_UNDER_PANEL_LIGHT),y)
subdir-ccflags-y += -DCONFIG_VENDOR_ZTE_UNDER_PANEL_LIGHT
sensorhub-objs += shub_algo_cmd_notify.o
endif

ifeq ($(CONFIG_ZTE_SENSOR_CALI_SUPPORT),y)
subdir-ccflags-y += -DCONFIG_ZTE_SENSOR_CALI_SUPPORT
endif
