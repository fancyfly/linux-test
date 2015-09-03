/*
** ###################################################################
**     Build:               b150220
**
**     Abstract:
**         Register bit field access macros.
**
**     Copyright (c) 2015 Freescale Semiconductor, Inc.
**     All rights reserved.
**
**     Redistribution and use in source and binary forms, with or without modification,
**     are permitted provided that the following conditions are met:
**
**     o Redistributions of source code must retain the above copyright notice, this list
**       of conditions and the following disclaimer.
**
**     o Redistributions in binary form must reproduce the above copyright notice, this
**       list of conditions and the following disclaimer in the documentation and/or
**       other materials provided with the distribution.
**
**     o Neither the name of Freescale Semiconductor, Inc. nor the names of its
**       contributors may be used to endorse or promote products derived from this
**       software without specific prior written permission.
**
**     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
**     ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
**     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
**     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
**     ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
**     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
**     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
**     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
**     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
**     SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**
**     http:                 www.freescale.com
**     mail:                 support@freescale.com
**
**     Revisions:
**
** ###################################################################
*/

/*!
 * @file  devices/MX8/include/fsl_bitaccess.h
 *
 * Header file containing register access macros.
 *
 * \addtogroup Peripheral_access_layer (HAL) Device Peripheral Access Layer
 *
 * @{
 */

#ifndef _FSL_BITACCESS_H
#define _FSL_BITACCESS_H  1

/*!
 * @addtogroup SCF Register Access Macros
 * @{
 */

/*
 * Macros for single instance registers
 */

#define BF_SET(reg, field)       HW_##reg##_SET(BM_##reg##_##field)
#define BF_CLR(reg, field)       HW_##reg##_CLR(BM_##reg##_##field)
#define BF_TOG(reg, field)       HW_##reg##_TOG(BM_##reg##_##field)

#define BF_SETV(reg, field, v)   HW_##reg##_SET(BF_##reg##_##field(v))
#define BF_CLRV(reg, field, v)   HW_##reg##_CLR(BF_##reg##_##field(v))
#define BF_TOGV(reg, field, v)   HW_##reg##_TOG(BF_##reg##_##field(v))

#define BV_FLD(reg, field, sym)  BF_##reg##_##field(BV_##reg##_##field##__##sym)
#define BV_VAL(reg, field, sym)  BV_##reg##_##field##__##sym

#define BF_RD(reg, field)        HW_##reg.B.field
#define BF_WR(reg, field, v)     BW_##reg##_##field(v)


/*******************************************************************************
 * Macros to create bitfield mask, shift, and width from CMSIS definitions
 ******************************************************************************/

/* Bitfield Mask */
#define SCF_BMSK(bit) (bit ## _MASK)

/* Bitfield Left Shift */
#define SCF_BLSH(bit) (bit ## _SHIFT)

/* Bitfield Width */
#define SCF_BWID(bit) (bit ## _WIDTH)

/* Bitfield Value */
#define SCF_BVAL(bit, val) ((val) << (SCF_BLSH(bit)))


/*******************************************************************************
 * Macros to set, clear, extact, and insert bitfields into register structures
 * or local variables
 ******************************************************************************/

/* Bitfield Set */
#define SCF_BSET(var, bit) (var |= (SCF_BMSK(bit)))

/* Bitfield Clear */
#define SCF_BCLR(var, bit) (var &= (~(SCF_BMSK(bit))))

/* Bitfield Extract */
#define SCF_BEXR(var, bit) ((var & (SCF_BMSK(bit))) >> (SCF_BLSH(bit)))

/* Bitfield Insert */
#define SCF_BINS(var, bit, val) (var = (var & (~(SCF_BMSK(bit)))) | SCF_BVAL(bit, val))


/*******************************************************************************
 * Macros to set, clear, extact, and insert bitfields into register structures
 * that support SCT
 ******************************************************************************/

#ifdef EMUL
/* Emulation does not have SCT hardware and must fallback to non-SCT definitions */

/* SCT Bitfield Set */
#define SCF_SCT_BSET(var, bit) (SCF_BSET(var, bit))

/* SCT Bitfield Clear */
#define SCF_SCT_BCLR(var, bit) (SCF_BCLR(var, bit))

/* SCT Bitfield Insert */
#define SCF_SCT_BINS(var, bit, val) (SCF_BINS(var, bit, val))

#else
/*!  @todo Port macros leverage SCT register access hardware */

/* SCT Bitfield Set */
#define SCF_SCT_BSET(var, bit) (SCF_BSET(var, bit))

/* SCT Bitfield Clear */
#define SCF_SCT_BCLR(var, bit) (SCF_BCLR(var, bit))

/* SCT Bitfield Insert */
#define SCF_SCT_BINS(var, bit, val) (SCF_BINS(var, bit, val))

#endif /* EMUL */

/*!
 * @}
 */ /* end of group SCF */

/*!
 * @}
 */ /* end of group Peripheral_access_layer */

#endif /* _FSL_BITACCESS_H */

/******************************************************************************/
