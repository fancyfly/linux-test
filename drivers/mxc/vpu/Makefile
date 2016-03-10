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

obj-$(CONFIG_MXC_VPU)    += vpu.o

vpu-objs:= cache.o \
 cdevice.o ccomponent.o ccomponent_decoder.o ccomponent_encoder.o \
 cport_vb2.o zvfops.o drventry.o zoe/core_cchannel.o zoe/core_ctask.o \
 zoe/core_zoe_module_core_src.o zoe/core_zoe_module_core_sink.o zoe/core_czvavlib.o \
 zoe/core_czvcodec.o zoe/core_czvcodecintf.o zoe/core_loader.o zoe/core_peripherals.o \
 zoe/core_zvavlib.o zoe/sosal_stubs.o zoe/sosal_stubs_event.o zoe/sosal_stubs_isr.o \
 zoe/sosal_stubs_memory.o zoe/sosal_stubs_mutex.o zoe/sosal_stubs_semaphore.o zoe/sosal_stubs_spinlock.o \
 zoe/sosal_stubs_thread.o zoe/sosal_os_common.o zoe/system_zoe_dbg_stubs.o zoe/system_zoe_ipc_srv.o \
 zoe/system_zoe_xdr.o zoe/system_ipc_test.o zoe/system_zoe_hal.o zoe/module_zoe_module_base.o \
 zoe/module_zoe_module_connection_intf.o zoe/module_zoe_module_control_intf.o zoe/module_zoe_module_data_intf.o \
 zoe/module_zoe_module_mgr.o zoe/module_zoe_module_mgr_intf.o zoe/module_zoe_module_names.o zoe/module_zoe_cbuffer_list.o \
 zoe/module_zoe_cdata_fifo.o zoe/module_zoe_module_mgr_intf_clnt.o zoe/module_zoe_module_connection_intf_clnt.o \
 zoe/module_zoe_module_control_intf_clnt.o zoe/module_zoe_module_data_intf_clnt.o zoe/module_rpconly_zoe_module_vdec_intf_clnt.o \
 zoe/module_zoe_module_mgr_intf_srv.o zoe/module_zoe_module_connection_intf_srv.o zoe/module_zoe_module_control_intf_srv.o \
 zoe/module_zoe_module_data_intf_srv.o zoe/fw_vpu_if.o zoe/bus_cpciecntl.o zoe/bus_cpcieintf.o \
 zoe/bus_cusbcntl.o zoe/bus_cusbintf.o zoe/bus_chpucntl.o zoe/bus_chpuintf.o \
 zoe/bus_zvdrv_interface.o zoe/shared_zoe_cfifo.o zoe/shared_zoe_cobject.o \
 zoe/shared_zoe_cobjectmgr.o zoe/shared_zoe_cqueue.o zoe/shared_zoe_cthread.o zoe/shared_zoe_util.o zoe/core_loader.o

# zoe/module_zoe_module_vdec_intf_srv.o zoe/module_zoe_module_vdec_adaptor.o zoe/fw_vsys_one.o
# zoe/fw_vpu_if_impl.o zoe/fw_vdec_logo.o zoe/fw_freescale_logo.o zoe/fw_freescale_yuv.o zoe/shared_zoe_cpp_helper.o

ifeq ($(CONFIG_MXC_VPU_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif

