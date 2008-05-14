#
# Makefile for the linux kernel.
#

# Object file lists.

obj-y		:= system.o iomux.o cpu.o mm.o clock.o devices.o serial.o usb.o
obj-$(CONFIG_MXC_SDMA_API) 	+= dma.o
obj-$(CONFIG_MACH_MX35_3DS)	+= mx35_3stack.o mx35_3stack_gpio.o mx35_3stack_cpld.o
obj-$(CONFIG_MACH_MX35EVB)	+= mx35evb.o mx35evb_cpld.o mx35evb_gpio.o

obj-$(CONFIG_MXC_PSEUDO_IRQS)	+= mx35_3stack_irq.o
obj-$(CONFIG_PM) 		+= pm.o
