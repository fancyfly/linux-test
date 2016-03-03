/* ========================================================================== */
/*  Project  : cafe                                                           */
/*  Module   : med_me_vid_buffer                                              */
/*  File     : med_me_vid_buffer.h                                            */
/*                                                                            */
/*  Author   : reggen 1.55                                                    */
/*  Created  :                                                                */
/* -------------------------------------------------------------------------- */
/*  Comments :      !! AUTO-GENERATED-FILE - DO NOT EDIT !!                   */
/*                                                                            */
/*  COPYRIGHT:     Zenverge Inc. Proprietary and Confidential                 */
/*                      Copyright (c) 2015, Zenverge Inc.                     */
/*                            All rights reserved.                            */
/*                                                                            */
/*         Permission to use, modify or copy this code is prohibited          */
/*           without the express written consent of Zenverge Inc.             */
/*                                                                            */
/* ========================================================================== */

#ifndef __MED_ME_VID_BUFFER_H__
#define __MED_ME_VID_BUFFER_H__

/** @ingroup rdl
 *  @file med_me_vid_buffer.h
 */
#define MED_ME_VID_BUFFER_PARTITION_ENABLE                            0x00000000
#define MED_ME_VID_BUFFER_NUM_OF_UNIT_ROWS                            0x00000004
#define MED_ME_VID_BUFFER_REF_PIC_WIDTH                               0x00000008
#define MED_ME_VID_BUFFER_BUFFER_HEIGHT                               0x0000000C
#define MED_ME_VID_BUFFER_REF_BASE_ADDRESS                            0x00000010
#define MED_ME_VID_BUFFER_SRC_MBY_UNIT_DIST_REQD_FROM_TOP_OF_BUFFER   0x00000014
#define MED_ME_VID_BUFFER_SRC_MBX_DIST_REQD_FROM_LEFT_OF_BUFFER       0x00000018
#define MED_ME_VID_BUFFER_REF_DMA_STRIDE                              0x0000001C
#define MED_ME_VID_BUFFER_REF_DMA_DDR_MODE                            0x00000020
#define MED_ME_VID_BUFFER_PIC_MODE                                    0x00000024
#define MED_ME_VID_BUFFER_REF_PIC_UPPER_LINE_IN_BUF                   0x00000080
#define MED_ME_VID_BUFFER_REF_PIC_LOWER_LINE_IN_BUF                   0x00000084
#define MED_ME_VID_BUFFER_REF_PIC_X_POS_UPPER_LINE_IN_BUF             0x00000088
#define MED_ME_VID_BUFFER_REF_PIC_X_POS_LOWER_LINE_IN_BUF             0x0000008C
#define MED_ME_VID_BUFFER_PART_STATUS                                 0x00000090
#define MED_ME_VID_BUFFER_OUTSTANDING_DATA_STATUS                     0x00000094

#endif
