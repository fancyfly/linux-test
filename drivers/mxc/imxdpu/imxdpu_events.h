/*
 * Copyright 2005-2015 Freescale Semiconductor, Inc.
 */
/*
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

o Redistributions of source code must retain the above copyright notice, this list
  of conditions and the following disclaimer.

o Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

o Neither the name of Freescale Semiconductor, Inc. nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef IMXDPU_EVENTS_H
#define IMXDPU_EVENTS_H

/*  Shadow load (Blit Engine) */
#define IMXDPU_STORE9_SHDLOAD_IRQ	 0U
#define IMXDPU_STORE9_SHDLOAD_CMD	 0U

/*  Frame complete (Blit Engine) */
#define IMXDPU_STORE9_FRAMECOMPLETE_IRQ	 1U
#define IMXDPU_STORE9_FRAMECOMPLETE_CMD	 1U

/*  Sequence complete (Blit Engine) */
#define IMXDPU_STORE9_SEQCOMPLETE_IRQ	 2U
#define IMXDPU_STORE9_SEQCOMPLETE_CMD	 2U

/*  Shadow load (Display Controller Content Stream 0) */
#define IMXDPU_EXTDST0_SHDLOAD_IRQ	 3U
#define IMXDPU_EXTDST0_SHDLOAD_CMD	 3U

/*  Frame complete (Display Controller Content Stream 0) */
#define IMXDPU_EXTDST0_FRAMECOMPLETE_IRQ	 4U
#define IMXDPU_EXTDST0_FRAMECOMPLETE_CMD	 4U

/*  Sequence complete (Display Controller Content Stream 0) */
#define IMXDPU_EXTDST0_SEQCOMPLETE_IRQ	 5U
#define IMXDPU_EXTDST0_SEQCOMPLETE_CMD	 5U

/*  Shadow load (Display Controller Safety Stream 0) */
#define IMXDPU_EXTDST4_SHDLOAD_IRQ	 6U
#define IMXDPU_EXTDST4_SHDLOAD_CMD	 6U

/*  Frame complete (Display Controller Safety Stream 0) */
#define IMXDPU_EXTDST4_FRAMECOMPLETE_IRQ	 7U
#define IMXDPU_EXTDST4_FRAMECOMPLETE_CMD	 7U

/*  Sequence complete (Display Controller Safety Stream 0) */
#define IMXDPU_EXTDST4_SEQCOMPLETE_IRQ	 8U
#define IMXDPU_EXTDST4_SEQCOMPLETE_CMD	 8U

/*  Shadow load (Display Controller Content Stream 1) */
#define IMXDPU_EXTDST1_SHDLOAD_IRQ	 9U
#define IMXDPU_EXTDST1_SHDLOAD_CMD	 9U

/*  Frame complete (Display Controller Content Stream 1) */
#define IMXDPU_EXTDST1_FRAMECOMPLETE_IRQ	 10U
#define IMXDPU_EXTDST1_FRAMECOMPLETE_CMD	 10U

/*  Sequence complete (Display Controller Content Stream 1) */
#define IMXDPU_EXTDST1_SEQCOMPLETE_IRQ	 11U
#define IMXDPU_EXTDST1_SEQCOMPLETE_CMD	 11U

/*  Shadow load (Display Controller Safety Stream 1) */
#define IMXDPU_EXTDST5_SHDLOAD_IRQ	 12U
#define IMXDPU_EXTDST5_SHDLOAD_CMD	 12U

/*  Frame complete (Display Controller Safety Stream 1) */
#define IMXDPU_EXTDST5_FRAMECOMPLETE_IRQ	 13U
#define IMXDPU_EXTDST5_FRAMECOMPLETE_CMD	 13U

/*  Sequence complete (Display Controller Safety Stream 1) */
#define IMXDPU_EXTDST5_SEQCOMPLETE_IRQ	 14U
#define IMXDPU_EXTDST5_SEQCOMPLETE_CMD	 14U

/*  Shadow load (Capture Controller Storage Stream 0) */
#define IMXDPU_STORE4_SHDLOAD_IRQ	 15U
#define IMXDPU_STORE4_SHDLOAD_CMD	 15U

/*  Frame complete (Capture Controller Storage Stream 0) */
#define IMXDPU_STORE4_FRAMECOMPLETE_IRQ	 16U
#define IMXDPU_STORE4_FRAMECOMPLETE_CMD	 16U

/*  Sequence complete (Capture Controller Storage Stream 0) */
#define IMXDPU_STORE4_SEQCOMPLETE_IRQ	 17U
#define IMXDPU_STORE4_SEQCOMPLETE_CMD	 17U

/*  Shadow load (Capture Controller Storage Stream 1) */
#define IMXDPU_STORE5_SHDLOAD_IRQ	 18U
#define IMXDPU_STORE5_SHDLOAD_CMD	 18U

/*  Frame complete (Capture Controller Storage Stream 1) */
#define IMXDPU_STORE5_FRAMECOMPLETE_IRQ	 19U
#define IMXDPU_STORE5_FRAMECOMPLETE_CMD	 19U

/*  Sequence complete (Capture Controller Storage Stream 1) */
#define IMXDPU_STORE5_SEQCOMPLETE_IRQ	 20U
#define IMXDPU_STORE5_SEQCOMPLETE_CMD	 20U

/*  Reserved */
#define IMXDPU_RESERVED21_IRQ	 21U
#define IMXDPU_RESERVED21_CMD	 21U

/*  Measurement valid (Video/Capture Plane 0 Histogram #4 unit) */
#define IMXDPU_HISTOGRAM4_VALID_IRQ	 22U
#define IMXDPU_HISTOGRAM4_VALID_CMD	 22U

/*  Reserved */
#define IMXDPU_RESERVED23_IRQ	 23U
#define IMXDPU_RESERVED23_CMD	 23U

/*  Measurement valid (Video/Capture Plane 1 Histogram #5 unit) */
#define IMXDPU_HISTOGRAM5_VALID_IRQ	 24U
#define IMXDPU_HISTOGRAM5_VALID_CMD	 24U

/*  Error condition (Display Plane 0 FrameDump #0 unit) */
#define IMXDPU_FRAMEDUMP0_ERROR_IRQ	 25U
#define IMXDPU_FRAMEDUMP0_ERROR_CMD	 25U

/*  Error condition (Display Plane 1 FrameDump #1 unit) */
#define IMXDPU_FRAMEDUMP1_ERROR_IRQ	 26U
#define IMXDPU_FRAMEDUMP1_ERROR_CMD	 26U

/*  Shadow load (Display Controller Display Stream 0) */
#define IMXDPU_DISENGCFG_SHDLOAD0_IRQ	 27U
#define IMXDPU_DISENGCFG_SHDLOAD0_CMD	 27U

/*  Frame complete (Display Controller Display Stream 0) */
#define IMXDPU_DISENGCFG_FRAMECOMPLETE0_IRQ	 28U
#define IMXDPU_DISENGCFG_FRAMECOMPLETE0_CMD	 28U

/*  Sequence complete (Display Controller Display Stream 0) */
#define IMXDPU_DISENGCFG_SEQCOMPLETE0_IRQ	 29U
#define IMXDPU_DISENGCFG_SEQCOMPLETE0_CMD	 29U

/*  Programmable interrupt 0 (Display Controller Display Stream 0 */
#define IMXDPU_FRAMEGEN0_INT0_IRQ	 30U
#define IMXDPU_FRAMEGEN0_INT0_CMD	 30U

/*  Programmable interrupt 1 (Display Controller Display Stream 0 */
#define IMXDPU_FRAMEGEN0_INT1_IRQ	 31U
#define IMXDPU_FRAMEGEN0_INT1_CMD	 31U

/*  Programmable interrupt 2 (Display Controller Display Stream 0 */
#define IMXDPU_FRAMEGEN0_INT2_IRQ	 32U
#define IMXDPU_FRAMEGEN0_INT2_CMD	 32U

/*  Programmable interrupt 3 (Display Controller Display Stream 0 */
#define IMXDPU_FRAMEGEN0_INT3_IRQ	 33U
#define IMXDPU_FRAMEGEN0_INT3_CMD	 33U

/*  Shadow load (Display Controller Display Stream 0 */
#define IMXDPU_SIG0_SHDLOAD_IRQ	 34U
#define IMXDPU_SIG0_SHDLOAD_CMD	 34U

/*  Measurement valid (Display Controller Display Stream 0 */
#define IMXDPU_SIG0_VALID_IRQ	 35U
#define IMXDPU_SIG0_VALID_CMD	 35U

/*  Error condition (Display Controller Display Stream 0 */
#define IMXDPU_SIG0_ERROR_IRQ	 36U
#define IMXDPU_SIG0_ERROR_CMD	 36U

/*  Shadow load (Display Controller Display Stream 1) */
#define IMXDPU_DISENGCFG_SHDLOAD1_IRQ	 37U
#define IMXDPU_DISENGCFG_SHDLOAD1_CMD	 37U

/*  Frame complete (Display Controller Display Stream 1) */
#define IMXDPU_DISENGCFG_FRAMECOMPLETE1_IRQ	 38U
#define IMXDPU_DISENGCFG_FRAMECOMPLETE1_CMD	 38U

/*  Sequence complete (Display Controller Display Stream 1) */
#define IMXDPU_DISENGCFG_SEQCOMPLETE1_IRQ	 39U
#define IMXDPU_DISENGCFG_SEQCOMPLETE1_CMD	 39U

/*  Programmable interrupt 0 (Display Controller Display Stream 1 */
#define IMXDPU_FRAMEGEN1_INT0_IRQ	 40U
#define IMXDPU_FRAMEGEN1_INT0_CMD	 40U

/*  Programmable interrupt 1 (Display Controller Display Stream 1 */
#define IMXDPU_FRAMEGEN1_INT1_IRQ	 41U
#define IMXDPU_FRAMEGEN1_INT1_CMD	 41U

/*  Programmable interrupt 2 (Display Controller Display Stream 1 */
#define IMXDPU_FRAMEGEN1_INT2_IRQ	 42U
#define IMXDPU_FRAMEGEN1_INT2_CMD	 42U

/*  Programmable interrupt 3 (Display Controller Display Stream 1 */
#define IMXDPU_FRAMEGEN1_INT3_IRQ	 43U
#define IMXDPU_FRAMEGEN1_INT3_CMD	 43U

/*  Shadow load (Display Controller Display Stream 1 */
#define IMXDPU_SIG1_SHDLOAD_IRQ	 44U
#define IMXDPU_SIG1_SHDLOAD_CMD	 44U

/*  Measurement valid (Display Controller Display Stream 1 */
#define IMXDPU_SIG1_VALID_IRQ	 45U
#define IMXDPU_SIG1_VALID_CMD	 45U

/*  Error condition (Display Controller Display Stream 1 */
#define IMXDPU_SIG1_ERROR_IRQ	 46U
#define IMXDPU_SIG1_ERROR_CMD	 46U

/*  Error condition (ItuIfc #4 unit Capture Plane 0) */
#define IMXDPU_ITUIFC4_ERROR_IRQ	 47U
#define IMXDPU_ITUIFC4_ERROR_CMD	 47U

/*  Error condition (ItuIfc #5 unit Capture Plane 1) */
#define IMXDPU_ITUIFC5_ERROR_IRQ	 48U
#define IMXDPU_ITUIFC5_ERROR_CMD	 48U

/*  Reserved do not use */
#define IMXDPU_RESERVED49_IRQ	  49U
#define IMXDPU_RESERVED49_CMD	 49U

/*  Error condition (Command Sequencer) */
#define IMXDPU_CMDSEQ_ERROR_IRQ	 50U
#define IMXDPU_CMDSEQ_ERROR_CMD	 50U

/*  Software interrupt 0 (Common Control) */
#define IMXDPU_COMCTRL_SW0_IRQ	 51U
#define IMXDPU_COMCTRL_SW0_CMD	 51U

/*  Software interrupt 1 (Common Control) */
#define IMXDPU_COMCTRL_SW1_IRQ	 52U
#define IMXDPU_COMCTRL_SW1_CMD	 52U

/*  Software interrupt 2 (Common Control) */
#define IMXDPU_COMCTRL_SW2_IRQ	 53U
#define IMXDPU_COMCTRL_SW2_CMD	 53U

/*  Software interrupt 3 (Common Control) */
#define IMXDPU_COMCTRL_SW3_IRQ	 54U
#define IMXDPU_COMCTRL_SW3_CMD	 54U

/*  Synchronization status activated (Display Controller Safety stream 0) */
#define IMXDPU_FRAMEGEN0_PRIMSYNC_ON_IRQ	 55U
#define IMXDPU_FRAMEGEN0_PRIMSYNC_ON_CMD	 55U

/*  Synchronization status deactivated (Display Controller Safety stream 0) */
#define IMXDPU_FRAMEGEN0_PRIMSYNC_OFF_IRQ	 56U
#define IMXDPU_FRAMEGEN0_PRIMSYNC_OFF_CMD	 56U

/*  Synchronization status activated (Display Controller Content stream 0) */
#define IMXDPU_FRAMEGEN0_SECSYNC_ON_IRQ	 57U
#define IMXDPU_FRAMEGEN0_SECSYNC_ON_CMD	 57U

/*  Synchronization status deactivated (Display Controller Content stream 0) */
#define IMXDPU_FRAMEGEN0_SECSYNC_OFF_IRQ	 58U
#define IMXDPU_FRAMEGEN0_SECSYNC_OFF_CMD	 58U

/*  Synchronization status activated (Display Controller Safety stream 1) */
#define IMXDPU_FRAMEGEN1_PRIMSYNC_ON_IRQ	 59U
#define IMXDPU_FRAMEGEN1_PRIMSYNC_ON_CMD	 59U

/*  Synchronization status deactivated (Display Controller Safety stream 1) */
#define IMXDPU_FRAMEGEN1_PRIMSYNC_OFF_IRQ	 60U
#define IMXDPU_FRAMEGEN1_PRIMSYNC_OFF_CMD	 60U

/*  Synchronization status activated (Display Controller Content stream 1) */
#define IMXDPU_FRAMEGEN1_SECSYNC_ON_IRQ	 61U
#define IMXDPU_FRAMEGEN1_SECSYNC_ON_CMD	 61U

/*  Synchronization status deactivated (Display Controller Content stream 1) */
#define IMXDPU_FRAMEGEN1_SECSYNC_OFF_IRQ	 62U
#define IMXDPU_FRAMEGEN1_SECSYNC_OFF_CMD	 62U

/*  Synchronization status activated (FrameCap #4 unit Capture Plane 0) */
#define IMXDPU_FRAMECAP4_SYNC_ON_IRQ	 63U
#define IMXDPU_FRAMECAP4_SYNC_ON_CMD	 63U

/*  Synchronization status deactivated (FrameCap #4 unit Capture Plane 0) */
#define IMXDPU_FRAMECAP4_SYNC_OFF_IRQ	 64U
#define IMXDPU_FRAMECAP4_SYNC_OFF_CMD	 64U

/*  Synchronization status activated (FrameCap #5 unit Capture Plane 1) */
#define IMXDPU_FRAMECAP5_SYNC_ON_IRQ	 65U
#define IMXDPU_FRAMECAP5_SYNC_ON_CMD	 65U

/*  Synchronization status deactivated (FrameCap #5 unit Capture Plane 1) */
#define IMXDPU_FRAMECAP5_SYNC_OFF_IRQ	 66U
#define IMXDPU_FRAMECAP5_SYNC_OFF_CMD	 66U

/*  Synchronization status (Display Controller Safety stream 0) */
#define IMXDPU_FRAMEGEN0_PRIMSYNC_CMD	 67U
#define IMXDPU_FRAMEGEN0_PRIMSYNC_STS	 0U

/*  Synchronization status (Display Controller Content stream 0) */
#define IMXDPU_FRAMEGEN0_SECSYNC_CMD	 68U
#define IMXDPU_FRAMEGEN0_SECSYNC_STS	 1U

/*  Synchronization status (Display Controller Safety stream 1) */
#define IMXDPU_FRAMEGEN1_PRIMSYNC_CMD	 69U
#define IMXDPU_FRAMEGEN1_PRIMSYNC_STS	 2U

/*  Synchronization status (Display Controller Content stream 1) */
#define IMXDPU_FRAMEGEN1_SECSYNC_CMD	 70U
#define IMXDPU_FRAMEGEN1_SECSYNC_STS	 3U

/*  Synchronization status (FrameCap #4 unit Capture Plane 0) */
#define IMXDPU_FRAMECAP4_SYNC_CMD	 71U
#define IMXDPU_FRAMECAP4_SYNC_STS	 4U

/*  Synchronization status (FrameCap #5 unit Capture Plane 1) */
#define IMXDPU_FRAMECAP5_SYNC_CMD	 72U
#define IMXDPU_FRAMECAP5_SYNC_STS	 5U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_STORE9_SHDLDREQ_CMD	 73U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_EXTDST0_SHDLDREQ_CMD	 74U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_EXTDST4_SHDLDREQ_CMD	 75U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_EXTDST1_SHDLDREQ_CMD	 76U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_EXTDST5_SHDLDREQ_CMD	 77U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_STORE4_SHDLDREQ_CMD	 78U

/*  Shadow load request (Display Controller Pixel Engine configuration */
#define IMXDPU_PIXENGCFG_STORE5_SHDLDREQ_CMD	 79U

/*  Shadow load request (Blit Engine FetchDecode #9 tree) */
#define IMXDPU_PIXENGCFG_FETCHDECODE9_SHDLDREQ_CMD	 80U

/*  Shadow load request (Blit Engine FetchPersp #9 tree) */
#define IMXDPU_PIXENGCFG_FETCHPERSP9_SHDLDREQ_CMD	 81U

/*  Shadow load request (Blit Engine FetchEco #9 tree) */
#define IMXDPU_PIXENGCFG_FETCHECO9_SHDLDREQ_CMD	 82U

/*  Shadow load request (Display Controller ConstFrame #0 tree) */
#define IMXDPU_PIXENGCFG_CONSTFRAME0_SHDLDREQ_CMD	 83U

/*  Shadow load request (Display Controller ConstFrame #4 tree) */
#define IMXDPU_PIXENGCFG_CONSTFRAME4_SHDLDREQ_CMD	 84U

/*  Shadow load request (Display Controller ConstFrame #1 tree) */
#define IMXDPU_PIXENGCFG_CONSTFRAME1_SHDLDREQ_CMD	 85U

/*  Shadow load request (Display Controller ConstFrame #5 tree) */
#define IMXDPU_PIXENGCFG_CONSTFRAME5_SHDLDREQ_CMD	 86U

/*  Shadow load request (Display Controller ExtSrc #4 tree) */
#define IMXDPU_PIXENGCFG_EXTSRC4_SHDLDREQ_CMD	 87U

/*  Shadow load request (Display Controller ExtSrc #5 tree) */
#define IMXDPU_PIXENGCFG_EXTSRC5_SHDLDREQ_CMD	 88U

/*  Shadow load request (Display Controller FetchDecode #2 tree) */
#define IMXDPU_PIXENGCFG_FETCHDECODE2_SHDLDREQ_CMD	 89U

/*  Shadow load request (Display Controller FetchDecode #3 tree) */
#define IMXDPU_PIXENGCFG_FETCHDECODE3_SHDLDREQ_CMD	 90U

/*  Shadow load request (Display Controller FetchWarp #2 tree) */
#define IMXDPU_PIXENGCFG_FETCHWARP2_SHDLDREQ_CMD	 91U

/*  Shadow load request (Display Controller FetchEco #2 tree) */
#define IMXDPU_PIXENGCFG_FETCHECO2_SHDLDREQ_CMD	 92U

/*  Shadow load request (Display Controller FetchDecode #0 tree) */
#define IMXDPU_PIXENGCFG_FETCHDECODE0_SHDLDREQ_CMD	 93U

/*  Shadow load request (Display Controller FetchEco #0 tree) */
#define IMXDPU_PIXENGCFG_FETCHECO0_SHDLDREQ_CMD	 94U

/*  Shadow load request (Display Controller FetchDecode #1 tree) */
#define IMXDPU_PIXENGCFG_FETCHDECODE1_SHDLDREQ_CMD	 95U

/*  Shadow load request (Display Controller FetchEco #1 tree) */
#define IMXDPU_PIXENGCFG_FETCHECO1_SHDLDREQ_CMD	 96U

/*  Shadow load request (Display Controller FetchLayer #0 tree) */
#define IMXDPU_PIXENGCFG_FETCHLAYER0_SHDLDREQ_CMD	 97U

/*  Shadow load request (Display Controller FetchLayer #1 tree) */
#define IMXDPU_PIXENGCFG_FETCHLAYER1_SHDLDREQ_CMD	 98U

/*  Shadow load request (Display Controller ExtSrc #0 tree) */
#define IMXDPU_PIXENGCFG_EXTSRC0_SHDLDREQ_CMD	 99U

/*  Shadow load request (Display Controller ExtSrc #1 tree) */
#define IMXDPU_PIXENGCFG_EXTSRC1_SHDLDREQ_CMD	 100U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ0_CMD	 101U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ1_CMD	 102U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ2_CMD	 103U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ3_CMD	 104U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ4_CMD	 105U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ5_CMD	 106U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ6_CMD	 107U

/*  Shadow load request (Display Controller FetchLayer #0 unit */
#define IMXDPU_FETCHLAYER0_SHDLDREQ7_CMD	 108U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ0_CMD	 109U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ1_CMD	 110U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ2_CMD	 111U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ3_CMD	 112U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ4_CMD	 113U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ5_CMD	 114U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ6_CMD	 115U

/*  Shadow load request (Display Controller FetchWarp #1 unit */
#define IMXDPU_FETCHWARP1_SHDLDREQ7_CMD	 116U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ0_CMD	 117U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ1_CMD	 118U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ2_CMD	 119U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ3_CMD	 120U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ4_CMD	 121U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ5_CMD	 122U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ6_CMD	 123U

/*  Shadow load request (Display Controller FetchLayer #1 unit */
#define IMXDPU_FETCHLAYER1_SHDLDREQ7_CMD	 124U

#endif				/* IMXDPU_EVENTS */
