
/*
 * Copyright (C) 2011 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/regulator/consumer.h>
#include <linux/fsl_devices.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-int-device.h>
#include "mxc_v4l2_capture.h"

#define MT9V114_VOLTAGE_ANALOG               2800000
#define MT9V114_VOLTAGE_DIGITAL_CORE         1800000
#define MT9V114_VOLTAGE_DIGITAL_IO           2800000

#define MIN_FPS 15
#define MAX_FPS 30
#define DEFAULT_FPS 30

#define MT9V114_XCLK_MIN 4000000
#define MT9V114_XCLK_MAX 44000000

enum mt9v114_mode {
	mt9v114_mode_MIN = 0,
	mt9v114_mode_VGA_640_480 = 0,
	mt9v114_mode_CIF_352_288 = 1,
	mt9v114_mode_MAX = 1
};

enum mt9v114_frame_rate {
	mt9v114_15_fps,
	mt9v114_30_fps
};

enum white_balance_mode {
	white_balance_MIN = 0,
	white_balance_cloudy1 = 0,
	white_balance_sunshine = 1,
	white_balance_cloudy2 = 2,
	white_balance_daylight = 3,
	white_balance_filament = 4,
	white_balance_auto = 5,
	white_balance_MAX = 5,
};

enum special_effect_mode {
	special_effect_MIN = 0,
	special_effect_effect_off = 0,
	special_effect_negtive = 1,
	special_effect_WB = 2,
	special_effect_MAX = 2,
};

struct reg_value {
	u16 u16RegAddr;
	u16 u16Val;
	u32 u32Delay_ms;
	bool double_bytes;
};

struct mt9v114_mode_info {
	enum mt9v114_mode mode;
	u32 width;
	u32 height;
	struct reg_value *init_data_ptr;
	u32 init_data_size;
};

struct mt9v114_white_balance_info {
	enum white_balance_mode white_balance_mode;
	struct reg_value *init_data_ptr;
	u32 init_data_size;
};

struct mt9v114_special_effect_info {
	enum special_effect_mode effect_mode;
	struct reg_value *init_data_ptr;
	u32 init_data_size;
};


/*!
 * Maintains the information on the current state of the sesor.
 */
struct sensor {
	const struct mt9v114_platform_data *platform_data;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_captureparm streamcap;
	bool on;

	/* control settings */
	int brightness;
	int hue;
	int contrast;
	int saturation;
	int red;
	int green;
	int blue;
	int ae_mode;

	u32 mclk;
	int csi;
} mt9v114_data;

static struct reg_value mt9v114_special_effect_off[] = {
	{0x098e, 0x8400, 0, 1},
	{0x8400, 0x01, 0, 0},
	{0x3040, 0x0041, 0, 1},
	{0x8400, 0x02, 0, 0},
};

static struct reg_value mt9v114_special_effect_negtive[] = {
	{0x098e, 0x8400, 0, 1},
	{0x8400, 0x01, 0, 0},
	{0xa010, 0x03, 0, 1},
	{0x8400, 0x02, 0, 0},
};

static struct reg_value mt9v114_special_effect_BW[] = {
	{0x098e, 0x8400, 0, 1},
	{0x8400, 0x01, 0, 0},
	{0xa010, 0x01, 0, 0},
	{0x8400, 0x02, 0, 0},
};

static struct reg_value mt9v114_white_balance_cloudy1[] = {
	{0x9401, 0x0c, 0, 0},
	{0x9436, 0x3d, 0, 0},
	{0x9437, 0x5b, 0, 0},
};

static struct reg_value mt9v114_white_balance_sunshine[] = {
	{0x9401, 0x0c, 0, 0},
	{0x9436, 0x42, 0, 0},
	{0x9437, 0x56, 0, 0},
};

static struct reg_value mt9v114_white_balance_cloudy2[] = {
	{0x9401, 0x0c, 0, 0},
	{0x9436, 0x4b, 0, 0},
	{0x9437, 0x49, 0, 0},
};

static struct reg_value mt9v114_white_balance_daylight[] = {
	{0x9401, 0x0c, 0, 0},
	{0x9436, 0x4d, 0, 0},
	{0x9437, 0x3c, 0, 0},
};

static struct reg_value mt9v114_white_balance_filamentlight[] = {
	{0x9401, 0x0c, 0, 0},
	{0x9436, 0x6b, 0, 0},
	{0x9437, 0x2d, 0, 0},
};

static struct reg_value mt9v114_white_balance_auto[] = {
	{0x9401, 0x0d, 0, 0},
};

static struct mt9v114_white_balance_info mt9v114_white_balance_mode_info_data[white_balance_MAX + 1] = {
	{white_balance_cloudy1, mt9v114_white_balance_cloudy1, ARRAY_SIZE(mt9v114_white_balance_cloudy1)},
	{white_balance_sunshine, mt9v114_white_balance_sunshine, ARRAY_SIZE(mt9v114_white_balance_sunshine)},
	{white_balance_cloudy2, mt9v114_white_balance_cloudy2, ARRAY_SIZE(mt9v114_white_balance_cloudy2)},
	{white_balance_daylight, mt9v114_white_balance_daylight, ARRAY_SIZE(mt9v114_white_balance_daylight)},
	{white_balance_filament, mt9v114_white_balance_filamentlight, ARRAY_SIZE(mt9v114_white_balance_filamentlight)},
	{white_balance_auto, mt9v114_white_balance_auto, ARRAY_SIZE(mt9v114_white_balance_auto)},
};

static struct mt9v114_special_effect_info mt9v114_special_effect_info_data[special_effect_MAX + 1] = {
	{special_effect_effect_off, mt9v114_special_effect_off, ARRAY_SIZE(mt9v114_special_effect_off)},
	{special_effect_negtive, mt9v114_special_effect_negtive, ARRAY_SIZE(mt9v114_special_effect_negtive)},
	{special_effect_WB, mt9v114_special_effect_BW, ARRAY_SIZE(mt9v114_special_effect_BW)},
};

static struct reg_value mt9v114_setting_30fps_CIF_352_288[] = {
	{0x001A, 0x0124, 5, 1}, /* RESET_AND_MISC_CONTROL*/

	{0x0010, 0x031A, 0, 1},	/* PLL_DIVIDERS*/
	{0x0012, 0x0300, 0, 1}, /* PLL_P_DIVIDERS*/
	{0x001E, 0x0771, 0, 1}, /* PAD_SLEW*/
	{0x0018, 0x0006, 100, 1}, /* STANDBY_CONTROL_AND_STATUS*/

	/* POLL STANDBY_CONTROL_AND_STATUS::FW_IN_STANDBY => 0x00*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x8082, 0x0194, 0, 1},
	{0x8084, 0x0163, 0, 1},
	{0x8086, 0x0107, 0, 1},
	{0x8088, 0x01C7, 0, 1},
	{0x808A, 0x01A1, 0, 1},
	{0x808C, 0x022A, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x0982, 0x0000, 0, 1}, /* ACCESS_CTL_STAT*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x8098, 0x3C3C, 0, 1},
	{0x809A, 0x1300, 0, 1},
	{0x809C, 0x0147, 0, 1},
	{0x809E, 0xCC31, 0, 1},
	{0x80A0, 0x8230, 0, 1},
	{0x80A2, 0xED02, 0, 1},
	{0x80A4, 0xCC00, 0, 1},
	{0x80A6, 0x90ED, 0, 1},
	{0x80A8, 0x00C6, 0, 1},
	{0x80AA, 0x04BD, 0, 1},
	{0x80AC, 0xDDBD, 0, 1},
	{0x80AE, 0x5FD7, 0, 1},
	{0x80B0, 0x8E86, 0, 1},
	{0x80B2, 0x90BD, 0, 1},
	{0x80B4, 0x0330, 0, 1},

	{0x80B6, 0xDD90, 0, 1},
	{0x80B8, 0xCC92, 0, 1},
	{0x80BA, 0x02BD, 0, 1},
	{0x80BC, 0x0330, 0, 1},
	{0x80BE, 0xDD92, 0, 1},
	{0x80C0, 0xCC94, 0, 1},
	{0x80C2, 0x04BD, 0, 1},
	{0x80C4, 0x0330, 0, 1},
	{0x80C6, 0xDD94, 0, 1},
	{0x80C8, 0xCC96, 0, 1},
	{0x80CA, 0x00BD, 0, 1},
	{0x80CC, 0x0330, 0, 1},
	{0x80CE, 0xDD96, 0, 1},
	{0x80D0, 0xCC07, 0, 1},
	{0x80D2, 0xFFDD, 0, 1},
	{0x80D4, 0x8ECC, 0, 1},
	{0x80D6, 0x3180, 0, 1},
	{0x80D8, 0x30ED, 0, 1},
	{0x80DA, 0x02CC, 0, 1},
	{0x80DC, 0x008E, 0, 1},
	{0x80DE, 0xED00, 0, 1},
	{0x80E0, 0xC605, 0, 1},
	{0x80E2, 0xBDDE, 0, 1},
	{0x80E4, 0x13BD, 0, 1},
	{0x80E6, 0x03FA, 0, 1},
	{0x80E8, 0x3838, 0, 1},
	{0x80EA, 0x3913, 0, 1},
	{0x80EC, 0x0001, 0, 1},
	{0x80EE, 0x0109, 0, 1},
	{0x80F0, 0xBC01, 0, 1},
	{0x80F2, 0x9D26, 0, 1},
	{0x80F4, 0x0813, 0, 1},
	{0x80F6, 0x0004, 0, 1},
	{0x80F8, 0x0108, 0, 1},
	{0x80FA, 0xFF02, 0, 1},
	{0x80FC, 0xEF7E, 0, 1},
	{0x80FE, 0xC278, 0, 1},
	{0x8330, 0x364F, 0, 1},
	{0x8332, 0x36CE, 0, 1},
	{0x8334, 0x02F3, 0, 1},
	{0x8336, 0x3AEC, 0, 1},
	{0x8338, 0x00CE, 0, 1},
	{0x833A, 0x0018, 0, 1},

	{0x833C, 0xBDE4, 0, 1},
	{0x833E, 0x5197, 0, 1},
	{0x8340, 0x8F38, 0, 1},
	{0x8342, 0xEC00, 0, 1},
	{0x8344, 0x938E, 0, 1},
	{0x8346, 0x2C02, 0, 1},
	{0x8348, 0x4F5F, 0, 1},
	{0x834A, 0x3900, 0, 1},
	{0x83E4, 0x3C13, 0, 1},
	{0x83E6, 0x0001, 0, 1},
	{0x83E8, 0x0CCC, 0, 1},
	{0x83EA, 0x3180, 0, 1},
	{0x83EC, 0x30ED, 0, 1},
	{0x83EE, 0x00CC, 0, 1},
	{0x83F0, 0x87FF, 0, 1},
	{0x83F2, 0xBDDD, 0, 1},
	{0x83F4, 0xF5BD, 0, 1},
	{0x83F6, 0xC2A9, 0, 1},
	{0x83F8, 0x3839, 0, 1},
	{0x83FA, 0xFE02, 0, 1},
	{0x83FC, 0xEF7E, 0, 1},
	{0x83FE, 0x00EB, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x83E0, 0x0098, 0, 1},
	{0x83E2, 0x03E4, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x300A, 0x03BE, 0, 1}, /* FRAME_LENGTH_LINES*/
	{0x300C, 0x02D6, 0, 1}, /* LINE_LENGTH_PCK*/
	{0x3010, 0x0012, 0, 1}, /* FINE_CORRECTION*/
	{0x9803, 0x07, 0, 0},	/* STAT_FD_ZONE_HEIGHT*/
	{0xA06E, 0x0098, 0, 1}, /* CAM_FD_CONFIG_FDPERIOD_50HZ*/
	{0xA070, 0x007E, 0, 1}, /* CAM_FD_CONFIG_FDPERIOD_60HZ*/
	{0xA072, 0x11, 0, 0},	/* CAM_FD_CONFIG_SEARCH_F1_50*/
	{0xA073, 0x13, 0, 0},	/* CAM_FD_CONFIG_SEARCH_F2_50*/
	{0xA074, 0x14, 0, 0}, /* CAM_FD_CONFIG_SEARCH_F1_60*/
	{0xA075, 0x16, 0, 0}, /* CAM_FD_CONFIG_SEARCH_F2_60*/
	{0xA076, 0x0014, 0, 1}, /* CAM_FD_CONFIG_MAX_FDZONE_50HZ*/
	{0xA078, 0x0018, 0, 1}, /* CAM_FD_CONFIG_MAX_FDZONE_60HZ*/
	{0x3E22, 0x3307, 0, 1}, /* SAMP_BOOST_ROW*/
	{0x3ECE, 0x4311, 0, 1}, /* DAC_LD_2_3*/
	{0x3ED0, 0x16AF, 0, 1}, /* DAC_LD_4_5*/
	{0x3210, 0x00B0, 0, 1}, /* COLOR_PIPELINE_CONTROL*/

	{0x3640, 0x0170, 0, 1}, /* P_G1_P0Q0*/
	{0x3642, 0xD6AC, 0, 1}, /* P_G1_P0Q1*/
	{0x3644, 0x12B0, 0, 1}, /* P_G1_P0Q2*/
	{0x3646, 0xD5A9, 0, 1}, /* P_G1_P0Q3*/
	{0x3648, 0x9A10, 0, 1}, /* P_G1_P0Q4*/
	{0x364A, 0x01B0, 0, 1}, /* P_R_P0Q0*/
	{0x364C, 0xB0AC, 0, 1}, /* P_R_P0Q1*/
	{0x364E, 0x12F0, 0, 1}, /* P_R_P0Q2*/
	{0x3650, 0xA0CC, 0, 1}, /* P_R_P0Q3*/
	{0x3652, 0xDF4F, 0, 1}, /* P_R_P0Q4*/
	{0x3654, 0x0130, 0, 1}, /* P_B_P0Q0*/
	{0x3656, 0xA88C, 0, 1}, /* P_B_P0Q1*/
	{0x3658, 0x0610, 0, 1}, /* P_B_P0Q2*/
	{0x365A, 0xEEEC, 0, 1}, /* P_B_P0Q3*/
	{0x365C, 0x8850, 0, 1}, /* P_B_P0Q4*/
	{0x365E, 0x0270, 0, 1}, /* P_G2_P0Q0*/
	{0x3660, 0xC94C, 0, 1}, /* P_G2_P0Q1*/
	{0x3662, 0x1470, 0, 1}, /* P_G2_P0Q2*/
	{0x3664, 0x8A09, 0, 1}, /* P_G2_P0Q3*/
	{0x3666, 0x98F0, 0, 1}, /* P_G2_P0Q4*/
	{0x3680, 0x1EAB, 0, 1}, /* P_G1_P1Q0*/
	{0x3682, 0x528B, 0, 1}, /* P_G1_P1Q1*/
	{0x3684, 0x9CAC, 0, 1}, /* P_G1_P1Q2*/
	{0x3686, 0xD1ED, 0, 1}, /* P_G1_P1Q3*/
	{0x3688, 0x744C, 0, 1}, /* P_G1_P1Q4*/
	{0x368A, 0x7CEC, 0, 1}, /* P_R_P1Q0*/
	{0x368C, 0x1289, 0, 1}, /* P_R_P1Q1*/
	{0x368E, 0xBB4D, 0, 1}, /* P_R_P1Q2*/
	{0x3690, 0xF74D, 0, 1}, /* P_R_P1Q3*/
	{0x3692, 0xAB6D, 0, 1}, /* P_R_P1Q4*/
	{0x3694, 0x304C, 0, 1}, /* P_B_P1Q0*/
	{0x3696, 0x4FAB, 0, 1}, /* P_B_P1Q1*/
	{0x3698, 0xDB6D, 0, 1}, /* P_B_P1Q2*/
	{0x369A, 0x892E, 0, 1}, /* P_B_P1Q3*/
	{0x369C, 0x14EC, 0, 1}, /* P_B_P1Q4*/
	{0x369E, 0x360A, 0, 1}, /* P_G2_P1Q0*/
	{0x36A0, 0x050C, 0, 1}, /* P_G2_P1Q1*/
	{0x36A2, 0xB8CD, 0, 1}, /* P_G2_P1Q2*/
	{0x36A4, 0x9BCD, 0, 1}, /* P_G2_P1Q3*/
	{0x36A6, 0x0A4F, 0, 1}, /* P_G2_P1Q4*/
	{0x36C0, 0x764F, 0, 1}, /* P_G1_P2Q0*/
	{0x36C2, 0x202E, 0, 1}, /* P_G1_P2Q1*/
	{0x36C4, 0x82F1, 0, 1}, /* P_G1_P2Q2*/

	{0x36C6, 0xB7AF, 0, 1}, /* P_G1_P2Q3*/
	{0x36C8, 0x6390, 0, 1}, /* P_G1_P2Q4*/
	{0x36CA, 0x1CF0, 0, 1}, /* P_R_P2Q0 */
	{0x36CC, 0x8A4C, 0, 1}, /* P_R_P2Q1 */
	{0x36CE, 0x9311, 0, 1}, /* P_R_P2Q2 */
	{0x36D0, 0x8B4F, 0, 1}, /* P_R_P2Q3 */
	{0x36D2, 0x4DB0, 0, 1}, /* P_R_P2Q4 */
	{0x36D4, 0x0C10, 0, 1}, /* P_B_P2Q0 */
	{0x36D6, 0x7ECD, 0, 1}, /* P_B_P2Q1 */
	{0x36D8, 0xA191, 0, 1}, /* P_B_P2Q2 */
	{0x36DA, 0xCE2F, 0, 1}, /* P_B_P2Q3 */
	{0x36DC, 0x5A50, 0, 1}, /* P_B_P2Q4 */
	{0x36DE, 0x720F, 0, 1}, /* P_G2_P2Q0*/
	{0x36E0, 0x34EE, 0, 1}, /* P_G2_P2Q1*/
	{0x36E2, 0xDEB0, 0, 1}, /* P_G2_P2Q2*/
	{0x36E4, 0xD2AF, 0, 1}, /* P_G2_P2Q3*/
	{0x36E6, 0x4B8F, 0, 1}, /* P_G2_P2Q4*/
	{0x3700, 0xC0EF, 0, 1}, /* P_G1_P3Q0*/
	{0x3702, 0xED0E, 0, 1}, /* P_G1_P3Q1*/
	{0x3704, 0xA690, 0, 1}, /* P_G1_P3Q2*/
	{0x3706, 0x6ED0, 0, 1}, /* P_G1_P3Q3*/
	{0x3708, 0x58AE, 0, 1}, /* P_G1_P3Q4*/
	{0x370A, 0x8250, 0, 1}, /* P_R_P3Q0 */
	{0x370C, 0x65EE, 0, 1}, /* P_R_P3Q1 */
	{0x370E, 0xB76F, 0, 1}, /* P_R_P3Q2 */
	{0x3710, 0x682E, 0, 1}, /* P_R_P3Q3 */
	{0x3712, 0x1A11, 0, 1}, /* P_R_P3Q4 */
	{0x3714, 0x8C70, 0, 1}, /* P_B_P3Q0 */
	{0x3716, 0x6FCD, 0, 1}, /* P_B_P3Q1 */
	{0x3718, 0xD1AF, 0, 1}, /* P_B_P3Q2 */
	{0x371A, 0x190F, 0, 1}, /* P_B_P3Q3 */
	{0x371C, 0x6871, 0, 1}, /* P_B_P3Q4  */
	{0x371E, 0xBF2F, 0, 1}, /* P_G2_P3Q0 */
	{0x3720, 0xCC2E, 0, 1}, /* P_G2_P3Q1 */
	{0x3722, 0xC4EE, 0, 1}, /* P_G2_P3Q2 */
	{0x3724, 0x78CF, 0, 1}, /* P_G2_P3Q3 */
	{0x3726, 0xBAD1, 0, 1}, /* P_G2_P3Q4 */
	{0x3740, 0x89B1, 0, 1}, /* P_G1_P4Q0 */
	{0x3742, 0xA0D0, 0, 1}, /* P_G1_P4Q1 */
	{0x3744, 0xBE6D, 0, 1}, /* P_G1_P4Q2 */
	{0x3746, 0x6B11, 0, 1}, /* P_G1_P4Q3*/
	{0x3748, 0xD78E, 0, 1}, /* P_G1_P4Q4*/
	{0x374A, 0xA171, 0, 1}, /* P_R_P4Q0 */

	{0x374C, 0x56EF, 0, 1},        /* P_R_P4Q1*/
	{0x374E, 0x0CD0, 0, 1},		/*P_R_P4Q2*/
	{0x3750, 0x0CF0, 0, 1},        /* P_R_P4Q3*/
	{0x3752, 0x66D0, 0, 1},        /* P_R_P4Q4*/
	{0x3754, 0xB731, 0, 1}, /* P_B_P4Q0*/
	{0x3756, 0x9C4E, 0, 1},	/* P_B_P4Q1*/
	{0x3758, 0xED2C, 0, 1},	/* P_B_P4Q2*/
	{0x375A, 0x1E71, 0, 1},	/* P_B_P4Q3*/
	{0x375C, 0x1713, 0, 1}, /* P_B_P4Q4        */
	{0x375E, 0x8A11, 0, 1},	/* P_G2_P4Q0*/
	{0x3760, 0xA550, 0, 1},	/* P_G2_P4Q1*/
	{0x3762, 0xC20E, 0, 1},	/* P_G2_P4Q2*/
	{0x3764, 0x52F1, 0, 1}, /* P_G2_P4Q3*/
	{0x3766, 0x228F, 0, 1}, /* P_G2_P4Q4*/
	{0x3782, 0x012C, 0, 1}, /* CENTER_ROW*/
	{0x3784, 0x0160, 0, 1}, /* CENTER_COLUMN*/
	{0x3210, 0x00B8, 0, 1}, /* COLOR_PIPELINE_CONTROL*/
	{0xA02F, 0x0259, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_0*/
	{0xA031, 0xFF65, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_1*/
	{0xA033, 0xFF5F, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_2*/
	{0xA035, 0xFFD8, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_3*/
	{0xA037, 0x00E1, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_4*/
	{0xA039, 0x0064, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_5*/
	{0xA03B, 0xFF5B, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_6*/
	{0xA03D, 0xFE72, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_7*/
	{0xA03F, 0x0351, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_8*/
	{0xA041, 0x0021, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_9*/
	{0xA043, 0x004A, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_10*/
	{0xA045, 0x0000, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_0*/
	{0xA047, 0xFFAB, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_1*/
	{0xA049, 0x0055, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_2*/
	{0xA04B, 0x000C, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_3*/
	{0xA04D, 0x001F, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_4*/
	{0xA04F, 0xFFD5, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_5*/
	{0xA051, 0x0097, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_6*/
	{0xA053, 0x00AF, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_7*/
	{0xA055, 0xFEB9, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_8*/
	{0xA057, 0x0010, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_9*/
	{0xA059, 0xFFDD, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_10*/
	{0x940A, 0x0000, 0, 1},	/* AWB_X_START            */
	{0x940C, 0x0000, 0, 1}, /* AWB_Y_START                    */
	{0x940E, 0x027F, 0, 1}, /* AWB_X_END                      */
	{0xA061, 0x002A, 0, 1},	/* CAM_AWB_CONFIG_X_SHIFT_PRE_ADJ*/

	{0xA063, 0x0038, 0, 1}, /* CAM_AWB_CONFIG_Y_SHIFT_PRE_ADJ */
	{0xA065, 0x04, 0, 0}, /* CAM_AWB_CONFIG_X_SCALE */
	{0xA066, 0x02, 0, 0}, /* CAM_AWB_CONFIG_Y_SCALE */
	{0x9409, 0xF0, 0, 0}, /* AWB_LUMA_THRESH_HIGH   */
	{0x9416, 0x2D, 0, 0}, /* AWB_R_SCENE_RATIO_LOWER*/
	{0x9417, 0x8C, 0, 0}, /* AWB_R_SCENE_RATIO_UPPER*/
	{0x9418, 0x16, 0, 0}, /* AWB_B_SCENE_RATIO_LOWER*/
	{0x9419, 0x78, 0, 0}, /* AWB_B_SCENE_RATIO_UPPER*/
	{0x2112, 0x0000, 0, 1}, /* AWB_WEIGHT_R0        */
	{0x2114, 0x0000, 0, 1}, /* AWB_WEIGHT_R1        */
	{0x2116, 0x0000, 0, 1}, /* AWB_WEIGHT_R2        */
	{0x2118, 0x0F80, 0, 1}, /* AWB_WEIGHT_R3*/
	{0x211A, 0x2A80, 0, 1}, /* AWB_WEIGHT_R4*/
	{0x211C, 0x0B40, 0, 1}, /* AWB_WEIGHT_R5*/
	{0x211E, 0x01AC, 0, 1}, /* AWB_WEIGHT_R6*/
	{0x2120, 0x0038, 0, 1}, /* AWB_WEIGHT_R7*/
	{0x326E, 0x0006, 0, 1}, /* LOW_PASS_YUV_FILTER*/
	{0x33F4, 0x000B, 0, 1}, /* KERNEL_CONFIG*/
	{0xA087, 0x00, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_0*/
	{0xA088, 0x07, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_1*/
	{0xA089, 0x16, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_2*/
	{0xA08A, 0x30, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_3*/
	{0xA08B, 0x52, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_4*/
	{0xA08C, 0x6D, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_5*/
	{0xA08D, 0x86, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_6*/
	{0xA08E, 0x9B, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_7*/
	{0xA08F, 0xAB, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_8*/
	{0xA090, 0xB9, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_9*/
	{0xA091, 0xC5, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_10*/
	{0xA092, 0xCF, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_11*/
	{0xA093, 0xD8, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_12*/
	{0xA094, 0xE0, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_13*/
	{0xA095, 0xE7, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_14*/
	{0xA096, 0xEE, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_15*/
	{0xA097, 0xF4, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_16*/
	{0xA098, 0xFA, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_17*/
	{0xA0AD, 0x0001, 0, 1}, /* CAM_LL_CONFIG_GAMMA_START_BM       */
	{0xA0AF, 0x0338, 0, 1}, /* CAM_LL_CONFIG_GAMMA_STOP_BM        */
	{0xA0B1, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_RED_START*/
	{0xA0B2, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_RED_STOP*/
	{0xA0B3, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_GREEN_START*/
	{0xA0B4, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_GREEN_STOP */
	{0xA0B5, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_BLUE_START */

	{0xA0B6, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_BLUE_STOP  */
	{0xA0B7, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_MIN_MAX_START*/
	{0xA0B8, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_MIN_MAX_STOP*/
	{0xA0B9, 0x0040, 0, 1}, /* CAM_LL_CONFIG_START_GAIN_METRIC*/
	{0xA0BB, 0x00C8, 0, 1}, /* CAM_LL_CONFIG_STOP_GAIN_METRIC*/
	{0xA07A, 0x04, 0, 0}, /* CAM_LL_CONFIG_AP_THRESH_START*/
	{0xA07B, 0x0F, 0, 0}, /* CAM_LL_CONFIG_AP_THRESH_STOP*/
	{0xA07C, 0x03, 0, 0}, /* CAM_LL_CONFIG_AP_GAIN_START*/
	{0xA07D, 0x00, 0, 0}, /* CAM_LL_CONFIG_AP_GAIN_STOP*/
	{0xA07E, 0x0078, 0, 1}, /* CAM_LL_CONFIG_CDC_THRESHOLD_BM*/
	{0xA080, 0x05, 0, 0}, /* CAM_LL_CONFIG_CDC_GATE_PERCENTAGE*/
	{0xA081, 0x28, 0, 0}, /* CAM_LL_CONFIG_DM_EDGE_TH_START*/
	{0xA082, 0x32, 0, 0}, /* CAM_LL_CONFIG_DM_EDGE_TH_STOP*/
	{0xA083, 0x000F, 0, 1}, /* CAM_LL_CONFIG_FTB_AVG_YSUM_START*/
	{0xA085, 0x0000, 0, 1}, /* CAM_LL_CONFIG_FTB_AVG_YSUM_STOP */
	{0xA020, 0x4B, 0, 0}, /* CAM_AE_CONFIG_BASE_TARGET         */
	{0xA027, 0x0050, 0, 1}, /* CAM_AE_CONFIG_MIN_VIRT_AGAIN    */
	{0xA029, 0x0140, 0, 1}, /* CAM_AE_CONFIG_MAX_VIRT_AGAIN    */
	{0xA025, 0x0080, 0, 1}, /* CAM_AE_CONFIG_MAX_VIRT_DGAIN    */
	{0xA01C, 0x00C8, 0, 1}, /* CAM_AE_CONFIG_TARGET_AGAIN      */
	{0xA01E, 0x0080, 0, 1}, /* CAM_AE_CONFIG_TARGET_DGAIN      */
	{0xA01A, 0x000B, 0, 1}, /* CAM_AE_CONFIG_TARGET_FDZONE     */
	{0xA05F, 0xA0, 0, 0}, /* CAM_AWB_CONFIG_START_SATURATION   */
	{0xA060, 0x28, 0, 0}, /* CAM_AWB_CONFIG_END_SATURATION*/
	{0xA05B, 0x0005, 0, 1}, /* CAM_AWB_CONFIG_START_BRIGHTNESS_BM*/
	{0xA05D, 0x0023, 0, 1}, /* CAM_AWB_CONFIG_STOP_BRIGHTNESS_BM */
	{0x9801, 0x000F, 0, 1}, /* STAT_CLIP_MAX                     */
	{0x9C02, 0x02, 0, 0}, /* LL_GAMMA_SELECT                     */
	{0x9001, 0x05, 0, 0}, /* AE_TRACK_MODE                       */
	{0x9007, 0x05, 0, 0}, /* AE_TRACK_TARGET_GATE                */
	{0x9003, 0x2D, 0, 0}, /* AE_TRACK_BLACK_LEVEL_MAX            */
	{0x9004, 0x02, 0, 0}, /* AE_TRACK_BLACK_LEVEL_STEP_SIZE      */
	{0x9005, 0x23, 0, 0}, /* AE_TRACK_BLACK_CLIP_PERCENT         */
	{0x8C03, 0x01, 0, 0}, /* FD_STAT_MIN                         */
	{0x8C04, 0x03, 0, 0}, /* FD_STAT_MAX*/
	{0x8C05, 0x05, 0, 0}, /* FD_MIN_AMPLITUDE*/
	{0x0018, 0x0002, 0, 1}, /* STANDBY_CONTROL_AND_STATUS*/

	/* amend the deviation of the color*/
	{0x098E, 0x8400 , 0, 1}, /* LOGICAL_ADDRESS_ACCESS [SEQ_CMD]*/
	{0x8400, 0x01, 0, 0}, /* SEQ_CMD                            */
	{0xA02F, 0x018A, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_0           */
	{0xA045, 0x0071, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_0          */
	{0xA031, 0xFFA9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_1           */
	{0xA047, 0xFF87, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_1          */
	{0xA033, 0xFFC9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_2           */
	{0xA049, 0x0040, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_2          */
	{0xA035, 0xFF03, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_3           */
	{0xA04B, 0x0093, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_3          */
	{0xA037, 0x0185, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_4*/
	{0xA04D, 0xFFED, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_4*/
	{0xA039, 0x007D, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_5 */
	{0xA04F, 0xFF76, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_5*/
	{0xA03B, 0xFF79, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_6 */
	{0xA051, 0x008B, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_6*/
	{0xA03D, 0xFEC9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_7 */
	{0xA053, 0x0090, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_7*/
	{0xA03F, 0x02C2, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_8 */
	{0xA055, 0xFF0E, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_8*/
	{0x8400, 0x02, 0, 0}, /* SEQ_CMD                  */

	/*improve the brightness*/
	{0x098E, 0x0808, 0, 1},	/* LOGICAL_ADDRESS_ACCESS [INT_COARSE_INTEGRATION_TIME]*/
	{0x8808, 0x0482, 0, 1},	/* INT_COARSE_INTEGRATION_TIME*/

	/*set to UYVY*/
	{0x098E, 0x8400, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_CMD]	//tried
	{0x8400, 0x01  , 0, 0}, // SEQ_CMD
	{0x332E, 0x0000, 0, 1}, 	// OUTPUT_FORMAT_CONFIGURATION
	{0x3330, 0x0000, 0, 1}, 	// OUTPUT_FORMAT_TEST
	{0x3C00, 0x1002, 0, 1}, 	// TX_CONTROL
	{0x8400, 0x02  , 0, 0},// SEQ_CMD


	{0x098E, 0x8400, 0, 1},
	{0x8400, 0x01, 0, 0},
	{0x300A, 0x01F9, 0, 1},
	{0xA076, 0x0003, 0, 1},
	{0xA078, 0x0004, 0, 1},
	{0xA01A, 0x0003, 0, 1},
	{0xA000, 0x0160, 0, 1},
	{0xA002, 0x0120, 0, 1},
	{0xA004, 0x024B, 0, 1},
	{0xA006, 0x01E0, 0, 1},
	{0x8400, 0x02, 0, 0},
};

static struct reg_value mt9v114_setting_15fps_VGA_640_480[] = {
	{0x001A, 0x0124, 5, 1}, /* RESET_AND_MISC_CONTROL*/

	{0x0010, 0x031A, 0, 1},	/* PLL_DIVIDERS*/
	{0x0012, 0x0300, 0, 1}, /* PLL_P_DIVIDERS*/
	{0x001E, 0x0771, 0, 1}, /* PAD_SLEW*/
	{0x0018, 0x0006, 100, 1}, /* STANDBY_CONTROL_AND_STATUS*/

	/* POLL STANDBY_CONTROL_AND_STATUS::FW_IN_STANDBY => 0x00*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x8082, 0x0194, 0, 1},
	{0x8084, 0x0163, 0, 1},
	{0x8086, 0x0107, 0, 1},
	{0x8088, 0x01C7, 0, 1},
	{0x808A, 0x01A1, 0, 1},
	{0x808C, 0x022A, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x0982, 0x0000, 0, 1}, /* ACCESS_CTL_STAT*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x8098, 0x3C3C, 0, 1},
	{0x809A, 0x1300, 0, 1},
	{0x809C, 0x0147, 0, 1},
	{0x809E, 0xCC31, 0, 1},
	{0x80A0, 0x8230, 0, 1},
	{0x80A2, 0xED02, 0, 1},
	{0x80A4, 0xCC00, 0, 1},
	{0x80A6, 0x90ED, 0, 1},
	{0x80A8, 0x00C6, 0, 1},
	{0x80AA, 0x04BD, 0, 1},
	{0x80AC, 0xDDBD, 0, 1},
	{0x80AE, 0x5FD7, 0, 1},
	{0x80B0, 0x8E86, 0, 1},
	{0x80B2, 0x90BD, 0, 1},
	{0x80B4, 0x0330, 0, 1},

	{0x80B6, 0xDD90, 0, 1},
	{0x80B8, 0xCC92, 0, 1},
	{0x80BA, 0x02BD, 0, 1},
	{0x80BC, 0x0330, 0, 1},
	{0x80BE, 0xDD92, 0, 1},
	{0x80C0, 0xCC94, 0, 1},
	{0x80C2, 0x04BD, 0, 1},
	{0x80C4, 0x0330, 0, 1},
	{0x80C6, 0xDD94, 0, 1},
	{0x80C8, 0xCC96, 0, 1},
	{0x80CA, 0x00BD, 0, 1},
	{0x80CC, 0x0330, 0, 1},
	{0x80CE, 0xDD96, 0, 1},
	{0x80D0, 0xCC07, 0, 1},
	{0x80D2, 0xFFDD, 0, 1},
	{0x80D4, 0x8ECC, 0, 1},
	{0x80D6, 0x3180, 0, 1},
	{0x80D8, 0x30ED, 0, 1},
	{0x80DA, 0x02CC, 0, 1},
	{0x80DC, 0x008E, 0, 1},
	{0x80DE, 0xED00, 0, 1},
	{0x80E0, 0xC605, 0, 1},
	{0x80E2, 0xBDDE, 0, 1},
	{0x80E4, 0x13BD, 0, 1},
	{0x80E6, 0x03FA, 0, 1},
	{0x80E8, 0x3838, 0, 1},
	{0x80EA, 0x3913, 0, 1},
	{0x80EC, 0x0001, 0, 1},
	{0x80EE, 0x0109, 0, 1},
	{0x80F0, 0xBC01, 0, 1},
	{0x80F2, 0x9D26, 0, 1},
	{0x80F4, 0x0813, 0, 1},
	{0x80F6, 0x0004, 0, 1},
	{0x80F8, 0x0108, 0, 1},
	{0x80FA, 0xFF02, 0, 1},
	{0x80FC, 0xEF7E, 0, 1},
	{0x80FE, 0xC278, 0, 1},
	{0x8330, 0x364F, 0, 1},
	{0x8332, 0x36CE, 0, 1},
	{0x8334, 0x02F3, 0, 1},
	{0x8336, 0x3AEC, 0, 1},
	{0x8338, 0x00CE, 0, 1},
	{0x833A, 0x0018, 0, 1},

	{0x833C, 0xBDE4, 0, 1},
	{0x833E, 0x5197, 0, 1},
	{0x8340, 0x8F38, 0, 1},
	{0x8342, 0xEC00, 0, 1},
	{0x8344, 0x938E, 0, 1},
	{0x8346, 0x2C02, 0, 1},
	{0x8348, 0x4F5F, 0, 1},
	{0x834A, 0x3900, 0, 1},
	{0x83E4, 0x3C13, 0, 1},
	{0x83E6, 0x0001, 0, 1},
	{0x83E8, 0x0CCC, 0, 1},
	{0x83EA, 0x3180, 0, 1},
	{0x83EC, 0x30ED, 0, 1},
	{0x83EE, 0x00CC, 0, 1},
	{0x83F0, 0x87FF, 0, 1},
	{0x83F2, 0xBDDD, 0, 1},
	{0x83F4, 0xF5BD, 0, 1},
	{0x83F6, 0xC2A9, 0, 1},
	{0x83F8, 0x3839, 0, 1},
	{0x83FA, 0xFE02, 0, 1},
	{0x83FC, 0xEF7E, 0, 1},
	{0x83FE, 0x00EB, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x83E0, 0x0098, 0, 1},
	{0x83E2, 0x03E4, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x300A, 0x03BE, 0, 1}, /* FRAME_LENGTH_LINES*/
	{0x300C, 0x02D6, 0, 1}, /* LINE_LENGTH_PCK*/
	{0x3010, 0x0012, 0, 1}, /* FINE_CORRECTION*/
	{0x9803, 0x07, 0, 0},	/* STAT_FD_ZONE_HEIGHT*/
	{0xA06E, 0x0098, 0, 1}, /* CAM_FD_CONFIG_FDPERIOD_50HZ*/
	{0xA070, 0x007E, 0, 1}, /* CAM_FD_CONFIG_FDPERIOD_60HZ*/
	{0xA072, 0x11, 0, 0},	/* CAM_FD_CONFIG_SEARCH_F1_50*/
	{0xA073, 0x13, 0, 0},	/* CAM_FD_CONFIG_SEARCH_F2_50*/
	{0xA074, 0x14, 0, 0}, /* CAM_FD_CONFIG_SEARCH_F1_60*/
	{0xA075, 0x16, 0, 0}, /* CAM_FD_CONFIG_SEARCH_F2_60*/
	{0xA076, 0x0014, 0, 1}, /* CAM_FD_CONFIG_MAX_FDZONE_50HZ*/
	{0xA078, 0x0018, 0, 1}, /* CAM_FD_CONFIG_MAX_FDZONE_60HZ*/
	{0x3E22, 0x3307, 0, 1}, /* SAMP_BOOST_ROW*/
	{0x3ECE, 0x4311, 0, 1}, /* DAC_LD_2_3*/
	{0x3ED0, 0x16AF, 0, 1}, /* DAC_LD_4_5*/
	{0x3210, 0x00B0, 0, 1}, /* COLOR_PIPELINE_CONTROL*/

	{0x3640, 0x0170, 0, 1}, /* P_G1_P0Q0*/
	{0x3642, 0xD6AC, 0, 1}, /* P_G1_P0Q1*/
	{0x3644, 0x12B0, 0, 1}, /* P_G1_P0Q2*/
	{0x3646, 0xD5A9, 0, 1}, /* P_G1_P0Q3*/
	{0x3648, 0x9A10, 0, 1}, /* P_G1_P0Q4*/
	{0x364A, 0x01B0, 0, 1}, /* P_R_P0Q0*/
	{0x364C, 0xB0AC, 0, 1}, /* P_R_P0Q1*/
	{0x364E, 0x12F0, 0, 1}, /* P_R_P0Q2*/
	{0x3650, 0xA0CC, 0, 1}, /* P_R_P0Q3*/
	{0x3652, 0xDF4F, 0, 1}, /* P_R_P0Q4*/
	{0x3654, 0x0130, 0, 1}, /* P_B_P0Q0*/
	{0x3656, 0xA88C, 0, 1}, /* P_B_P0Q1*/
	{0x3658, 0x0610, 0, 1}, /* P_B_P0Q2*/
	{0x365A, 0xEEEC, 0, 1}, /* P_B_P0Q3*/
	{0x365C, 0x8850, 0, 1}, /* P_B_P0Q4*/
	{0x365E, 0x0270, 0, 1}, /* P_G2_P0Q0*/
	{0x3660, 0xC94C, 0, 1}, /* P_G2_P0Q1*/
	{0x3662, 0x1470, 0, 1}, /* P_G2_P0Q2*/
	{0x3664, 0x8A09, 0, 1}, /* P_G2_P0Q3*/
	{0x3666, 0x98F0, 0, 1}, /* P_G2_P0Q4*/
	{0x3680, 0x1EAB, 0, 1}, /* P_G1_P1Q0*/
	{0x3682, 0x528B, 0, 1}, /* P_G1_P1Q1*/
	{0x3684, 0x9CAC, 0, 1}, /* P_G1_P1Q2*/
	{0x3686, 0xD1ED, 0, 1}, /* P_G1_P1Q3*/
	{0x3688, 0x744C, 0, 1}, /* P_G1_P1Q4*/
	{0x368A, 0x7CEC, 0, 1}, /* P_R_P1Q0*/
	{0x368C, 0x1289, 0, 1}, /* P_R_P1Q1*/
	{0x368E, 0xBB4D, 0, 1}, /* P_R_P1Q2*/
	{0x3690, 0xF74D, 0, 1}, /* P_R_P1Q3*/
	{0x3692, 0xAB6D, 0, 1}, /* P_R_P1Q4*/
	{0x3694, 0x304C, 0, 1}, /* P_B_P1Q0*/
	{0x3696, 0x4FAB, 0, 1}, /* P_B_P1Q1*/
	{0x3698, 0xDB6D, 0, 1}, /* P_B_P1Q2*/
	{0x369A, 0x892E, 0, 1}, /* P_B_P1Q3*/
	{0x369C, 0x14EC, 0, 1}, /* P_B_P1Q4*/
	{0x369E, 0x360A, 0, 1}, /* P_G2_P1Q0*/
	{0x36A0, 0x050C, 0, 1}, /* P_G2_P1Q1*/
	{0x36A2, 0xB8CD, 0, 1}, /* P_G2_P1Q2*/
	{0x36A4, 0x9BCD, 0, 1}, /* P_G2_P1Q3*/
	{0x36A6, 0x0A4F, 0, 1}, /* P_G2_P1Q4*/
	{0x36C0, 0x764F, 0, 1}, /* P_G1_P2Q0*/
	{0x36C2, 0x202E, 0, 1}, /* P_G1_P2Q1*/
	{0x36C4, 0x82F1, 0, 1}, /* P_G1_P2Q2*/

	{0x36C6, 0xB7AF, 0, 1}, /* P_G1_P2Q3*/
	{0x36C8, 0x6390, 0, 1}, /* P_G1_P2Q4*/
	{0x36CA, 0x1CF0, 0, 1}, /* P_R_P2Q0 */
	{0x36CC, 0x8A4C, 0, 1}, /* P_R_P2Q1 */
	{0x36CE, 0x9311, 0, 1}, /* P_R_P2Q2 */
	{0x36D0, 0x8B4F, 0, 1}, /* P_R_P2Q3 */
	{0x36D2, 0x4DB0, 0, 1}, /* P_R_P2Q4 */
	{0x36D4, 0x0C10, 0, 1}, /* P_B_P2Q0 */
	{0x36D6, 0x7ECD, 0, 1}, /* P_B_P2Q1 */
	{0x36D8, 0xA191, 0, 1}, /* P_B_P2Q2 */
	{0x36DA, 0xCE2F, 0, 1}, /* P_B_P2Q3 */
	{0x36DC, 0x5A50, 0, 1}, /* P_B_P2Q4 */
	{0x36DE, 0x720F, 0, 1}, /* P_G2_P2Q0*/
	{0x36E0, 0x34EE, 0, 1}, /* P_G2_P2Q1*/
	{0x36E2, 0xDEB0, 0, 1}, /* P_G2_P2Q2*/
	{0x36E4, 0xD2AF, 0, 1}, /* P_G2_P2Q3*/
	{0x36E6, 0x4B8F, 0, 1}, /* P_G2_P2Q4*/
	{0x3700, 0xC0EF, 0, 1}, /* P_G1_P3Q0*/
	{0x3702, 0xED0E, 0, 1}, /* P_G1_P3Q1*/
	{0x3704, 0xA690, 0, 1}, /* P_G1_P3Q2*/
	{0x3706, 0x6ED0, 0, 1}, /* P_G1_P3Q3*/
	{0x3708, 0x58AE, 0, 1}, /* P_G1_P3Q4*/
	{0x370A, 0x8250, 0, 1}, /* P_R_P3Q0 */
	{0x370C, 0x65EE, 0, 1}, /* P_R_P3Q1 */
	{0x370E, 0xB76F, 0, 1}, /* P_R_P3Q2 */
	{0x3710, 0x682E, 0, 1}, /* P_R_P3Q3 */
	{0x3712, 0x1A11, 0, 1}, /* P_R_P3Q4 */
	{0x3714, 0x8C70, 0, 1}, /* P_B_P3Q0 */
	{0x3716, 0x6FCD, 0, 1}, /* P_B_P3Q1 */
	{0x3718, 0xD1AF, 0, 1}, /* P_B_P3Q2 */
	{0x371A, 0x190F, 0, 1}, /* P_B_P3Q3 */
	{0x371C, 0x6871, 0, 1}, /* P_B_P3Q4  */
	{0x371E, 0xBF2F, 0, 1}, /* P_G2_P3Q0 */
	{0x3720, 0xCC2E, 0, 1}, /* P_G2_P3Q1 */
	{0x3722, 0xC4EE, 0, 1}, /* P_G2_P3Q2 */
	{0x3724, 0x78CF, 0, 1}, /* P_G2_P3Q3 */
	{0x3726, 0xBAD1, 0, 1}, /* P_G2_P3Q4 */
	{0x3740, 0x89B1, 0, 1}, /* P_G1_P4Q0 */
	{0x3742, 0xA0D0, 0, 1}, /* P_G1_P4Q1 */
	{0x3744, 0xBE6D, 0, 1}, /* P_G1_P4Q2 */
	{0x3746, 0x6B11, 0, 1}, /* P_G1_P4Q3*/
	{0x3748, 0xD78E, 0, 1}, /* P_G1_P4Q4*/
	{0x374A, 0xA171, 0, 1}, /* P_R_P4Q0 */

	{0x374C, 0x56EF, 0, 1},        /* P_R_P4Q1*/
	{0x374E, 0x0CD0, 0, 1},		/*P_R_P4Q2*/
	{0x3750, 0x0CF0, 0, 1},        /* P_R_P4Q3*/
	{0x3752, 0x66D0, 0, 1},        /* P_R_P4Q4*/
	{0x3754, 0xB731, 0, 1}, /* P_B_P4Q0*/
	{0x3756, 0x9C4E, 0, 1},	/* P_B_P4Q1*/
	{0x3758, 0xED2C, 0, 1},	/* P_B_P4Q2*/
	{0x375A, 0x1E71, 0, 1},	/* P_B_P4Q3*/
	{0x375C, 0x1713, 0, 1}, /* P_B_P4Q4        */
	{0x375E, 0x8A11, 0, 1},	/* P_G2_P4Q0*/
	{0x3760, 0xA550, 0, 1},	/* P_G2_P4Q1*/
	{0x3762, 0xC20E, 0, 1},	/* P_G2_P4Q2*/
	{0x3764, 0x52F1, 0, 1}, /* P_G2_P4Q3*/
	{0x3766, 0x228F, 0, 1}, /* P_G2_P4Q4*/
	{0x3782, 0x012C, 0, 1}, /* CENTER_ROW*/
	{0x3784, 0x0160, 0, 1}, /* CENTER_COLUMN*/
	{0x3210, 0x00B8, 0, 1}, /* COLOR_PIPELINE_CONTROL*/
	{0xA02F, 0x0259, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_0*/
	{0xA031, 0xFF65, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_1*/
	{0xA033, 0xFF5F, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_2*/
	{0xA035, 0xFFD8, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_3*/
	{0xA037, 0x00E1, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_4*/
	{0xA039, 0x0064, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_5*/
	{0xA03B, 0xFF5B, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_6*/
	{0xA03D, 0xFE72, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_7*/
	{0xA03F, 0x0351, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_8*/
	{0xA041, 0x0021, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_9*/
	{0xA043, 0x004A, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_10*/
	{0xA045, 0x0000, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_0*/
	{0xA047, 0xFFAB, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_1*/
	{0xA049, 0x0055, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_2*/
	{0xA04B, 0x000C, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_3*/
	{0xA04D, 0x001F, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_4*/
	{0xA04F, 0xFFD5, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_5*/
	{0xA051, 0x0097, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_6*/
	{0xA053, 0x00AF, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_7*/
	{0xA055, 0xFEB9, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_8*/
	{0xA057, 0x0010, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_9*/
	{0xA059, 0xFFDD, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_10*/
	{0x940A, 0x0000, 0, 1},	/* AWB_X_START            */
	{0x940C, 0x0000, 0, 1}, /* AWB_Y_START                    */
	{0x940E, 0x027F, 0, 1}, /* AWB_X_END                      */
	{0xA061, 0x002A, 0, 1},	/* CAM_AWB_CONFIG_X_SHIFT_PRE_ADJ*/

	{0xA063, 0x0038, 0, 1}, /* CAM_AWB_CONFIG_Y_SHIFT_PRE_ADJ */
	{0xA065, 0x04, 0, 0}, /* CAM_AWB_CONFIG_X_SCALE */
	{0xA066, 0x02, 0, 0}, /* CAM_AWB_CONFIG_Y_SCALE */
	{0x9409, 0xF0, 0, 0}, /* AWB_LUMA_THRESH_HIGH   */
	{0x9416, 0x2D, 0, 0}, /* AWB_R_SCENE_RATIO_LOWER*/
	{0x9417, 0x8C, 0, 0}, /* AWB_R_SCENE_RATIO_UPPER*/
	{0x9418, 0x16, 0, 0}, /* AWB_B_SCENE_RATIO_LOWER*/
	{0x9419, 0x78, 0, 0}, /* AWB_B_SCENE_RATIO_UPPER*/
	{0x2112, 0x0000, 0, 1}, /* AWB_WEIGHT_R0        */
	{0x2114, 0x0000, 0, 1}, /* AWB_WEIGHT_R1        */
	{0x2116, 0x0000, 0, 1}, /* AWB_WEIGHT_R2        */
	{0x2118, 0x0F80, 0, 1}, /* AWB_WEIGHT_R3*/
	{0x211A, 0x2A80, 0, 1}, /* AWB_WEIGHT_R4*/
	{0x211C, 0x0B40, 0, 1}, /* AWB_WEIGHT_R5*/
	{0x211E, 0x01AC, 0, 1}, /* AWB_WEIGHT_R6*/
	{0x2120, 0x0038, 0, 1}, /* AWB_WEIGHT_R7*/
	{0x326E, 0x0006, 0, 1}, /* LOW_PASS_YUV_FILTER*/
	{0x33F4, 0x000B, 0, 1}, /* KERNEL_CONFIG*/
	{0xA087, 0x00, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_0*/
	{0xA088, 0x07, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_1*/
	{0xA089, 0x16, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_2*/
	{0xA08A, 0x30, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_3*/
	{0xA08B, 0x52, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_4*/
	{0xA08C, 0x6D, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_5*/
	{0xA08D, 0x86, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_6*/
	{0xA08E, 0x9B, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_7*/
	{0xA08F, 0xAB, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_8*/
	{0xA090, 0xB9, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_9*/
	{0xA091, 0xC5, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_10*/
	{0xA092, 0xCF, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_11*/
	{0xA093, 0xD8, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_12*/
	{0xA094, 0xE0, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_13*/
	{0xA095, 0xE7, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_14*/
	{0xA096, 0xEE, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_15*/
	{0xA097, 0xF4, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_16*/
	{0xA098, 0xFA, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_17*/
	{0xA0AD, 0x0001, 0, 1}, /* CAM_LL_CONFIG_GAMMA_START_BM       */
	{0xA0AF, 0x0338, 0, 1}, /* CAM_LL_CONFIG_GAMMA_STOP_BM        */
	{0xA0B1, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_RED_START*/
	{0xA0B2, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_RED_STOP*/
	{0xA0B3, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_GREEN_START*/
	{0xA0B4, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_GREEN_STOP */
	{0xA0B5, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_BLUE_START */

	{0xA0B6, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_BLUE_STOP  */
	{0xA0B7, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_MIN_MAX_START*/
	{0xA0B8, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_MIN_MAX_STOP*/
	{0xA0B9, 0x0040, 0, 1}, /* CAM_LL_CONFIG_START_GAIN_METRIC*/
	{0xA0BB, 0x00C8, 0, 1}, /* CAM_LL_CONFIG_STOP_GAIN_METRIC*/
	{0xA07A, 0x04, 0, 0}, /* CAM_LL_CONFIG_AP_THRESH_START*/
	{0xA07B, 0x0F, 0, 0}, /* CAM_LL_CONFIG_AP_THRESH_STOP*/
	{0xA07C, 0x03, 0, 0}, /* CAM_LL_CONFIG_AP_GAIN_START*/
	{0xA07D, 0x00, 0, 0}, /* CAM_LL_CONFIG_AP_GAIN_STOP*/
	{0xA07E, 0x0078, 0, 1}, /* CAM_LL_CONFIG_CDC_THRESHOLD_BM*/
	{0xA080, 0x05, 0, 0}, /* CAM_LL_CONFIG_CDC_GATE_PERCENTAGE*/
	{0xA081, 0x28, 0, 0}, /* CAM_LL_CONFIG_DM_EDGE_TH_START*/
	{0xA082, 0x32, 0, 0}, /* CAM_LL_CONFIG_DM_EDGE_TH_STOP*/
	{0xA083, 0x000F, 0, 1}, /* CAM_LL_CONFIG_FTB_AVG_YSUM_START*/
	{0xA085, 0x0000, 0, 1}, /* CAM_LL_CONFIG_FTB_AVG_YSUM_STOP */
	{0xA020, 0x4B, 0, 0}, /* CAM_AE_CONFIG_BASE_TARGET         */
	{0xA027, 0x0050, 0, 1}, /* CAM_AE_CONFIG_MIN_VIRT_AGAIN    */
	{0xA029, 0x0140, 0, 1}, /* CAM_AE_CONFIG_MAX_VIRT_AGAIN    */
	{0xA025, 0x0080, 0, 1}, /* CAM_AE_CONFIG_MAX_VIRT_DGAIN    */
	{0xA01C, 0x00C8, 0, 1}, /* CAM_AE_CONFIG_TARGET_AGAIN      */
	{0xA01E, 0x0080, 0, 1}, /* CAM_AE_CONFIG_TARGET_DGAIN      */
	{0xA01A, 0x000B, 0, 1}, /* CAM_AE_CONFIG_TARGET_FDZONE     */
	{0xA05F, 0xA0, 0, 0}, /* CAM_AWB_CONFIG_START_SATURATION   */
	{0xA060, 0x28, 0, 0}, /* CAM_AWB_CONFIG_END_SATURATION*/
	{0xA05B, 0x0005, 0, 1}, /* CAM_AWB_CONFIG_START_BRIGHTNESS_BM*/
	{0xA05D, 0x0023, 0, 1}, /* CAM_AWB_CONFIG_STOP_BRIGHTNESS_BM */
	{0x9801, 0x000F, 0, 1}, /* STAT_CLIP_MAX                     */
	{0x9C02, 0x02, 0, 0}, /* LL_GAMMA_SELECT                     */
	{0x9001, 0x05, 0, 0}, /* AE_TRACK_MODE                       */
	{0x9007, 0x05, 0, 0}, /* AE_TRACK_TARGET_GATE                */
	{0x9003, 0x2D, 0, 0}, /* AE_TRACK_BLACK_LEVEL_MAX            */
	{0x9004, 0x02, 0, 0}, /* AE_TRACK_BLACK_LEVEL_STEP_SIZE      */
	{0x9005, 0x23, 0, 0}, /* AE_TRACK_BLACK_CLIP_PERCENT         */
	{0x8C03, 0x01, 0, 0}, /* FD_STAT_MIN                         */
	{0x8C04, 0x03, 0, 0}, /* FD_STAT_MAX*/
	{0x8C05, 0x05, 0, 0}, /* FD_MIN_AMPLITUDE*/
	{0x0018, 0x0002, 0, 1}, /* STANDBY_CONTROL_AND_STATUS*/

	/* amend the deviation of the color*/
	{0x098E, 0x8400 , 0, 1}, /* LOGICAL_ADDRESS_ACCESS [SEQ_CMD]*/
	{0x8400, 0x01, 0, 0}, /* SEQ_CMD                            */
	{0xA02F, 0x018A, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_0           */
	{0xA045, 0x0071, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_0          */
	{0xA031, 0xFFA9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_1           */
	{0xA047, 0xFF87, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_1          */
	{0xA033, 0xFFC9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_2           */
	{0xA049, 0x0040, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_2          */
	{0xA035, 0xFF03, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_3           */
	{0xA04B, 0x0093, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_3          */
	{0xA037, 0x0185, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_4*/
	{0xA04D, 0xFFED, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_4*/
	{0xA039, 0x007D, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_5 */
	{0xA04F, 0xFF76, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_5*/
	{0xA03B, 0xFF79, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_6 */
	{0xA051, 0x008B, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_6*/
	{0xA03D, 0xFEC9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_7 */
	{0xA053, 0x0090, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_7*/
	{0xA03F, 0x02C2, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_8 */
	{0xA055, 0xFF0E, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_8*/
	{0x8400, 0x02, 0, 0}, /* SEQ_CMD                  */

	/*improve the brightness*/
	{0x098E, 0x0808, 0, 1},	/* LOGICAL_ADDRESS_ACCESS [INT_COARSE_INTEGRATION_TIME]*/
	{0x8808, 0x0482, 0, 1},	/* INT_COARSE_INTEGRATION_TIME*/

	/*set to UYVY*/
	{0x098E, 0x8400, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_CMD]
	{0x8400, 0x01  , 0, 0}, // SEQ_CMD
	{0x332E, 0x0000, 0, 1}, 	// OUTPUT_FORMAT_CONFIGURATION
	{0x3330, 0x0000, 0, 1}, 	// OUTPUT_FORMAT_TEST
	{0x3C00, 0x1002, 0, 1}, 	// TX_CONTROL
	{0x8400, 0x02  , 0, 0},// SEQ_CMD

	{0x098E, 0x8400, 0, 1},
	{0x8400, 0x01, 20, 0},
	{0x300A, 0x03BE, 0, 1},
	{0xA076, 0x000C, 0, 1},
	{0xA078, 0x000F, 0, 1},
	{0xA01A, 0x000C, 0, 1},
	{0xA004, 0x0280, 0, 1},
	{0xA006, 0x01E0, 0, 1},
	{0xA000, 0x0280, 0, 1},
	{0xA002, 0x01E0, 0, 1},
	{0x8400, 0x02, 20, 0},
};

static struct reg_value mt9v114_init_setting[] = {
	/*[Initial]
	{0x001A, 0x0326, 100, 1},  RESET_AND_MISC_CONTROL
	this command will get no ASK, so do it outside	*/

	{0x001A, 0x0124, 5, 1}, /* RESET_AND_MISC_CONTROL*/

	{0x0010, 0x031A, 0, 1},	/* PLL_DIVIDERS*/
	{0x0012, 0x0300, 0, 1}, /* PLL_P_DIVIDERS*/
	{0x001E, 0x0771, 0, 1}, /* PAD_SLEW*/
	{0x0018, 0x0006, 100, 1}, /* STANDBY_CONTROL_AND_STATUS*/

	/* POLL STANDBY_CONTROL_AND_STATUS::FW_IN_STANDBY => 0x00*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x8082, 0x0194, 0, 1},
	{0x8084, 0x0163, 0, 1},
	{0x8086, 0x0107, 0, 1},
	{0x8088, 0x01C7, 0, 1},
	{0x808A, 0x01A1, 0, 1},
	{0x808C, 0x022A, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x0982, 0x0000, 0, 1}, /* ACCESS_CTL_STAT*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x8098, 0x3C3C, 0, 1},
	{0x809A, 0x1300, 0, 1},
	{0x809C, 0x0147, 0, 1},
	{0x809E, 0xCC31, 0, 1},
	{0x80A0, 0x8230, 0, 1},
	{0x80A2, 0xED02, 0, 1},
	{0x80A4, 0xCC00, 0, 1},
	{0x80A6, 0x90ED, 0, 1},
	{0x80A8, 0x00C6, 0, 1},
	{0x80AA, 0x04BD, 0, 1},
	{0x80AC, 0xDDBD, 0, 1},
	{0x80AE, 0x5FD7, 0, 1},
	{0x80B0, 0x8E86, 0, 1},
	{0x80B2, 0x90BD, 0, 1},
	{0x80B4, 0x0330, 0, 1},

	{0x80B6, 0xDD90, 0, 1},
	{0x80B8, 0xCC92, 0, 1},
	{0x80BA, 0x02BD, 0, 1},
	{0x80BC, 0x0330, 0, 1},
	{0x80BE, 0xDD92, 0, 1},
	{0x80C0, 0xCC94, 0, 1},
	{0x80C2, 0x04BD, 0, 1},
	{0x80C4, 0x0330, 0, 1},
	{0x80C6, 0xDD94, 0, 1},
	{0x80C8, 0xCC96, 0, 1},
	{0x80CA, 0x00BD, 0, 1},
	{0x80CC, 0x0330, 0, 1},
	{0x80CE, 0xDD96, 0, 1},
	{0x80D0, 0xCC07, 0, 1},
	{0x80D2, 0xFFDD, 0, 1},
	{0x80D4, 0x8ECC, 0, 1},
	{0x80D6, 0x3180, 0, 1},
	{0x80D8, 0x30ED, 0, 1},
	{0x80DA, 0x02CC, 0, 1},
	{0x80DC, 0x008E, 0, 1},
	{0x80DE, 0xED00, 0, 1},
	{0x80E0, 0xC605, 0, 1},
	{0x80E2, 0xBDDE, 0, 1},
	{0x80E4, 0x13BD, 0, 1},
	{0x80E6, 0x03FA, 0, 1},
	{0x80E8, 0x3838, 0, 1},
	{0x80EA, 0x3913, 0, 1},
	{0x80EC, 0x0001, 0, 1},
	{0x80EE, 0x0109, 0, 1},
	{0x80F0, 0xBC01, 0, 1},
	{0x80F2, 0x9D26, 0, 1},
	{0x80F4, 0x0813, 0, 1},
	{0x80F6, 0x0004, 0, 1},
	{0x80F8, 0x0108, 0, 1},
	{0x80FA, 0xFF02, 0, 1},
	{0x80FC, 0xEF7E, 0, 1},
	{0x80FE, 0xC278, 0, 1},
	{0x8330, 0x364F, 0, 1},
	{0x8332, 0x36CE, 0, 1},
	{0x8334, 0x02F3, 0, 1},
	{0x8336, 0x3AEC, 0, 1},
	{0x8338, 0x00CE, 0, 1},
	{0x833A, 0x0018, 0, 1},

	{0x833C, 0xBDE4, 0, 1},
	{0x833E, 0x5197, 0, 1},
	{0x8340, 0x8F38, 0, 1},
	{0x8342, 0xEC00, 0, 1},
	{0x8344, 0x938E, 0, 1},
	{0x8346, 0x2C02, 0, 1},
	{0x8348, 0x4F5F, 0, 1},
	{0x834A, 0x3900, 0, 1},
	{0x83E4, 0x3C13, 0, 1},
	{0x83E6, 0x0001, 0, 1},
	{0x83E8, 0x0CCC, 0, 1},
	{0x83EA, 0x3180, 0, 1},
	{0x83EC, 0x30ED, 0, 1},
	{0x83EE, 0x00CC, 0, 1},
	{0x83F0, 0x87FF, 0, 1},
	{0x83F2, 0xBDDD, 0, 1},
	{0x83F4, 0xF5BD, 0, 1},
	{0x83F6, 0xC2A9, 0, 1},
	{0x83F8, 0x3839, 0, 1},
	{0x83FA, 0xFE02, 0, 1},
	{0x83FC, 0xEF7E, 0, 1},
	{0x83FE, 0x00EB, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x098A, 0x0000, 0, 1}, /* PHYSICAL_ADDRESS_ACCESS*/
	{0x83E0, 0x0098, 0, 1},
	{0x83E2, 0x03E4, 0, 1},
	{0x098E, 0x0000, 0, 1}, /* LOGICAL_ADDRESS_ACCESS*/
	{0x300A, 0x03BE, 0, 1}, /* FRAME_LENGTH_LINES*/
	{0x300C, 0x02D6, 0, 1}, /* LINE_LENGTH_PCK*/
	{0x3010, 0x0012, 0, 1}, /* FINE_CORRECTION*/
	{0x9803, 0x07, 0, 0},	/* STAT_FD_ZONE_HEIGHT*/
	{0xA06E, 0x0098, 0, 1}, /* CAM_FD_CONFIG_FDPERIOD_50HZ*/
	{0xA070, 0x007E, 0, 1}, /* CAM_FD_CONFIG_FDPERIOD_60HZ*/
	{0xA072, 0x11, 0, 0},	/* CAM_FD_CONFIG_SEARCH_F1_50*/
	{0xA073, 0x13, 0, 0},	/* CAM_FD_CONFIG_SEARCH_F2_50*/
	{0xA074, 0x14, 0, 0}, /* CAM_FD_CONFIG_SEARCH_F1_60*/
	{0xA075, 0x16, 0, 0}, /* CAM_FD_CONFIG_SEARCH_F2_60*/
	{0xA076, 0x0014, 0, 1}, /* CAM_FD_CONFIG_MAX_FDZONE_50HZ*/
	{0xA078, 0x0018, 0, 1}, /* CAM_FD_CONFIG_MAX_FDZONE_60HZ*/
	{0x3E22, 0x3307, 0, 1}, /* SAMP_BOOST_ROW*/
	{0x3ECE, 0x4311, 0, 1}, /* DAC_LD_2_3*/
	{0x3ED0, 0x16AF, 0, 1}, /* DAC_LD_4_5*/
	{0x3210, 0x00B0, 0, 1}, /* COLOR_PIPELINE_CONTROL*/

	{0x3640, 0x0170, 0, 1}, /* P_G1_P0Q0*/
	{0x3642, 0xD6AC, 0, 1}, /* P_G1_P0Q1*/
	{0x3644, 0x12B0, 0, 1}, /* P_G1_P0Q2*/
	{0x3646, 0xD5A9, 0, 1}, /* P_G1_P0Q3*/
	{0x3648, 0x9A10, 0, 1}, /* P_G1_P0Q4*/
	{0x364A, 0x01B0, 0, 1}, /* P_R_P0Q0*/
	{0x364C, 0xB0AC, 0, 1}, /* P_R_P0Q1*/
	{0x364E, 0x12F0, 0, 1}, /* P_R_P0Q2*/
	{0x3650, 0xA0CC, 0, 1}, /* P_R_P0Q3*/
	{0x3652, 0xDF4F, 0, 1}, /* P_R_P0Q4*/
	{0x3654, 0x0130, 0, 1}, /* P_B_P0Q0*/
	{0x3656, 0xA88C, 0, 1}, /* P_B_P0Q1*/
	{0x3658, 0x0610, 0, 1}, /* P_B_P0Q2*/
	{0x365A, 0xEEEC, 0, 1}, /* P_B_P0Q3*/
	{0x365C, 0x8850, 0, 1}, /* P_B_P0Q4*/
	{0x365E, 0x0270, 0, 1}, /* P_G2_P0Q0*/
	{0x3660, 0xC94C, 0, 1}, /* P_G2_P0Q1*/
	{0x3662, 0x1470, 0, 1}, /* P_G2_P0Q2*/
	{0x3664, 0x8A09, 0, 1}, /* P_G2_P0Q3*/
	{0x3666, 0x98F0, 0, 1}, /* P_G2_P0Q4*/
	{0x3680, 0x1EAB, 0, 1}, /* P_G1_P1Q0*/
	{0x3682, 0x528B, 0, 1}, /* P_G1_P1Q1*/
	{0x3684, 0x9CAC, 0, 1}, /* P_G1_P1Q2*/
	{0x3686, 0xD1ED, 0, 1}, /* P_G1_P1Q3*/
	{0x3688, 0x744C, 0, 1}, /* P_G1_P1Q4*/
	{0x368A, 0x7CEC, 0, 1}, /* P_R_P1Q0*/
	{0x368C, 0x1289, 0, 1}, /* P_R_P1Q1*/
	{0x368E, 0xBB4D, 0, 1}, /* P_R_P1Q2*/
	{0x3690, 0xF74D, 0, 1}, /* P_R_P1Q3*/
	{0x3692, 0xAB6D, 0, 1}, /* P_R_P1Q4*/
	{0x3694, 0x304C, 0, 1}, /* P_B_P1Q0*/
	{0x3696, 0x4FAB, 0, 1}, /* P_B_P1Q1*/
	{0x3698, 0xDB6D, 0, 1}, /* P_B_P1Q2*/
	{0x369A, 0x892E, 0, 1}, /* P_B_P1Q3*/
	{0x369C, 0x14EC, 0, 1}, /* P_B_P1Q4*/
	{0x369E, 0x360A, 0, 1}, /* P_G2_P1Q0*/
	{0x36A0, 0x050C, 0, 1}, /* P_G2_P1Q1*/
	{0x36A2, 0xB8CD, 0, 1}, /* P_G2_P1Q2*/
	{0x36A4, 0x9BCD, 0, 1}, /* P_G2_P1Q3*/
	{0x36A6, 0x0A4F, 0, 1}, /* P_G2_P1Q4*/
	{0x36C0, 0x764F, 0, 1}, /* P_G1_P2Q0*/
	{0x36C2, 0x202E, 0, 1}, /* P_G1_P2Q1*/
	{0x36C4, 0x82F1, 0, 1}, /* P_G1_P2Q2*/

	{0x36C6, 0xB7AF, 0, 1}, /* P_G1_P2Q3*/
	{0x36C8, 0x6390, 0, 1}, /* P_G1_P2Q4*/
	{0x36CA, 0x1CF0, 0, 1}, /* P_R_P2Q0 */
	{0x36CC, 0x8A4C, 0, 1}, /* P_R_P2Q1 */
	{0x36CE, 0x9311, 0, 1}, /* P_R_P2Q2 */
	{0x36D0, 0x8B4F, 0, 1}, /* P_R_P2Q3 */
	{0x36D2, 0x4DB0, 0, 1}, /* P_R_P2Q4 */
	{0x36D4, 0x0C10, 0, 1}, /* P_B_P2Q0 */
	{0x36D6, 0x7ECD, 0, 1}, /* P_B_P2Q1 */
	{0x36D8, 0xA191, 0, 1}, /* P_B_P2Q2 */
	{0x36DA, 0xCE2F, 0, 1}, /* P_B_P2Q3 */
	{0x36DC, 0x5A50, 0, 1}, /* P_B_P2Q4 */
	{0x36DE, 0x720F, 0, 1}, /* P_G2_P2Q0*/
	{0x36E0, 0x34EE, 0, 1}, /* P_G2_P2Q1*/
	{0x36E2, 0xDEB0, 0, 1}, /* P_G2_P2Q2*/
	{0x36E4, 0xD2AF, 0, 1}, /* P_G2_P2Q3*/
	{0x36E6, 0x4B8F, 0, 1}, /* P_G2_P2Q4*/
	{0x3700, 0xC0EF, 0, 1}, /* P_G1_P3Q0*/
	{0x3702, 0xED0E, 0, 1}, /* P_G1_P3Q1*/
	{0x3704, 0xA690, 0, 1}, /* P_G1_P3Q2*/
	{0x3706, 0x6ED0, 0, 1}, /* P_G1_P3Q3*/
	{0x3708, 0x58AE, 0, 1}, /* P_G1_P3Q4*/
	{0x370A, 0x8250, 0, 1}, /* P_R_P3Q0 */
	{0x370C, 0x65EE, 0, 1}, /* P_R_P3Q1 */
	{0x370E, 0xB76F, 0, 1}, /* P_R_P3Q2 */
	{0x3710, 0x682E, 0, 1}, /* P_R_P3Q3 */
	{0x3712, 0x1A11, 0, 1}, /* P_R_P3Q4 */
	{0x3714, 0x8C70, 0, 1}, /* P_B_P3Q0 */
	{0x3716, 0x6FCD, 0, 1}, /* P_B_P3Q1 */
	{0x3718, 0xD1AF, 0, 1}, /* P_B_P3Q2 */
	{0x371A, 0x190F, 0, 1}, /* P_B_P3Q3 */
	{0x371C, 0x6871, 0, 1}, /* P_B_P3Q4  */
	{0x371E, 0xBF2F, 0, 1}, /* P_G2_P3Q0 */
	{0x3720, 0xCC2E, 0, 1}, /* P_G2_P3Q1 */
	{0x3722, 0xC4EE, 0, 1}, /* P_G2_P3Q2 */
	{0x3724, 0x78CF, 0, 1}, /* P_G2_P3Q3 */
	{0x3726, 0xBAD1, 0, 1}, /* P_G2_P3Q4 */
	{0x3740, 0x89B1, 0, 1}, /* P_G1_P4Q0 */
	{0x3742, 0xA0D0, 0, 1}, /* P_G1_P4Q1 */
	{0x3744, 0xBE6D, 0, 1}, /* P_G1_P4Q2 */
	{0x3746, 0x6B11, 0, 1}, /* P_G1_P4Q3*/
	{0x3748, 0xD78E, 0, 1}, /* P_G1_P4Q4*/
	{0x374A, 0xA171, 0, 1}, /* P_R_P4Q0 */

	{0x374C, 0x56EF, 0, 1},        /* P_R_P4Q1*/
	{0x374E, 0x0CD0, 0, 1},		/*P_R_P4Q2*/
	{0x3750, 0x0CF0, 0, 1},        /* P_R_P4Q3*/
	{0x3752, 0x66D0, 0, 1},        /* P_R_P4Q4*/
	{0x3754, 0xB731, 0, 1}, /* P_B_P4Q0*/
	{0x3756, 0x9C4E, 0, 1},	/* P_B_P4Q1*/
	{0x3758, 0xED2C, 0, 1},	/* P_B_P4Q2*/
	{0x375A, 0x1E71, 0, 1},	/* P_B_P4Q3*/
	{0x375C, 0x1713, 0, 1}, /* P_B_P4Q4        */
	{0x375E, 0x8A11, 0, 1},	/* P_G2_P4Q0*/
	{0x3760, 0xA550, 0, 1},	/* P_G2_P4Q1*/
	{0x3762, 0xC20E, 0, 1},	/* P_G2_P4Q2*/
	{0x3764, 0x52F1, 0, 1}, /* P_G2_P4Q3*/
	{0x3766, 0x228F, 0, 1}, /* P_G2_P4Q4*/
	{0x3782, 0x012C, 0, 1}, /* CENTER_ROW*/
	{0x3784, 0x0160, 0, 1}, /* CENTER_COLUMN*/
	{0x3210, 0x00B8, 0, 1}, /* COLOR_PIPELINE_CONTROL*/
	{0xA02F, 0x0259, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_0*/
	{0xA031, 0xFF65, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_1*/
	{0xA033, 0xFF5F, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_2*/
	{0xA035, 0xFFD8, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_3*/
	{0xA037, 0x00E1, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_4*/
	{0xA039, 0x0064, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_5*/
	{0xA03B, 0xFF5B, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_6*/
	{0xA03D, 0xFE72, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_7*/
	{0xA03F, 0x0351, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_8*/
	{0xA041, 0x0021, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_9*/
	{0xA043, 0x004A, 0, 1},	/* CAM_AWB_CONFIG_CCM_L_10*/
	{0xA045, 0x0000, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_0*/
	{0xA047, 0xFFAB, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_1*/
	{0xA049, 0x0055, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_2*/
	{0xA04B, 0x000C, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_3*/
	{0xA04D, 0x001F, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_4*/
	{0xA04F, 0xFFD5, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_5*/
	{0xA051, 0x0097, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_6*/
	{0xA053, 0x00AF, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_7*/
	{0xA055, 0xFEB9, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_8*/
	{0xA057, 0x0010, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_9*/
	{0xA059, 0xFFDD, 0, 1},	/* CAM_AWB_CONFIG_CCM_RL_10*/
	{0x940A, 0x0000, 0, 1},	/* AWB_X_START            */
	{0x940C, 0x0000, 0, 1}, /* AWB_Y_START                    */
	{0x940E, 0x027F, 0, 1}, /* AWB_X_END                      */
	{0xA061, 0x002A, 0, 1},	/* CAM_AWB_CONFIG_X_SHIFT_PRE_ADJ*/

	{0xA063, 0x0038, 0, 1}, /* CAM_AWB_CONFIG_Y_SHIFT_PRE_ADJ */
	{0xA065, 0x04, 0, 0}, /* CAM_AWB_CONFIG_X_SCALE */
	{0xA066, 0x02, 0, 0}, /* CAM_AWB_CONFIG_Y_SCALE */
	{0x9409, 0xF0, 0, 0}, /* AWB_LUMA_THRESH_HIGH   */
	{0x9416, 0x2D, 0, 0}, /* AWB_R_SCENE_RATIO_LOWER*/
	{0x9417, 0x8C, 0, 0}, /* AWB_R_SCENE_RATIO_UPPER*/
	{0x9418, 0x16, 0, 0}, /* AWB_B_SCENE_RATIO_LOWER*/
	{0x9419, 0x78, 0, 0}, /* AWB_B_SCENE_RATIO_UPPER*/
	{0x2112, 0x0000, 0, 1}, /* AWB_WEIGHT_R0        */
	{0x2114, 0x0000, 0, 1}, /* AWB_WEIGHT_R1        */
	{0x2116, 0x0000, 0, 1}, /* AWB_WEIGHT_R2        */
	{0x2118, 0x0F80, 0, 1}, /* AWB_WEIGHT_R3*/
	{0x211A, 0x2A80, 0, 1}, /* AWB_WEIGHT_R4*/
	{0x211C, 0x0B40, 0, 1}, /* AWB_WEIGHT_R5*/
	{0x211E, 0x01AC, 0, 1}, /* AWB_WEIGHT_R6*/
	{0x2120, 0x0038, 0, 1}, /* AWB_WEIGHT_R7*/
	{0x326E, 0x0006, 0, 1}, /* LOW_PASS_YUV_FILTER*/
	{0x33F4, 0x000B, 0, 1}, /* KERNEL_CONFIG*/
	{0xA087, 0x00, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_0*/
	{0xA088, 0x07, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_1*/
	{0xA089, 0x16, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_2*/
	{0xA08A, 0x30, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_3*/
	{0xA08B, 0x52, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_4*/
	{0xA08C, 0x6D, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_5*/
	{0xA08D, 0x86, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_6*/
	{0xA08E, 0x9B, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_7*/
	{0xA08F, 0xAB, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_8*/
	{0xA090, 0xB9, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_9*/
	{0xA091, 0xC5, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_10*/
	{0xA092, 0xCF, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_11*/
	{0xA093, 0xD8, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_12*/
	{0xA094, 0xE0, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_13*/
	{0xA095, 0xE7, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_14*/
	{0xA096, 0xEE, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_15*/
	{0xA097, 0xF4, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_16*/
	{0xA098, 0xFA, 0, 0}, /* CAM_LL_CONFIG_GAMMA_CONTRAST_CURVE_17*/
	{0xA0AD, 0x0001, 0, 1}, /* CAM_LL_CONFIG_GAMMA_START_BM       */
	{0xA0AF, 0x0338, 0, 1}, /* CAM_LL_CONFIG_GAMMA_STOP_BM        */
	{0xA0B1, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_RED_START*/
	{0xA0B2, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_RED_STOP*/
	{0xA0B3, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_GREEN_START*/
	{0xA0B4, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_GREEN_STOP */
	{0xA0B5, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_BLUE_START */

	{0xA0B6, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_BLUE_STOP  */
	{0xA0B7, 0x10, 0, 0}, /* CAM_LL_CONFIG_NR_MIN_MAX_START*/
	{0xA0B8, 0x2D, 0, 0}, /* CAM_LL_CONFIG_NR_MIN_MAX_STOP*/
	{0xA0B9, 0x0040, 0, 1}, /* CAM_LL_CONFIG_START_GAIN_METRIC*/
	{0xA0BB, 0x00C8, 0, 1}, /* CAM_LL_CONFIG_STOP_GAIN_METRIC*/
	{0xA07A, 0x04, 0, 0}, /* CAM_LL_CONFIG_AP_THRESH_START*/
	{0xA07B, 0x0F, 0, 0}, /* CAM_LL_CONFIG_AP_THRESH_STOP*/
	{0xA07C, 0x03, 0, 0}, /* CAM_LL_CONFIG_AP_GAIN_START*/
	{0xA07D, 0x00, 0, 0}, /* CAM_LL_CONFIG_AP_GAIN_STOP*/
	{0xA07E, 0x0078, 0, 1}, /* CAM_LL_CONFIG_CDC_THRESHOLD_BM*/
	{0xA080, 0x05, 0, 0}, /* CAM_LL_CONFIG_CDC_GATE_PERCENTAGE*/
	{0xA081, 0x28, 0, 0}, /* CAM_LL_CONFIG_DM_EDGE_TH_START*/
	{0xA082, 0x32, 0, 0}, /* CAM_LL_CONFIG_DM_EDGE_TH_STOP*/
	{0xA083, 0x000F, 0, 1}, /* CAM_LL_CONFIG_FTB_AVG_YSUM_START*/
	{0xA085, 0x0000, 0, 1}, /* CAM_LL_CONFIG_FTB_AVG_YSUM_STOP */
	{0xA020, 0x4B, 0, 0}, /* CAM_AE_CONFIG_BASE_TARGET         */
	{0xA027, 0x0050, 0, 1}, /* CAM_AE_CONFIG_MIN_VIRT_AGAIN    */
	{0xA029, 0x0140, 0, 1}, /* CAM_AE_CONFIG_MAX_VIRT_AGAIN    */
	{0xA025, 0x0080, 0, 1}, /* CAM_AE_CONFIG_MAX_VIRT_DGAIN    */
	{0xA01C, 0x00C8, 0, 1}, /* CAM_AE_CONFIG_TARGET_AGAIN      */
	{0xA01E, 0x0080, 0, 1}, /* CAM_AE_CONFIG_TARGET_DGAIN      */
	{0xA01A, 0x000B, 0, 1}, /* CAM_AE_CONFIG_TARGET_FDZONE     */
	{0xA05F, 0xA0, 0, 0}, /* CAM_AWB_CONFIG_START_SATURATION   */
	{0xA060, 0x28, 0, 0}, /* CAM_AWB_CONFIG_END_SATURATION*/
	{0xA05B, 0x0005, 0, 1}, /* CAM_AWB_CONFIG_START_BRIGHTNESS_BM*/
	{0xA05D, 0x0023, 0, 1}, /* CAM_AWB_CONFIG_STOP_BRIGHTNESS_BM */
	{0x9801, 0x000F, 0, 1}, /* STAT_CLIP_MAX                     */
	{0x9C02, 0x02, 0, 0}, /* LL_GAMMA_SELECT                     */
	{0x9001, 0x05, 0, 0}, /* AE_TRACK_MODE                       */
	{0x9007, 0x05, 0, 0}, /* AE_TRACK_TARGET_GATE                */
	{0x9003, 0x2D, 0, 0}, /* AE_TRACK_BLACK_LEVEL_MAX            */
	{0x9004, 0x02, 0, 0}, /* AE_TRACK_BLACK_LEVEL_STEP_SIZE      */
	{0x9005, 0x23, 0, 0}, /* AE_TRACK_BLACK_CLIP_PERCENT         */
	{0x8C03, 0x01, 0, 0}, /* FD_STAT_MIN                         */
	{0x8C04, 0x03, 0, 0}, /* FD_STAT_MAX*/
	{0x8C05, 0x05, 0, 0}, /* FD_MIN_AMPLITUDE*/
	{0x0018, 0x0002, 0, 1}, /* STANDBY_CONTROL_AND_STATUS*/

	/* amend the deviation of the color*/
	{0x098E, 0x8400 , 0, 1}, /* LOGICAL_ADDRESS_ACCESS [SEQ_CMD]*/
	{0x8400, 0x01, 0, 0}, /* SEQ_CMD                            */
	{0xA02F, 0x018A, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_0           */
	{0xA045, 0x0071, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_0          */
	{0xA031, 0xFFA9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_1           */
	{0xA047, 0xFF87, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_1          */
	{0xA033, 0xFFC9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_2           */
	{0xA049, 0x0040, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_2          */
	{0xA035, 0xFF03, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_3           */
	{0xA04B, 0x0093, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_3          */
	{0xA037, 0x0185, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_4*/
	{0xA04D, 0xFFED, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_4*/
	{0xA039, 0x007D, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_5 */
	{0xA04F, 0xFF76, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_5*/
	{0xA03B, 0xFF79, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_6 */
	{0xA051, 0x008B, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_6*/
	{0xA03D, 0xFEC9, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_7 */
	{0xA053, 0x0090, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_7*/
	{0xA03F, 0x02C2, 0, 1}, /* CAM_AWB_CONFIG_CCM_L_8 */
	{0xA055, 0xFF0E, 0, 1}, /* CAM_AWB_CONFIG_CCM_RL_8*/
	{0x8400, 0x02, 0, 0}, /* SEQ_CMD                  */

	/*improve the brightness*/
	{0x098E, 0x0808, 0, 1},	/* LOGICAL_ADDRESS_ACCESS [INT_COARSE_INTEGRATION_TIME]*/
	{0x8808, 0x0482, 0, 1},	/* INT_COARSE_INTEGRATION_TIME*/

	/*set to UYVY*/
	{0x098E, 0x8400, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_CMD]	//tried
	{0x8400, 0x01  , 0, 0}, // SEQ_CMD
	{0x332E, 0x0000, 0, 1}, 	// OUTPUT_FORMAT_CONFIGURATION
	{0x3330, 0x0000, 0, 1}, 	// OUTPUT_FORMAT_TEST
	{0x3C00, 0x1002, 0, 1}, 	// TX_CONTROL
	{0x8400, 0x02  , 0, 0},// SEQ_CMD
};

static struct mt9v114_mode_info mt9v114_mode_info_data[2][mt9v114_mode_MAX + 1] = {
	{
		{mt9v114_mode_VGA_640_480, 640, 480, mt9v114_setting_15fps_VGA_640_480, ARRAY_SIZE(mt9v114_setting_15fps_VGA_640_480)},
		{mt9v114_mode_CIF_352_288, 352, 288, NULL, 0},
	},
	{
		{mt9v114_mode_VGA_640_480, 640, 480, NULL, 0},
		{mt9v114_mode_CIF_352_288, 352, 288, mt9v114_setting_30fps_CIF_352_288, ARRAY_SIZE(mt9v114_setting_30fps_CIF_352_288)},
	},
};

static struct regulator *io_regulator;
static struct regulator *core_regulator;
static struct regulator *analog_regulator;
static struct regulator *gpo_regulator;
static struct mxc_camera_platform_data *camera_plat;

static int mt9v114_probe(struct i2c_client *adapter,
				const struct i2c_device_id *device_id);
static int mt9v114_remove(struct i2c_client *client);

static s32 mt9v114_read_reg(u16 reg, u16 *val, bool double_bytes);
static s32 mt9v114_write_reg(u16 reg, u16 val, bool double_bytes);

static const struct i2c_device_id mt9v114_id[] = {
	{"mt9v114", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mt9v114_id);

static struct i2c_driver mt9v114_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = "mt9v114",
		  },
	.probe  = mt9v114_probe,
	.remove = mt9v114_remove,
	.id_table = mt9v114_id,
};

extern void gpio_sensor_active(unsigned int csi_index);
extern void gpio_sensor_inactive(unsigned int csi);

static int mt9v114_set_white_balance(enum white_balance_mode  balance_mode)
{
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u16 Val = 0;
	int retval = 0;

	if (balance_mode > white_balance_MAX || balance_mode < white_balance_MIN) {
		pr_err("Wrong white balance_mode detected!\n");
		return -1;
	}
	pModeSetting = mt9v114_white_balance_mode_info_data[balance_mode].init_data_ptr;
	iModeSettingArySize = mt9v114_white_balance_mode_info_data[balance_mode].init_data_size;

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = pModeSetting->u32Delay_ms;
		RegAddr = pModeSetting->u16RegAddr;
		Val = pModeSetting->u16Val;
		retval = mt9v114_write_reg(RegAddr, Val, pModeSetting->double_bytes);
		if (retval < 0)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
err:
	return retval;

}

static int mt9v114_set_special_effect(enum special_effect_mode  effect_mode)
{
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u16 Val = 0;
	int retval = 0;

	if (effect_mode > special_effect_MAX || effect_mode < special_effect_MIN) {
		pr_err("Wrong white balance_mode detected!\n");
		return -1;
	}
	pModeSetting = mt9v114_special_effect_info_data[effect_mode].init_data_ptr;
	iModeSettingArySize = mt9v114_special_effect_info_data[effect_mode].init_data_size;

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = pModeSetting->u32Delay_ms;
		RegAddr = pModeSetting->u16RegAddr;
		Val = pModeSetting->u16Val;
		retval = mt9v114_write_reg(RegAddr, Val, pModeSetting->double_bytes);
		if (retval < 0)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
err:
	return retval;

}

static s32 mt9v114_read_reg(u16 reg, u16 *val, bool double_bytes)
{
	u8 au8RegBuf[2] = {0};
	u8 u8RdVal[2];

	au8RegBuf[0] = reg >> 8;
	au8RegBuf[1] = reg & 0xff;

	if (2 != i2c_master_send(mt9v114_data.i2c_client, au8RegBuf, 2)) {
		pr_err("%s:write reg error:reg=%x\n",
				__func__, reg);
		return -1;
	}
	if (double_bytes) {
		if (2 != i2c_master_recv(mt9v114_data.i2c_client, u8RdVal, 2)) {
			pr_err("%s:read reg error:reg=%x,val=%x\n",
					__func__, reg, u8RdVal);
			return -1;
		}

		*val = ((u16)u8RdVal[1]) | ((u16)u8RdVal[0] << 8);
	} else {
		if (1 != i2c_master_recv(mt9v114_data.i2c_client, u8RdVal, 1)) {
			pr_err("%s:read reg error:reg=%x,val=%x\n",
					__func__, reg, u8RdVal);
			return -1;
		}

		*val = (u16)u8RdVal[0];
	}

	return *val;
}

static s32 mt9v114_write_reg(u16 reg, u16 val, bool double_bytes)
{
	u8 au8Buf[4] = {0};

	if (double_bytes) {
		au8Buf[0] = reg >> 8;
		au8Buf[1] = reg & 0xff;
		au8Buf[2] = val >> 8;
		au8Buf[3] = val & 0xff;

		if (i2c_master_send(mt9v114_data.i2c_client, au8Buf, 4)
			< 0) {
			pr_err("%s:write reg error:reg=%x,val=%x\n",
				__func__, reg, val);
			return -1;
		}
	} else {
		au8Buf[0] = reg >> 8;
		au8Buf[1] = reg & 0xff;
		au8Buf[2] = (u8)val;

		if (i2c_master_send(mt9v114_data.i2c_client, au8Buf, 3)
			< 0) {
			pr_err("%s:write reg error:reg=%x,val=%x\n",
				__func__, reg, val);
			return -1;
		}
	}

	return 0;
}

static int mt9v114_init_mode(enum mt9v114_frame_rate frame_rate,
			    enum mt9v114_mode mode)
{
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u16 Val = 0;
	int retval = 0;

	if (mode > mt9v114_mode_MAX || mode < mt9v114_mode_MIN) {
		pr_err("Wrong mt9v114 mode detected!\n");
		return -1;
	}

	pModeSetting = mt9v114_mode_info_data[frame_rate][mode].init_data_ptr;
	iModeSettingArySize =
		mt9v114_mode_info_data[frame_rate][mode].init_data_size;

	mt9v114_data.pix.width = mt9v114_mode_info_data[frame_rate][mode].width;
	mt9v114_data.pix.height = mt9v114_mode_info_data[frame_rate][mode].height;

	if (NULL == pModeSetting)
		return -EINVAL;
	if (mt9v114_data.pix.width == 0 || mt9v114_data.pix.height == 0)
		return -EINVAL;

	/* reset camera */
	mt9v114_write_reg(0x001a, 0x0326, true);
	msleep(5);

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = pModeSetting->u32Delay_ms;
		RegAddr = pModeSetting->u16RegAddr;
		Val = pModeSetting->u16Val;

		retval = mt9v114_write_reg(RegAddr, Val, pModeSetting->double_bytes);
		if (retval < 0)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
err:
	return retval;
}

/* --------------- IOCTL functions from v4l2_int_ioctl_desc --------------- */

static int ioctl_g_ifparm(struct v4l2_int_device *s, struct v4l2_ifparm *p)
{
	if (s == NULL) {
		pr_err("   ERROR!! no slave device set!\n");
		return -1;
	}

	memset(p, 0, sizeof(*p));
	p->u.bt656.clock_curr = mt9v114_data.mclk;
	pr_debug("   clock_curr=mclk=%d\n", mt9v114_data.mclk);
	p->if_type = V4L2_IF_TYPE_BT656;
	p->u.bt656.mode = V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT;
	p->u.bt656.clock_min = MT9V114_XCLK_MIN;
	p->u.bt656.clock_max = MT9V114_XCLK_MAX;
	p->u.bt656.bt_sync_correct = 1;  /* Indicate external vsync */
	p->u.bt656.latch_clk_inv = 1;
	p->u.bt656.nobt_vs_inv = 1;
	p->u.bt656.nobt_hs_inv = 0;

	return 0;
}

/*!
 * ioctl_s_power - V4L2 sensor interface handler for VIDIOC_S_POWER ioctl
 * @s: pointer to standard V4L2 device structure
 * @on: indicates power mode (on or off)
 *
 * Turns the power on or off, depending on the value of on and returns the
 * appropriate error code.
 */
static int ioctl_s_power(struct v4l2_int_device *s, int on)
{
	struct sensor *sensor = s->priv;

	if (on && !sensor->on) {
		gpio_sensor_active(mt9v114_data.csi);
		if (io_regulator)
			if (regulator_enable(io_regulator) != 0)
				return -EIO;
		if (core_regulator)
			if (regulator_enable(core_regulator) != 0)
				return -EIO;
		if (gpo_regulator)
			if (regulator_enable(gpo_regulator) != 0)
				return -EIO;
		if (analog_regulator)
			if (regulator_enable(analog_regulator) != 0)
				return -EIO;
		/* Make sure power on */
		if (camera_plat->pwdn)
			camera_plat->pwdn(0);

	} else if (!on && sensor->on) {
		if (analog_regulator)
			regulator_disable(analog_regulator);
		if (core_regulator)
			regulator_disable(core_regulator);
		if (io_regulator)
			regulator_disable(io_regulator);
		if (gpo_regulator)
			regulator_disable(gpo_regulator);
		gpio_sensor_inactive(mt9v114_data.csi);
		/* Make sure power down*/
		if (camera_plat->pwdn)
			camera_plat->pwdn(1);
	}

	sensor->on = on;

	return 0;
}

/*!
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct sensor *sensor = s->priv;
	struct v4l2_captureparm *cparm = &a->parm.capture;
	int ret = 0;

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		memset(a, 0, sizeof(*a));
		a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		cparm->capability = sensor->streamcap.capability;
		cparm->timeperframe = sensor->streamcap.timeperframe;
		cparm->capturemode = sensor->streamcap.capturemode;
		ret = 0;
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*!
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */
static int ioctl_s_parm(struct v4l2_int_device *s, struct v4l2_streamparm *a)
{
	struct sensor *sensor = s->priv;
	struct v4l2_fract *timeperframe = &a->parm.capture.timeperframe;
	u32 tgt_fps;	/* target frames per secound */
	enum mt9v114_frame_rate frame_rate;
	int ret = 0;

	/* Make sure power on */
	if (camera_plat->pwdn)
		camera_plat->pwdn(0);

	switch (a->type) {
	/* This is the only case currently handled. */
	case V4L2_BUF_TYPE_VIDEO_CAPTURE:
		/* Check that the new frame rate is allowed. */
		if ((timeperframe->numerator == 0) ||
		    (timeperframe->denominator == 0)) {
			timeperframe->denominator = DEFAULT_FPS;
			timeperframe->numerator = 1;
		}

		tgt_fps = timeperframe->denominator /
			  timeperframe->numerator;

		if (tgt_fps > MAX_FPS) {
			timeperframe->denominator = MAX_FPS;
			timeperframe->numerator = 1;
		} else if (tgt_fps < MIN_FPS) {
			timeperframe->denominator = MIN_FPS;
			timeperframe->numerator = 1;
		}

		/* Actual frame rate we use */
		tgt_fps = timeperframe->denominator /
			  timeperframe->numerator;

		if (tgt_fps == 15)
			frame_rate = mt9v114_15_fps;
		else if (tgt_fps == 30)
			frame_rate = mt9v114_30_fps;
		else {
			pr_err(" The camera frame rate is not supported!\n");
			printk(KERN_INFO "frame is not 30\n");
			return -EINVAL;
		}

		sensor->streamcap.timeperframe = *timeperframe;
		sensor->streamcap.capturemode =
				(u32)a->parm.capture.capturemode;

		ret = mt9v114_init_mode(frame_rate,
				       sensor->streamcap.capturemode);
		break;

	/* These are all the possible cases. */
	case V4L2_BUF_TYPE_VIDEO_OUTPUT:
	case V4L2_BUF_TYPE_VIDEO_OVERLAY:
	case V4L2_BUF_TYPE_VBI_CAPTURE:
	case V4L2_BUF_TYPE_VBI_OUTPUT:
	case V4L2_BUF_TYPE_SLICED_VBI_CAPTURE:
	case V4L2_BUF_TYPE_SLICED_VBI_OUTPUT:
		pr_debug("   type is not " \
			"V4L2_BUF_TYPE_VIDEO_CAPTURE but %d\n",
			a->type);
			printk(KERN_INFO "those setting not support\n");
		ret = -EINVAL;
		break;

	default:
		pr_debug("   type is unknown - %d\n", a->type);
		printk(KERN_INFO "unknown setting \n");
		ret = -EINVAL;
		break;
	}

	return ret;
}

/*!
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct sensor *sensor = s->priv;

	f->fmt.pix = sensor->pix;

	return 0;
}

/*!
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the video_control[] array.  Otherwise, returns -EINVAL
 * if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int ret = 0;

	switch (vc->id) {
	case V4L2_CID_BRIGHTNESS:
		vc->value = mt9v114_data.brightness;
		break;
	case V4L2_CID_HUE:
		vc->value = mt9v114_data.hue;
		break;
	case V4L2_CID_CONTRAST:
		vc->value = mt9v114_data.contrast;
		break;
	case V4L2_CID_SATURATION:
		vc->value = mt9v114_data.saturation;
		break;
	case V4L2_CID_RED_BALANCE:
		vc->value = mt9v114_data.red;
		break;
	case V4L2_CID_BLUE_BALANCE:
		vc->value = mt9v114_data.blue;
		break;
	case V4L2_CID_EXPOSURE:
		vc->value = mt9v114_data.ae_mode;
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

/*!
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the video_control[] array).  Otherwise,
 * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *vc)
{
	int retval = 0;

	pr_debug("In mt9v114:ioctl_s_ctrl %d\n",
		 vc->id);

	switch (vc->id) {
	case V4L2_CID_BRIGHTNESS:
		break;
	case V4L2_CID_CONTRAST:
		break;
	case V4L2_CID_SATURATION:
		break;
	case V4L2_CID_HUE:
		break;
	case V4L2_CID_AUTO_WHITE_BALANCE:
		break;
	case V4L2_CID_DO_WHITE_BALANCE:
		switch (vc->value) {
		case V4L2_MXC_WHITE_BALANCE_AUTO:
			retval = mt9v114_set_white_balance(white_balance_auto);
			break;
		case V4L2_MXC_WHITE_BALANCE_CLOUDY1:
			retval = mt9v114_set_white_balance(white_balance_cloudy1);
			break;
		case V4L2_MXC_WHITE_BALANCE_SUNSHINE:
			retval = mt9v114_set_white_balance(white_balance_sunshine);
			break;
		case V4L2_MXC_WHITE_BALANCE_CLOUDY2:
			retval = mt9v114_set_white_balance(white_balance_cloudy2);
			break;
		case V4L2_MXC_WHITE_BALANCE_DAYLIGHT:
			retval = mt9v114_set_white_balance(white_balance_daylight);
			break;
		case V4L2_MXC_WHITE_BALANCE_FILAMENTLIGHT:
			retval = mt9v114_set_white_balance(white_balance_filament);
			break;
		default:
			retval = -EPERM;
			break;
		}
		break;
	case V4L2_CID_RED_BALANCE:
		break;
	case V4L2_CID_BLUE_BALANCE:
		break;
	case V4L2_CID_GAMMA:
		break;
	case V4L2_CID_EXPOSURE:
		break;
	case V4L2_CID_AUTOGAIN:
		break;
	case V4L2_CID_GAIN:
		break;
	case V4L2_CID_HFLIP:
		break;
	case V4L2_CID_VFLIP:
		break;
	case V4L2_CID_SPECIAL_EFFECT:
		switch (vc->value) {
		case V4L2_MXC_SPECIAL_EFFECT_NORMAL:
			retval = mt9v114_set_special_effect(special_effect_effect_off);
			break;
		case V4L2_MXC_SPECIAL_EFFECT_NEGTIVE:
			retval = mt9v114_set_special_effect(special_effect_negtive);
			break;
		case V4L2_MXC_SPECIAL_EFFECT_BW:
			retval = mt9v114_set_special_effect(special_effect_WB);
			break;
		default:
			retval = -EPERM;
			break;
		}
		break;
	default:
		retval = -EPERM;
		break;
	}

	return retval;
}

/*!
 * ioctl_enum_framesizes - V4L2 sensor interface handler for
 *			   VIDIOC_ENUM_FRAMESIZES ioctl
 * @s: pointer to standard V4L2 device structure
 * @fsize: standard V4L2 VIDIOC_ENUM_FRAMESIZES ioctl structure
 *
 * Return 0 if successful, otherwise -EINVAL.
 */
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
				 struct v4l2_frmsizeenum *fsize)
{
	if (fsize->index > mt9v114_mode_MAX)
		return -EINVAL;

	fsize->pixel_format = mt9v114_data.pix.pixelformat;
	fsize->discrete.width =
			mt9v114_mode_info_data[0][fsize->index].width;
	fsize->discrete.height =
			mt9v114_mode_info_data[0][fsize->index].height;
	return 0;
}

/*!
 * ioctl_g_chip_ident - V4L2 sensor interface handler for
 *			VIDIOC_DBG_G_CHIP_IDENT ioctl
 * @s: pointer to standard V4L2 device structure
 * @id: pointer to int
 *
 * Return 0.
 */
static int ioctl_g_chip_ident(struct v4l2_int_device *s, int *id)
{
	((struct v4l2_dbg_chip_ident *)id)->match.type =
					V4L2_CHIP_MATCH_I2C_DRIVER;
	strcpy(((struct v4l2_dbg_chip_ident *)id)->match.name, "mt9v114_camera");

	return 0;
}

/*!
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 */
static int ioctl_init(struct v4l2_int_device *s)
{

	return 0;
}

/*!
 * ioctl_enum_fmt_cap - V4L2 sensor interface handler for VIDIOC_ENUM_FMT
 * @s: pointer to standard V4L2 device structure
 * @fmt: pointer to standard V4L2 fmt description structure
 *
 * Return 0.
 */
static int ioctl_enum_fmt_cap(struct v4l2_int_device *s,
			      struct v4l2_fmtdesc *fmt)
{
	if (fmt->index > mt9v114_mode_MAX)
		return -EINVAL;

	fmt->pixelformat = mt9v114_data.pix.pixelformat;

	return 0;
}

/*!
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	struct sensor *sensor = s->priv;
	u32 tgt_xclk;	/* target xclk */
	u32 tgt_fps;	/* target frames per secound */
	enum mt9v114_frame_rate frame_rate;
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u16 Val = 0;
	int retval = 0;

	gpio_sensor_active(mt9v114_data.csi);
	mt9v114_data.on = true;

	/* mclk */
	tgt_xclk = mt9v114_data.mclk;
	tgt_xclk = min(tgt_xclk, (u32)MT9V114_XCLK_MAX);
	tgt_xclk = max(tgt_xclk, (u32)MT9V114_XCLK_MIN);
	mt9v114_data.mclk = tgt_xclk;

	pr_debug("   Setting mclk to %d MHz\n", tgt_xclk / 1000000);
	set_mclk_rate(&mt9v114_data.mclk, mt9v114_data.csi);

	/* Default camera frame rate is set in probe */
	tgt_fps = sensor->streamcap.timeperframe.denominator /
		  sensor->streamcap.timeperframe.numerator;

	if (tgt_fps == 15)
		frame_rate = mt9v114_15_fps;
	else if (tgt_fps == 30)
		frame_rate = mt9v114_30_fps;
	else
		return -EINVAL; /* Only support 15fps or 30fps now. */

	pModeSetting = mt9v114_init_setting;
	iModeSettingArySize = ARRAY_SIZE(mt9v114_init_setting);

	/* reset camera */
	mt9v114_write_reg(0x001a, 0x0326, true);
	msleep(5);

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = pModeSetting->u32Delay_ms;
		RegAddr = pModeSetting->u16RegAddr;
		Val = pModeSetting->u16Val;

		retval = mt9v114_write_reg(RegAddr, Val, pModeSetting->double_bytes);
		if (retval < 0)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
err:
	return retval;
}

/*!
 * ioctl_dev_exit - V4L2 sensor interface handler for vidioc_int_dev_exit_num
 * @s: pointer to standard V4L2 device structure
 *
 * Delinitialise the device when slave detaches to the master.
 */
static int ioctl_dev_exit(struct v4l2_int_device *s)
{
	gpio_sensor_inactive(mt9v114_data.csi);

	return 0;
}

/*!
 * This structure defines all the ioctls for this module and links them to the
 * enumeration.
 */
static struct v4l2_int_ioctl_desc mt9v114_ioctl_desc[] = {
	{vidioc_int_dev_init_num,
				(v4l2_int_ioctl_func *)ioctl_dev_init},
	{vidioc_int_dev_exit_num, ioctl_dev_exit},
	{vidioc_int_s_power_num,
				(v4l2_int_ioctl_func *)ioctl_s_power},
	{vidioc_int_g_ifparm_num,
				(v4l2_int_ioctl_func *)ioctl_g_ifparm},
/*	{vidioc_int_g_needs_reset_num,
				(v4l2_int_ioctl_func *)ioctl_g_needs_reset}, */
/*	{vidioc_int_reset_num, (v4l2_int_ioctl_func *)ioctl_reset}, */
	{vidioc_int_init_num, (v4l2_int_ioctl_func *)ioctl_init},
	{vidioc_int_enum_fmt_cap_num,
				(v4l2_int_ioctl_func *)ioctl_enum_fmt_cap},
/*	{vidioc_int_try_fmt_cap_num,
				(v4l2_int_ioctl_func *)ioctl_try_fmt_cap}, */
	{vidioc_int_g_fmt_cap_num, (v4l2_int_ioctl_func *)ioctl_g_fmt_cap},
/*	{vidioc_int_s_fmt_cap_num, (v4l2_int_ioctl_func *)ioctl_s_fmt_cap}, */
	{vidioc_int_g_parm_num, (v4l2_int_ioctl_func *)ioctl_g_parm},
	{vidioc_int_s_parm_num, (v4l2_int_ioctl_func *)ioctl_s_parm},
/*	{vidioc_int_queryctrl_num, (v4l2_int_ioctl_func *)ioctl_queryctrl}, */
	{vidioc_int_g_ctrl_num, (v4l2_int_ioctl_func *)ioctl_g_ctrl},
	{vidioc_int_s_ctrl_num, (v4l2_int_ioctl_func *)ioctl_s_ctrl},
	{vidioc_int_enum_framesizes_num,
				(v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{vidioc_int_g_chip_ident_num,
				(v4l2_int_ioctl_func *)ioctl_g_chip_ident},
};

static struct v4l2_int_slave mt9v114_slave = {
	.ioctls = mt9v114_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(mt9v114_ioctl_desc),
};

static struct v4l2_int_device mt9v114_int_device = {
	.module = THIS_MODULE,
	.name = "mt9v114",
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &mt9v114_slave,
	},
};

/*!
 * mt9v114 I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int mt9v114_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int retval;
	struct mxc_camera_platform_data *plat_data = client->dev.platform_data;

	/* Set initial values for the sensor struct. */
	memset(&mt9v114_data, 0, sizeof(mt9v114_data));
	mt9v114_data.mclk = 24000000; /* 6 - 54 MHz, typical 24MHz */
	mt9v114_data.mclk = plat_data->mclk;
	mt9v114_data.csi = plat_data->csi;

	mt9v114_data.i2c_client = client;
	mt9v114_data.pix.pixelformat = IPU_PIX_FMT_UYVY;
	mt9v114_data.pix.width = 640;
	mt9v114_data.pix.height = 480;
	mt9v114_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
					   V4L2_CAP_TIMEPERFRAME;
	mt9v114_data.streamcap.capturemode = 0;
	mt9v114_data.streamcap.timeperframe.denominator = DEFAULT_FPS;
	mt9v114_data.streamcap.timeperframe.numerator = 1;

	if (plat_data->io_regulator) {
		io_regulator = regulator_get(&client->dev,
					     plat_data->io_regulator);
		if (!IS_ERR(io_regulator)) {
			regulator_set_voltage(io_regulator,
					      MT9V114_VOLTAGE_DIGITAL_IO,
					      MT9V114_VOLTAGE_DIGITAL_IO);
			if (regulator_enable(io_regulator) != 0) {
				pr_err("%s:io set voltage error\n", __func__);
				goto err1;
			} else {
				dev_dbg(&client->dev,
					"%s:io set voltage ok\n", __func__);
			}
		} else
			io_regulator = NULL;
	}

	if (plat_data->core_regulator) {
		core_regulator = regulator_get(&client->dev,
					       plat_data->core_regulator);
		if (!IS_ERR(core_regulator)) {
			regulator_set_voltage(core_regulator,
					      MT9V114_VOLTAGE_DIGITAL_CORE,
					      MT9V114_VOLTAGE_DIGITAL_CORE);
			if (regulator_enable(core_regulator) != 0) {
				pr_err("%s:core set voltage error\n", __func__);
				goto err2;
			} else {
				dev_dbg(&client->dev,
					"%s:core set voltage ok\n", __func__);
			}
		} else
			core_regulator = NULL;
	}

	if (plat_data->analog_regulator) {
		analog_regulator = regulator_get(&client->dev,
						 plat_data->analog_regulator);
		if (!IS_ERR(analog_regulator)) {
			regulator_set_voltage(analog_regulator,
					      MT9V114_VOLTAGE_ANALOG,
					      MT9V114_VOLTAGE_ANALOG);
			if (regulator_enable(analog_regulator) != 0) {
				pr_err("%s:analog set voltage error\n",
					__func__);
				goto err3;
			} else {
				dev_dbg(&client->dev,
					"%s:analog set voltage ok\n", __func__);
			}
		} else
			analog_regulator = NULL;
	}

	if (plat_data->pwdn)
		plat_data->pwdn(0);

	camera_plat = plat_data;

	mt9v114_int_device.priv = &mt9v114_data;
	retval = v4l2_int_device_register(&mt9v114_int_device);

	return retval;

err3:
	if (core_regulator) {
		regulator_disable(core_regulator);
		regulator_put(core_regulator);
	}
err2:
	if (io_regulator) {
		regulator_disable(io_regulator);
		regulator_put(io_regulator);
	}
err1:
	return -1;
}

/*!
 * mt9v114 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int mt9v114_remove(struct i2c_client *client)
{
	v4l2_int_device_unregister(&mt9v114_int_device);

	if (gpo_regulator) {
		regulator_disable(gpo_regulator);
		regulator_put(gpo_regulator);
	}

	if (analog_regulator) {
		regulator_disable(analog_regulator);
		regulator_put(analog_regulator);
	}

	if (core_regulator) {
		regulator_disable(core_regulator);
		regulator_put(core_regulator);
	}

	if (io_regulator) {
		regulator_disable(io_regulator);
		regulator_put(io_regulator);
	}

	return 0;
}

/*!
 * mt9v114 init function
 * Called by insmod mt9v114_camera.ko.
 *
 * @return  Error code indicating success or failure
 */
static __init int mt9v114_init(void)
{
	printk(KERN_INFO "%s is called", __func__);
	u8 err;

	err = i2c_add_driver(&mt9v114_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d \n",
			__func__, err);

	return err;
}

/*!
 * MT9V114 cleanup function
 * Called on rmmod mt9v114_camera.ko
 *
 * @return  Error code indicating success or failure
 */
static void __exit mt9v114_clean(void)
{
	i2c_del_driver(&mt9v114_i2c_driver);
}

module_init(mt9v114_init);
module_exit(mt9v114_clean);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MT9V114 Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");
