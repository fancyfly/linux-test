/*
 * Copyright 2004-2007 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

/*!
 * @file mxc_nd3.h
 *
 * @brief This file contains the NAND Flash Controller register information on 
 * MXC30031(SkyePlus ADS).
 *
 *
 * @ingroup NAND_MTD
 */

#ifndef __MXC_ND_H__
#define __MXC_ND_H__

#include <asm/hardware.h>

/*
 * Addresses for NFC registers
 */

/* AXI Bus Mapped */
#define NFC_FLASH_ADDR_CMD      (*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0xE00)))
#define NFC_CONFIG1             (*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0xE04)))
#define NFC_ECC_STATUS_RESULT   (*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0xE08)))
#define LAUNCH_NFC              (*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0xE0C)))

/* IP Bus Mapped */

#define NFC_WRPROT              (*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x00)))
#define NFC_WRPROT_UNLOCK_BLK_ADD0         \
								(*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x04)))
#define NFC_WRPROT_UNLOCK_BLK_ADD1         \
								(*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x08)))
#define NFC_WRPROT_UNLOCK_BLK_ADD2         \
								(*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x0C)))
#define NFC_WRPROT_UNLOCK_BLK_ADD3         \
								(*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x10)))
#define NFC_CONFIG2             (*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x14)))
#define NFC_IPC					(*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x18)))
#define NFC_AXI_ERR_ADD			(*((volatile u32 *)IO_ADDRESS(NFC_BASE_ADDR_IP + 0x1C)))

/*!
 * Addresses for NFC RAM BUFFER Main area 0
 */
#define MAIN_AREA0        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x000)
#define MAIN_AREA1        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x200)
#define MAIN_AREA2        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x400)
#define MAIN_AREA3        (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x600)

/*!
 * Addresses for NFC SPARE BUFFER Spare area 0
 */
#define SPARE_AREA0       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x800)
#define SPARE_AREA1       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x810)
#define SPARE_AREA2       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x820)
#define SPARE_AREA3       (volatile u16 *)IO_ADDRESS(NFC_BASE_ADDR_AXI + 0x830)

/*!
 * Set FCMD to 1, rest to 0 in LAUNCH_NFC Register for Command
 * operation
 */
#define NFC_CMD            0x1

/*!
 * Set FADD to 1, rest to 0 in LAUNCH_NFC Register for Address
 * operation
 */
#define NFC_ADDR           0x2

/*!
 * Set FDI to 1, rest to 0 in LAUNCH_NFC Register for Input
 * operation
 */
#define NFC_INPUT          0x4

/*!
 * Set FDO to 001, rest to 0 in LAUNCH_NFC Register for Data Output
 * operation
 */
#define NFC_OUTPUT         0x8

/*!
 * Set FD0 to 010, rest to 0 in LAUNCH_NFC Register for Read ID
 * operation
 */
#define NFC_ID             0x10

/*!
 * Set FDO to 100, rest to 0 in LAUNCH_NFC Register for Read Status
 * operation
 */
#define NFC_STATUS         0x20

/* Set INT 1, reset to 0 in NFC_IPC Register for Interupt operation.*/
#define NFC_INT            0x80000000
#define NFC_IPC_RB_B	   (1<<29)

/* Bit Definitions in Register NFC_CONFIG2 */
#define NFC_ECC_EN          (1 << 3)
#define NFC_INT_MSK         (1 << 4)
#define NFC_BIG             (1 << 5)
#define NFC_RST             (1 << 6)

/* Bit Definitions in Register NFC_CONFIG1 */
#define NFC_SP_EN           (1)
#define NFC_CE              (1 << 1)

#define NFC_FLASH_ADDR_SHIFT 16

#define NFC_SET_BLS_BITS     0x00030000
#define NFC_BLS_UNLOCK       0xFFFEFFFF
#define NFC_SET_WPC_BITS     0x00000007
#define NFC_WPC_UNLOCK       0xFFFFFFFC
#define NFC_UEBA0			 16
#define NFC_SET_RBA_BITS     0x00000030
#define NFC_SET_RBA0         0xFFFFFFCF
#define NFC_SET_RBA1         0xFFFFFFDF
#define NFC_SET_RBA2         0xFFFFFFEF
#define NFC_SET_RBA3         0xFFFFFFFF
#define NFC_SET_PPB_BITS     0x00000180
#define NFC_SET_PPB64        0xFFFFFEFF

#endif				/* MXCND_H */
