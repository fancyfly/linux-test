#
# Makefile for the VPU drivers.
#

ZOEROOT := $(srctree)/drivers/mxc/vpu

EXTRA_CFLAGS += -I$(ZOEROOT)/zoe/include
EXTRA_CFLAGS += -I$(ZOEROOT)/zoe/include/kernel/dv
EXTRA_CFLAGS += -DZOE_LINUXKER_BUILD -DZOE_DEBUG -DZOE_MODULE_MINIMAL
EXTRA_CFLAGS += -include$(ZOEROOT)/zoe/include/kernel/dv/target_config.h
EXTRA_CFLAGS += -include$(ZOEROOT)/zoe/include/zoe_target.h
EXTRA_CFLAGS += -include$(ZOEROOT)/zoe/include/kernel/dv/zvautoconf.h

obj-$(CONFIG_MXC_VPU)    += cdevice.o
obj-$(CONFIG_MXC_VPU)    += ccomponent.o
obj-$(CONFIG_MXC_VPU)    += ccomponent_decoder.o
obj-$(CONFIG_MXC_VPU)    += ccomponent_encoder.o
obj-$(CONFIG_MXC_VPU)    += cport_vb2.o
obj-$(CONFIG_MXC_VPU)    += zvfops.o
obj-$(CONFIG_MXC_VPU)    += drventry.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_cchannel.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_ctask.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_zoe_module_core_src.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_zoe_module_core_sink.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_czvavlib.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_czvcodec.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_czvcodecintf.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_loader.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_peripherals.o
obj-$(CONFIG_MXC_VPU)    += zoe/core_zvavlib.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_event.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_isr.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_memory.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_mutex.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_semaphore.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_spinlock.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_stubs_thread.o
obj-$(CONFIG_MXC_VPU)    += zoe/sosal_os_common.o
obj-$(CONFIG_MXC_VPU)    += zoe/system_zoe_dbg_stubs.o
obj-$(CONFIG_MXC_VPU)    += zoe/system_zoe_ipc_srv.o
obj-$(CONFIG_MXC_VPU)    += zoe/system_zoe_xdr.o
obj-$(CONFIG_MXC_VPU)    += zoe/system_ipc_test.o
obj-$(CONFIG_MXC_VPU)    += zoe/system_zoe_hal.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_base.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_connection_intf.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_control_intf.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_data_intf.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_mgr.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_mgr_intf.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_names.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_cbuffer_list.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_cdata_fifo.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_mgr_intf_clnt.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_connection_intf_clnt.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_control_intf_clnt.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_data_intf_clnt.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_rpconly_zoe_module_vdec_intf_clnt.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_mgr_intf_srv.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_connection_intf_srv.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_control_intf_srv.o
obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_data_intf_srv.o
#obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_vdec_intf_srv.o
#obj-$(CONFIG_MXC_VPU)    += zoe/module_zoe_module_vdec_adaptor.o
#obj-$(CONFIG_MXC_VPU)    += zoe/fw_vsys_one.o
#obj-$(CONFIG_MXC_VPU)    += zoe/fw_vpu_if_impl.o
#obj-$(CONFIG_MXC_VPU)    += zoe/fw_vdec_logo.o
#obj-$(CONFIG_MXC_VPU)    += zoe/fw_freescale_logo.o
#obj-$(CONFIG_MXC_VPU)    += zoe/fw_freescale_yuv.o
obj-$(CONFIG_MXC_VPU)    += zoe/fw_vpu_if.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_cpciecntl.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_cpcieintf.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_cusbcntl.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_cusbintf.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_chpucntl.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_chpuintf.o
obj-$(CONFIG_MXC_VPU)    += zoe/bus_zvdrv_interface.o
obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_cfifo.o
obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_cobject.o
obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_cobjectmgr.o
obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_cqueue.o
obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_cthread.o
obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_util.o
#obj-$(CONFIG_MXC_VPU)    += zoe/shared_zoe_cpp_helper.o

ifeq ($(CONFIG_MXC_VPU_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

