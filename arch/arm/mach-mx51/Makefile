#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y   := system.o iomux.o cpu.o mm.o clock.o devices.o serial.o dma.o

obj-$(CONFIG_MACH_MX51_3STACK) += mx51_3stack.o mx51_3stack_gpio.o

ifneq ($(strip $(CONFIG_USB_GADGET_ARC) $(CONFIG_USB_EHCI_ARC_OTG)),)
	obj-y	+= usb_dr.o
endif

