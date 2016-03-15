/*
 * Copyright 2005-2016 Freescale Semiconductor, Inc.
 */
/*
All rights reserved.
  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:
  o Redistributions of source code must retain the above copyright notice,
    this list of conditions and the following disclaimer.
  o Redistributions in binary form must reproduce the above copyright notice,
    thislist of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  o Neither the name of Freescale Semiconductor, Inc. nor the names of its
    contributors may be used to endorse or promote products derived from this
    software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IMXDPU_INTSTEER_H
#define IMXDPU_INTSTEER_H

#define IMXDPU_IRQSTEER_OFFSET          0
#define IMXDPU_IRQSTEER_CHANnCTL_OFFSET 0

#define IMXDPU_IRQSTEER_CHANnCTL_0      1<<0
#define IMXDPU_IRQSTEER_CHANnCTL_1      1<<1
#define IMXDPU_IRQSTEER_CHANnCTL_2      1<<2
#define IMXDPU_IRQSTEER_CHANnCTL_3      1<<3

#define IMXDPU_IRQSTEER_CHnMASKn_OFFSET(__reg_offset__) ((__reg_offset__) + 0x4)
#define IMXDPU_IRQSTEER_CHn_SETn_OFFSET(__reg_offset__) ((__reg_offset__) + 0x44)
#define IMXDPU_IRQSTEER_CHn_STATUSn_OFFSET(__reg_offset__) ((__reg_offset__) + 0x84)
#define IMXDPU_IRQSTEER_CHn_MASKn_OFFSET(__reg_offset__) ((__reg_offset__)+4)
#define IMXDPU_IRQSTEER_CHn_MINT_OFFSET     0xc4
#define IMXDPU_IRQSTEER_CHn_MSTRSTAT_OFFSET 0xc8

int imxdpu_intsteer_get_line(int imxdpu_irq);
int imxdpu_intsteer_get_reg_shift(int imxdpu_irq);
int imxdpu_intsteer_get_reg_offset(int imxdpu_irq);
int imxdpu_intsteer_get_irq_offset(int imxdpu_irq);
uint32_t imxdpu_ss_read(void __iomem * ss_base, uint32_t offset);
void imxdpu_ss_write(void __iomem * ss_base, uint32_t offset, uint32_t value);
int imxdpu_intsteer_enable_irq(void __iomem * ss_base, int imxdpu_irq);
int imxdpu_intsteer_disable_irq(void __iomem * ss_base, int imxdpu_irq);

#endif				/* IMXDPU_INTSTEER_H */
