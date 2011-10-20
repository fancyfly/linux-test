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

#define MT9P111_VOLTAGE_ANALOG               2800000
#define MT9P111_VOLTAGE_DIGITAL_CORE         1800000
#define MT9P111_VOLTAGE_DIGITAL_IO           1800000

#define MIN_FPS 15
#define MAX_FPS 30
#define DEFAULT_FPS 30

#define MT9P111_XCLK_MIN 6000000
#define MT9P111_XCLK_MAX 24000000

enum mt9p111_mode {
	mt9p111_mode_MIN = 0,
	mt9p111_mode_VGA_640_480 = 0,
	mt9p111_mode_QSXGA_2592_1944 = 1,
	mt9p111_mode_MAX = 1
};

enum mt9p111_frame_rate {
	mt9p111_15_fps,
	mt9p111_30_fps
};

struct reg_value {
	u16 u16RegAddr;
	u16 u16Val;
	u32 u32Delay_ms;
	bool double_bytes;
};

struct mt9p111_mode_info {
	enum mt9p111_mode mode;
	u32 width;
	u32 height;
	struct reg_value *init_data_ptr;
	u32 init_data_size;
};

/*!
 * Maintains the information on the current state of the sesor.
 */
struct sensor {
	const struct mt9p111_platform_data *platform_data;
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
} mt9p111_data;

static struct reg_value mt9p111_init_setting[] = {
	//+++++++++++++++++++++++++++++++++++++++++++++++++
	// Reset the sensor..hardware specific for demo2 board...toggles the reset pin
	//for devware
	{ 0x0010, 0x0320, 0, 1},	//PLL Dividers = 800
	{ 0x0012, 0x0070, 0, 1},	//PLL P Dividers = 112
	{ 0x0014, 0x2025, 0, 1},	//PLL Control: TEST_BYPASS off = 8229
	{ 0x001E, 0x0773, 0, 1},	//Pad Slew Pad Config = 1911
	{ 0x0022, 0x01E0, 0, 1},	//VDD_DIS counter delay
	{ 0x002A, 0x7B46, 0, 1},	//PLL P Dividers 4-5-6 = 31558
	{ 0x002C, 0x0000, 0, 1},	//PLL P Dividers 7 = 0
	{ 0x002E, 0x0000, 0, 1},	//Sensor Clock Divider = 0
	{ 0x0018, 0x4008, 10, 1},	//Standby Control and Status: Out of standby
	{ 0x30D4, 0x9080, 0, 1},	//Disable Double Samplings
	{ 0x98E, 0x1000, 0, 1},
	{ 0xC86C, 0x0518, 0, 1},//Output Width (A) = 1304
	{ 0xC86E, 0x03D4, 0, 1},//Output Height (A) = 980
	{ 0xC83A, 0x000C, 0, 1},//Row Start (A) = 12
	{ 0xC83C, 0x0018, 0, 1},//Column Start (A) = 24
	{ 0xC83E, 0x07B1, 0, 1},//Row End (A) = 1969
	{ 0xC840, 0x0A45, 0, 1},//Column End (A) = 2629
	{ 0xC842, 0x0001, 0, 1},//Row Speed (A) = 1
	{ 0xC844, 0x0103, 0, 1},//Core Skip X (A) = 259
	{ 0xC846, 0x0103, 0, 1},//Core Skip Y (A) = 259
	{ 0xC848, 0x0103, 0, 1},//Pipe Skip X (A) = 259
	{ 0xC84A, 0x0103, 0, 1},//Pipe Skip Y (A) = 259
	{ 0xC84C, 0x00F6, 0, 1},//Power Mode (A) = 246
	{ 0xC84E, 0x0001, 0, 1},//Bin Mode (A) = 1
	{ 0xC850, 0x00, 0, 0},	//Orientation (A) = 0
	{ 0xC851, 0x00, 0, 0},	//Pixel Order (A) = 0
	{ 0xC852, 0x019C, 0, 1},//Fine Correction (A) = 412
	{ 0xC854, 0x0732, 0, 1},//Fine IT Min (A) = 1842
	{ 0xC856, 0x048E, 0, 1},//Fine IT Max Margin (A) = 1166
	{ 0xC858, 0x0002, 0, 1},//Coarse IT Min (A) = 2
	{ 0xC85A, 0x0001, 0, 1},//Coarse IT Max Margin (A) = 1
	{ 0xC85C, 0x0423, 0, 1},//Min Frame Lines (A) = 1059
	{ 0xC85E, 0xFFFF, 0, 1},//Max Frame Lines (A) = 65535
	{ 0xC860, 0x0423, 0, 1},//Base Frame Lines (A) = 1059
	{ 0xC862, 0x0EB7, 0, 1},//Min Line Length (A) = 3767
	{ 0xC864, 0xFFFE, 0, 1},//Max Line Length (A) = 65534
	{ 0xC866, 0x7B46, 0, 1},//P456 Divider (A) = 31558
	{ 0xC868, 0x0423, 0, 1},//Frame Lines (A) = 1059
	{ 0xC86A, 0x0EB7, 0, 1},//Line Length (A) = 3767
	{ 0xC870, 0x0014, 0, 1},//RX FIFO Watermark (A) = 20
	{ 0xC8AA, 0x0280, 0, 1},//Output_0 Image Width = 640
	{ 0xC8AC, 0x01E0, 0, 1},//Output_0 Image Height = 480
	{ 0xC8AE, 0x0001, 0, 1},//Output_0 Image Format = 1
	{ 0xC8B0, 0x0000, 0, 1},//Output_0 Format Order = 0
	{ 0xC8B8, 0x0004, 0, 1},//Output_0 JPEG control = 4
	{ 0xC8A4, 0x0A28, 0, 1},//Output Width (B) = 2600
	{ 0xC8A6, 0x07A0, 0, 1},//Output Height (B) = 1952
	{ 0xC872, 0x0010, 0, 1},//Row Start (B) = 16
	{ 0xC874, 0x001C, 0, 1},//Column Start (B) = 28
	{ 0xC876, 0x07AF, 0, 1},//Row End (B) = 1967
	{ 0xC878, 0x0A43, 0, 1},//Column End (B) = 2627
	{ 0xC87A, 0x0001, 0, 1},//Row Speed (B) = 1
	{ 0xC87C, 0x0101, 0, 1},//Core Skip X (B) = 257
	{ 0xC87E, 0x0101, 0, 1},//Core Skip Y (B) = 257
	{ 0xC880, 0x0101, 0, 1},//Pipe Skip X (B) = 257
	{ 0xC882, 0x0101, 0, 1},//Pipe Skip Y (B) = 257
	{ 0xC884, 0x00F2, 0, 1},//Power Mode (B) = 242
	{ 0xC886, 0x0000, 0, 1},//Bin Mode (B) = 0
	{ 0xC888, 0x00	, 0, 0},//Orientation (B) = 0
	{ 0xC889, 0x00	, 0, 0},//Pixel Order (B) = 0
	{ 0xC88A, 0x009C, 0, 1},	//Fine Correction (B) = 156
	{ 0xC88C, 0x034A, 0, 1},	//Fine IT Min (B) = 842
	{ 0xC88E, 0x02A6, 0, 1},	//Fine IT Max Margin (B) = 678
	{ 0xC890, 0x0002, 0, 1},	//Coarse IT Min (B) = 2
	{ 0xC892, 0x0001, 0, 1},	//Coarse IT Max Margin (B) = 1
	{ 0xC894, 0x07EF, 0, 1},	//Min Frame Lines (B) = 2031
	{ 0xC896, 0xFFFF, 0, 1},	//Max Frame Lines (B) = 65535
	{ 0xC898, 0x07EF, 0, 1},	//Base Frame Lines (B) = 2031
	{ 0xC89A, 0x37C4, 0, 1},	//Min Line Length (B) = 14276
	{ 0xC89C, 0xFFFE, 0, 1},	//Max Line Length (B) = 65534
	{ 0xC89E, 0x7B46, 0, 1},	//P456 Divider (B) = 31558
	{ 0xC8A0, 0x07EF, 0, 1},	//Frame Lines (B) = 2031
	{ 0xC8A2, 0x37C4, 0, 1},	//Line Length (B) = 14276
	{ 0xC8A8, 0x0014, 0, 1},	//RX FIFO Watermark (B) = 20
	{ 0xC8C0, 0x0A20, 0, 1},	//Output_1 Image Width = 2592
	{ 0xC8C2, 0x0798, 0, 1},	//Output_1 Image Height = 1944
	{ 0xC8C4, 0x0001, 0, 1},	//Output_1 Image Format = 1
	{ 0xC8C6, 0x0000, 0, 1},	//Output_1 Format Order = 0
	{ 0xC8CE, 0x0004, 0, 1},	//Output_1 JPEG control = 4
	{ 0xA010, 0x0119, 0, 1},	//fd_min_expected50hz_flicker_period = 281
	{ 0xA012, 0x012D, 0, 1},	//fd_max_expected50hz_flicker_period = 301
	{ 0xA014, 0x00E9, 0, 1},	//fd_min_expected60hz_flicker_period = 233
	{ 0xA016, 0x00FD, 0, 1},	//fd_max_expected60hz_flicker_period = 253
	{ 0xA018, 0x0123, 0, 1},	//fd_expected50hz_flicker_period (A) = 291
	{ 0xA01A, 0x004D, 0, 1},	//fd_expected50hz_flicker_period (B) = 77
	{ 0xA01C, 0x00F3, 0, 1},	//fd_expected60hz_flicker_period (A) = 243
	{ 0xA01E, 0x0040, 0, 1},	//fd_expected60hz_flicker_period (B) = 64
	{ 0xDC0A, 0x06	, 0, 0},//Scaler Allow Zoom Ratio = 6
	{ 0xDC1C, 0x2710, 0, 1},	//System Zoom Ratio = 10000
	{ 0xE004, 0x0F00, 50, 1},	//I2C Master Clock Divider = 3840

	//  k28a_rev03_patch01_basic_REV5
	{0x0982, 0x0000, 0, 1}, 	// ACCESS_CTL_STAT
	{0x098A, 0x0000, 0, 1}, 	// PHYSICAL_ADDRESS_ACCESS
	{0x886C, 0xC0F1, 0, 1},
	{0x886E, 0xC5E1, 0, 1},
	{0x8870, 0x246A, 0, 1},
	{0x8872, 0x1280, 0, 1},
	{0x8874, 0xC4E1, 0, 1},
	{0x8876, 0xD20F, 0, 1},
	{0x8878, 0x2069, 0, 1},
	{0x887A, 0x0000, 0, 1},
	{0x887C, 0x6A62, 0, 1},
	{0x887E, 0x1303, 0, 1},
	{0x8880, 0x0084, 0, 1},
	{0x8882, 0x1734, 0, 1},
	{0x8884, 0x7005, 0, 1},
	{0x8886, 0xD801, 0, 1},
	{0x8888, 0x8A41, 0, 1},
	{0x888A, 0xD900, 0, 1},
	{0x888C, 0x0D5A, 0, 1},
	{0x888E, 0x0664, 0, 1},
	{0x8890, 0x8B61, 0, 1},
	{0x8892, 0xE80B, 0, 1},
	{0x8894, 0x000D, 0, 1},
	{0x8896, 0x0020, 0, 1},
	{0x8898, 0xD508, 0, 1},
	{0x889A, 0x1504, 0, 1},
	{0x889C, 0x1400, 0, 1},
	{0x889E, 0x7840, 0, 1},
	{0x88A0, 0xD007, 0, 1},
	{0x88A2, 0x0DFB, 0, 1},
	{0x88A4, 0x9004, 0, 1},
	{0x88A6, 0xC4C1, 0, 1},
	{0x88A8, 0x2029, 0, 1},
	{0x88AA, 0x0300, 0, 1},
	{0x88AC, 0x0219, 0, 1},
	{0x88AE, 0x06C4, 0, 1},
	{0x88B0, 0xFF80, 0, 1},
	{0x88B2, 0x08C4, 0, 1},
	{0x88B4, 0xFF80, 0, 1},
	{0x88B6, 0x086C, 0, 1},
	{0x88B8, 0xFF80, 0, 1},
	{0x88BA, 0x08C0, 0, 1},
	{0x88BC, 0xFF80, 0, 1},
	{0x88BE, 0x08C4, 0, 1},
	{0x88C0, 0xFF80, 0, 1},
	{0x88C2, 0x097C, 0, 1},
	{0x88C4, 0x0001, 0, 1},
	{0x88C6, 0x0005, 0, 1},
	{0x88C8, 0x0000, 0, 1},
	{0x88CA, 0x0000, 0, 1},
	{0x88CC, 0xC0F1, 0, 1},
	{0x88CE, 0x0976, 0, 1},
	{0x88D0, 0x06C4, 0, 1},
	{0x88D2, 0xD639, 0, 1},
	{0x88D4, 0x7708, 0, 1},
	{0x88D6, 0x8E01, 0, 1},
	{0x88D8, 0x1604, 0, 1},
	{0x88DA, 0x1091, 0, 1},
	{0x88DC, 0x2046, 0, 1},
	{0x88DE, 0x00C1, 0, 1},
	{0x88E0, 0x202F, 0, 1},
	{0x88E2, 0x2047, 0, 1},
	{0x88E4, 0xAE21, 0, 1},
	{0x88E6, 0x0F8F, 0, 1},
	{0x88E8, 0x1440, 0, 1},
	{0x88EA, 0x8EAA, 0, 1},
	{0x88EC, 0x8E0B, 0, 1},
	{0x88EE, 0x224A, 0, 1},
	{0x88F0, 0x2040, 0, 1},
	{0x88F2, 0x8E2D, 0, 1},
	{0x88F4, 0xBD08, 0, 1},
	{0x88F6, 0x7D05, 0, 1},
	{0x88F8, 0x8E0C, 0, 1},
	{0x88FA, 0xB808, 0, 1},
	{0x88FC, 0x7825, 0, 1},
	{0x88FE, 0x7510, 0, 1},
	{0x8900, 0x22C2, 0, 1},
	{0x8902, 0x248C, 0, 1},
	{0x8904, 0x081D, 0, 1},
	{0x8906, 0x0363, 0, 1},
	{0x8908, 0xD9FF, 0, 1},
	{0x890A, 0x2502, 0, 1},
	{0x890C, 0x1002, 0, 1},
	{0x890E, 0x2A05, 0, 1},
	{0x8910, 0x03FE, 0, 1},
	{0x8912, 0x0A16, 0, 1},
	{0x8914, 0x06E4, 0, 1},
	{0x8916, 0x702F, 0, 1},
	{0x8918, 0x7810, 0, 1},
	{0x891A, 0x7D02, 0, 1},
	{0x891C, 0x7DB0, 0, 1},
	{0x891E, 0xF00B, 0, 1},
	{0x8920, 0x78A2, 0, 1},
	{0x8922, 0x2805, 0, 1},
	{0x8924, 0x03FE, 0, 1},
	{0x8926, 0x0A02, 0, 1},
	{0x8928, 0x06E4, 0, 1},
	{0x892A, 0x702F, 0, 1},
	{0x892C, 0x7810, 0, 1},
	{0x892E, 0x651D, 0, 1},
	{0x8930, 0x7DB0, 0, 1},
	{0x8932, 0x7DAF, 0, 1},
	{0x8934, 0x8E08, 0, 1},
	{0x8936, 0xBD06, 0, 1},
	{0x8938, 0xD120, 0, 1},
	{0x893A, 0xB8C3, 0, 1},
	{0x893C, 0x78A5, 0, 1},
	{0x893E, 0xB88F, 0, 1},
	{0x8940, 0x1908, 0, 1},
	{0x8942, 0x0024, 0, 1},
	{0x8944, 0x2841, 0, 1},
	{0x8946, 0x0201, 0, 1},
	{0x8948, 0x1E26, 0, 1},
	{0x894A, 0x1042, 0, 1},
	{0x894C, 0x0F15, 0, 1},
	{0x894E, 0x1463, 0, 1},
	{0x8950, 0x1E27, 0, 1},
	{0x8952, 0x1002, 0, 1},
	{0x8954, 0x224C, 0, 1},
	{0x8956, 0xA000, 0, 1},
	{0x8958, 0x224A, 0, 1},
	{0x895A, 0x2040, 0, 1},
	{0x895C, 0x22C2, 0, 1},
	{0x895E, 0x2482, 0, 1},
	{0x8960, 0x204F, 0, 1},
	{0x8962, 0x2040, 0, 1},
	{0x8964, 0x224C, 0, 1},
	{0x8966, 0xA000, 0, 1},
	{0x8968, 0xB8A2, 0, 1},
	{0x896A, 0xF204, 0, 1},
	{0x896C, 0x2045, 0, 1},
	{0x896E, 0x2180, 0, 1},
	{0x8970, 0xAE01, 0, 1},
	{0x8972, 0x0D9E, 0, 1},
	{0x8974, 0xFFE3, 0, 1},
	{0x8976, 0x70E9, 0, 1},
	{0x8978, 0x0125, 0, 1},
	{0x897A, 0x06C4, 0, 1},
	{0x897C, 0xC0F1, 0, 1},
	{0x897E, 0xD010, 0, 1},
	{0x8980, 0xD110, 0, 1},
	{0x8982, 0xD20D, 0, 1},
	{0x8984, 0xA020, 0, 1},
	{0x8986, 0x8A00, 0, 1},
	{0x8988, 0x0809, 0, 1},
	{0x898A, 0x01DE, 0, 1},
	{0x898C, 0xB8A7, 0, 1},
	{0x898E, 0xAA00, 0, 1},
	{0x8990, 0xDBFF, 0, 1},
	{0x8992, 0x2B41, 0, 1},
	{0x8994, 0x0200, 0, 1},
	{0x8996, 0xAA0C, 0, 1},
	{0x8998, 0x1228, 0, 1},
	{0x899A, 0x0080, 0, 1},
	{0x899C, 0xAA6D, 0, 1},
	{0x899E, 0x0815, 0, 1},
	{0x89A0, 0x01DE, 0, 1},
	{0x89A2, 0xB8A7, 0, 1},
	{0x89A4, 0x1A28, 0, 1},
	{0x89A6, 0x0002, 0, 1},
	{0x89A8, 0x8123, 0, 1},
	{0x89AA, 0x7960, 0, 1},
	{0x89AC, 0x1228, 0, 1},
	{0x89AE, 0x0080, 0, 1},
	{0x89B0, 0xC0D1, 0, 1},
	{0x89B2, 0x7EE0, 0, 1},
	{0x89B4, 0xFF80, 0, 1},
	{0x89B6, 0x0158, 0, 1},
	{0x89B8, 0xFF00, 0, 1},
	{0x89BA, 0x0618, 0, 1},
	{0x89BC, 0x8000, 0, 1},
	{0x89BE, 0x0008, 0, 1},
	{0x89C0, 0xFF80, 0, 1},
	{0x89C2, 0x0A08, 0, 1},
	{0x89C4, 0xE280, 0, 1},
	{0x89C6, 0x24CA, 0, 1},
	{0x89C8, 0x7082, 0, 1},
	{0x89CA, 0x78E0, 0, 1},
	{0x89CC, 0x20E8, 0, 1},
	{0x89CE, 0x01A2, 0, 1},
	{0x89D0, 0x1002, 0, 1},
	{0x89D2, 0x0D02, 0, 1},
	{0x89D4, 0x1902, 0, 1},
	{0x89D6, 0x0094, 0, 1},
	{0x89D8, 0x7FE0, 0, 1},
	{0x89DA, 0x7028, 0, 1},
	{0x89DC, 0x7308, 0, 1},
	{0x89DE, 0x1000, 0, 1},
	{0x89E0, 0x0900, 0, 1},
	{0x89E2, 0x7904, 0, 1},
	{0x89E4, 0x7947, 0, 1},
	{0x89E6, 0x1B00, 0, 1},
	{0x89E8, 0x0064, 0, 1},
	{0x89EA, 0x7EE0, 0, 1},
	{0x89EC, 0xE280, 0, 1},
	{0x89EE, 0x24CA, 0, 1},
	{0x89F0, 0x7082, 0, 1},
	{0x89F2, 0x78E0, 0, 1},
	{0x89F4, 0x20E8, 0, 1},
	{0x89F6, 0x01A2, 0, 1},
	{0x89F8, 0x1102, 0, 1},
	{0x89FA, 0x0502, 0, 1},
	{0x89FC, 0x1802, 0, 1},
	{0x89FE, 0x00B4, 0, 1},
	{0x8A00, 0x7FE0, 0, 1},
	{0x8A02, 0x7028, 0, 1},
	{0x8A04, 0x0000, 0, 1},
	{0x8A06, 0x0000, 0, 1},
	{0x8A08, 0xFF80, 0, 1},
	{0x8A0A, 0x097C, 0, 1},
	{0x8A0C, 0xFF80, 0, 1},
	{0x8A0E, 0x08CC, 0, 1},
	{0x8A10, 0x0000, 0, 1},
	{0x8A12, 0x08DC, 0, 1},
	{0x8A14, 0x0000, 0, 1},
	{0x8A16, 0x0998, 0, 1},
	{0x098E, 0x0016, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [MON_ADDRESS_LO]
	{0x8016, 0x086C, 0, 1}, 	// MON_ADDRESS_LO
	{0x8002, 0x0001, 250, 1}, 	// MON_CMD
	//  POLL  MON_PATCH_0 =>  0x01

	{0x098E, 0xC40C, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS
	{0xC40C, 0x00FF, 0, 1}, 	// AFM_POS_MAX
	{0xC40A, 0x0000, 0, 1}, 	// AFM_POS_MIN


	// Patch-specific defaults that need to changed
	{0x098E, 0xC40C, 0, 1},     // LOGICAL_ADDRESS_ACCESS
	{0xC40C, 0x00FF, 0, 1},     // AFM_POS_MAX
	{0xC40A, 0x0000, 0, 1},     // AFM_POS_MIN


	{0x30D4, 0x9080, 0, 1}, 	// COLUMN_CORRECTION
	{0x316E, 0xCAFF, 0, 1}, 	// DAC_ECL
	{0x305E, 0x10A0, 0, 1}, 	// GLOBAL_GAIN
	{0x3E00, 0x0010, 0, 1}, 	// SAMP_CONTROL
	{0x3E02, 0xED02, 0, 1}, 	// SAMP_ADDR_EN
	{0x3E04, 0xC88C, 0, 1}, 	// SAMP_RD1_SIG
	{0x3E06, 0xC88C, 0, 1}, 	// SAMP_RD1_SIG_BOOST
	{0x3E08, 0x700A, 0, 1}, 	// SAMP_RD1_RST
	{0x3E0A, 0x701E, 0, 1}, 	// SAMP_RD1_RST_BOOST
	{0x3E0C, 0x00FF, 0, 1}, 	// SAMP_RST1_EN
	{0x3E0E, 0x00FF, 0, 1}, 	// SAMP_RST1_BOOST
	{0x3E10, 0x00FF, 0, 1}, 	// SAMP_RST1_CLOOP_SH
	{0x3E12, 0x0000, 0, 1}, 	// SAMP_RST_BOOST_SEQ
	{0x3E14, 0xC78C, 0, 1}, 	// SAMP_SAMP1_SIG
	{0x3E16, 0x6E06, 0, 1}, 	// SAMP_SAMP1_RST
	{0x3E18, 0xA58C, 0, 1}, 	// SAMP_TX_EN
	{0x3E1A, 0xA58E, 0, 1}, 	// SAMP_TX_BOOST
	{0x3E1C, 0xA58E, 0, 1}, 	// SAMP_TX_CLOOP_SH
	{0x3E1E, 0xC0D0, 0, 1}, 	// SAMP_TX_BOOST_SEQ
	{0x3E20, 0xEB00, 0, 1}, 	// SAMP_VLN_EN
	{0x3E22, 0x00FF, 0, 1}, 	// SAMP_VLN_HOLD
	{0x3E24, 0xEB02, 0, 1}, 	// SAMP_VCL_EN
	{0x3E26, 0xEA02, 0, 1}, 	// SAMP_COLCLAMP
	{0x3E28, 0xEB0A, 0, 1}, 	// SAMP_SH_VCL
	{0x3E2A, 0xEC01, 0, 1}, 	// SAMP_SH_VREF
	{0x3E2C, 0xEB01, 0, 1}, 	// SAMP_SH_VBST
	{0x3E2E, 0x00FF, 0, 1}, 	// SAMP_SPARE
	{0x3E30, 0x00F3, 0, 1}, 	// SAMP_READOUT
	{0x3E32, 0x3DFA, 0, 1}, 	// SAMP_RESET_DONE
	{0x3E34, 0x00FF, 0, 1}, 	// SAMP_VLN_CLAMP
	{0x3E36, 0x00F3, 0, 1}, 	// SAMP_ASC_INT
	{0x3E38, 0x0000, 0, 1}, 	// SAMP_RS_CLOOP_SH_R
	{0x3E3A, 0xF802, 0, 1}, 	// SAMP_RS_CLOOP_SH
	{0x3E3C, 0x0FFF, 0, 1}, 	// SAMP_RS_BOOST_SEQ
	{0x3E3E, 0xEA10, 0, 1}, 	// SAMP_TXLO_GND
	{0x3E40, 0xEB05, 0, 1}, 	// SAMP_VLN_PER_COL
	{0x3E42, 0xE5C8, 0, 1}, 	// SAMP_RD2_SIG
	{0x3E44, 0xE5C8, 0, 1}, 	// SAMP_RD2_SIG_BOOST
	{0x3E46, 0x8C70, 0, 1}, 	// SAMP_RD2_RST
	{0x3E48, 0x8C71, 0, 1}, 	// SAMP_RD2_RST_BOOST
	{0x3E4A, 0x00FF, 0, 1}, 	// SAMP_RST2_EN
	{0x3E4C, 0x00FF, 0, 1}, 	// SAMP_RST2_BOOST
	{0x3E4E, 0x00FF, 0, 1}, 	// SAMP_RST2_CLOOP_SH
	{0x3E50, 0xE38D, 0, 1}, 	// SAMP_SAMP2_SIG
	{0x3E52, 0x8B0A, 0, 1}, 	// SAMP_SAMP2_RST
	{0x3E58, 0xEB0A, 0, 1}, 	// SAMP_PIX_CLAMP_EN
	{0x3E5C, 0x0A00, 0, 1}, 	// SAMP_PIX_PULLUP_EN
	{0x3E5E, 0x00FF, 0, 1}, 	// SAMP_PIX_PULLDOWN_EN_R
	{0x3E60, 0x00FF, 0, 1}, 	// SAMP_PIX_PULLDOWN_EN_S
	{0x3E90, 0x3C01, 0, 1}, 	// RST_ADDR_EN
	{0x3E92, 0x00FF, 0, 1}, 	// RST_RST_EN
	{0x3E94, 0x00FF, 0, 1}, 	// RST_RST_BOOST
	{0x3E96, 0x3C00, 0, 1}, 	// RST_TX_EN
	{0x3E98, 0x3C00, 0, 1}, 	// RST_TX_BOOST
	{0x3E9A, 0x3C00, 0, 1}, 	// RST_TX_CLOOP_SH
	{0x3E9C, 0xC0E0, 0, 1}, 	// RST_TX_BOOST_SEQ
	{0x3E9E, 0x00FF, 0, 1}, 	// RST_RST_CLOOP_SH
	{0x3EA0, 0x0000, 0, 1}, 	// RST_RST_BOOST_SEQ
	{0x3EA6, 0x3C00, 0, 1}, 	// RST_PIX_PULLUP_EN
	{0x3ED8, 0x3057, 0, 1}, 	// DAC_LD_12_13
	{0x316C, 0xB44F, 0, 1}, 	// DAC_TXLO
	{0x316E, 0xCAFF, 0, 1}, 	// DAC_ECL
	{0x3ED2, 0xEA0A, 0, 1}, 	// DAC_LD_6_7
	{0x3ED4, 0x00A3, 0, 1}, 	// DAC_LD_8_9
	{0x3EDC, 0x6020, 0, 1}, 	// DAC_LD_16_17
	{0x3EE6, 0xA541, 0, 1}, 	// DAC_LD_26_27
	{0x31E0, 0x0000, 0, 1}, 	// PIX_DEF_ID
	{0x3ED0, 0x2409, 0, 1}, 	// DAC_LD_4_5
	{0x3EDE, 0x0A49, 0, 1}, 	// DAC_LD_18_19
	{0x3EE0, 0x4910, 0, 1}, 	// DAC_LD_20_21
	{0x3EE2, 0x09D2, 0, 1}, 	// DAC_LD_22_23
	{0x30B6, 0x0006, 0, 1}, 	// AUTOLR_CONTROL
	{0x8404, 0x06, 100, 0}, 	// SEQ_CMD
	{0x337C, 0x0006, 0, 1}, 	// YUV_YCBCR_CONTROL

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	//Lens shading correction
	{0x3640, 0x03D0, 0, 1},
	{0x3642, 0x00CD, 0, 1},
	{0x3644, 0x37F1, 0, 1},
	{0x3646, 0x75ED, 0, 1},
	{0x3648, 0x9FD1, 0, 1},
	{0x364A, 0x0590, 0, 1},
	{0x364C, 0x9B0E, 0, 1},
	{0x364E, 0x4130, 0, 1},
	{0x3650, 0x41CF, 0, 1},
	{0x3652, 0xCB30, 0, 1},
	{0x3654, 0x03F0, 0, 1},
	{0x3656, 0x3F0C, 0, 1},
	{0x3658, 0x6CF0, 0, 1},
	{0x365A, 0x750C, 0, 1},
	{0x365C, 0xD390, 0, 1},
	{0x365E, 0x0310, 0, 1},
	{0x3660, 0xB94E, 0, 1},
	{0x3662, 0x24D1, 0, 1},
	{0x3664, 0x29AF, 0, 1},
	{0x3666, 0x9831, 0, 1},
	{0x3680, 0x3DCD, 0, 1},
	{0x3682, 0x8BF0, 0, 1},
	{0x3684, 0x8D0D, 0, 1},
	{0x3686, 0x0731, 0, 1},
	{0x3688, 0x5F10, 0, 1},
	{0x368A, 0x108F, 0, 1},
	{0x368C, 0xC40F, 0, 1},
	{0x368E, 0x9B4F, 0, 1},
	{0x3690, 0x59B0, 0, 1},
	{0x3692, 0x30AF, 0, 1},
	{0x3694, 0x1DEC, 0, 1},
	{0x3696, 0xCA2E, 0, 1},
	{0x3698, 0x056F, 0, 1},
	{0x369A, 0x5BEF, 0, 1},
	{0x369C, 0x5CAB, 0, 1},
	{0x369E, 0x23AD, 0, 1},
	{0x36A0, 0x9890, 0, 1},
	{0x36A2, 0x074F, 0, 1},
	{0x36A4, 0x0E91, 0, 1},
	{0x36A6, 0x75ED, 0, 1},
	{0x36C0, 0x6231, 0, 1},
	{0x36C2, 0xF42F, 0, 1},
	{0x36C4, 0xFE30, 0, 1},
	{0x36C6, 0x4832, 0, 1},
	{0x36C8, 0xDD53, 0, 1},
	{0x36CA, 0x39D1, 0, 1},
	{0x36CC, 0xE470, 0, 1},
	{0x36CE, 0x8532, 0, 1},
	{0x36D0, 0x3A72, 0, 1},
	{0x36D2, 0xC991, 0, 1},
	{0x36D4, 0x41B1, 0, 1},
	{0x36D6, 0x458D, 0, 1},
	{0x36D8, 0x5FCE, 0, 1},
	{0x36DA, 0x7951, 0, 1},
	{0x36DC, 0xF533, 0, 1},
	{0x36DE, 0x7411, 0, 1},
	{0x36E0, 0x99B1, 0, 1},
	{0x36E2, 0xD150, 0, 1},
	{0x36E4, 0x0B73, 0, 1},
	{0x36E6, 0xE913, 0, 1},
	{0x3700, 0x3B30, 0, 1},
	{0x3702, 0x1311, 0, 1},
	{0x3704, 0x2450, 0, 1},
	{0x3706, 0xDDD1, 0, 1},
	{0x3708, 0x88D1, 0, 1},
	{0x370A, 0x06F0, 0, 1},
	{0x370C, 0x59F0, 0, 1},
	{0x370E, 0xE6CF, 0, 1},
	{0x3710, 0xD7D1, 0, 1},
	{0x3712, 0x7B92, 0, 1},
	{0x3714, 0x580E, 0, 1},
	{0x3716, 0x6A2F, 0, 1},
	{0x3718, 0x38D1, 0, 1},
	{0x371A, 0x7D6D, 0, 1},
	{0x371C, 0xB991, 0, 1},
	{0x371E, 0x7D8F, 0, 1},
	{0x3720, 0x19B0, 0, 1},
	{0x3722, 0x2330, 0, 1},
	{0x3724, 0xC88F, 0, 1},
	{0x3726, 0xA58F, 0, 1},
	{0x3740, 0xCB51, 0, 1},
	{0x3742, 0x1813, 0, 1},
	{0x3744, 0xFB74, 0, 1},
	{0x3746, 0x9DF5, 0, 1},
	{0x3748, 0x5C16, 0, 1},
	{0x374A, 0xFB71, 0, 1},
	{0x374C, 0x2BF3, 0, 1},
	{0x374E, 0xBBF3, 0, 1},
	{0x3750, 0x9235, 0, 1},
	{0x3752, 0x1A36, 0, 1},
	{0x3754, 0xA6F1, 0, 1},
	{0x3756, 0x58F2, 0, 1},
	{0x3758, 0xFA74, 0, 1},
	{0x375A, 0x80B5, 0, 1},
	{0x375C, 0x5136, 0, 1},
	{0x375E, 0xDC71, 0, 1},
	{0x3760, 0x4E53, 0, 1},
	{0x3762, 0x8475, 0, 1},
	{0x3764, 0xAF55, 0, 1},
	{0x3766, 0x64D6, 0, 1},
	{0x3782, 0x03B4, 0, 1},
	{0x3784, 0x051C, 0, 1}, 	// CENTER_COLUMN
	{0x3210, 0x49B8, 1, 1}, 	// COLOR_PIPELINE_CONTROL

	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	{0x098E, 0xAC01, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [AWB_MODE]
	{0xAC01, 0xAB  , 0, 0}, // AWB_MODE
	{0xAC46, 0x0221, 0, 1}, 	// AWB_LEFT_CCM_0
	{0xAC48, 0xFEAE, 0, 1}, 	// AWB_LEFT_CCM_1
	{0xAC4A, 0x0032, 0, 1}, 	// AWB_LEFT_CCM_2
	{0xAC4C, 0xFFC5, 0, 1}, 	// AWB_LEFT_CCM_3
	{0xAC4E, 0x0154, 0, 1}, 	// AWB_LEFT_CCM_4
	{0xAC50, 0xFFE7, 0, 1}, 	// AWB_LEFT_CCM_5
	{0xAC52, 0xFFB1, 0, 1}, 	// AWB_LEFT_CCM_6
	{0xAC54, 0xFEC5, 0, 1}, 	// AWB_LEFT_CCM_7
	{0xAC56, 0x028A, 0, 1}, 	// AWB_LEFT_CCM_8
	{0xAC58, 0x0130, 0, 1}, 	// AWB_LEFT_CCM_R2BRATIO
	{0xAC5C, 0x01CD, 0, 1}, 	// AWB_RIGHT_CCM_0
	{0xAC5E, 0xFF63, 0, 1}, 	// AWB_RIGHT_CCM_1
	{0xAC60, 0xFFD0, 0, 1}, 	// AWB_RIGHT_CCM_2
	{0xAC62, 0xFFCD, 0, 1}, 	// AWB_RIGHT_CCM_3
	{0xAC64, 0x013B, 0, 1}, 	// AWB_RIGHT_CCM_4
	{0xAC66, 0xFFF8, 0, 1}, 	// AWB_RIGHT_CCM_5
	{0xAC68, 0xFFFB, 0, 1}, 	// AWB_RIGHT_CCM_6
	{0xAC6A, 0xFF78, 0, 1}, 	// AWB_RIGHT_CCM_7
	{0xAC6C, 0x018D, 0, 1}, 	// AWB_RIGHT_CCM_8
	{0xAC6E, 0x0055, 0, 1}, 	// AWB_RIGHT_CCM_R2BRATIO
	{0xB842, 0x0037, 0, 1}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_X
	{0xB844, 0x0044, 0, 1}, 	// STAT_AWB_GRAY_CHECKER_OFFSET_Y
	{0x3240, 0x0024, 0, 1}, 	// AWB_XY_SCALE
	{0x3240, 0x0024, 0, 1}, 	// AWB_XY_SCALE
	{0x3242, 0x0000, 0, 1}, 	// AWB_WEIGHT_R0
	{0x3244, 0x0000, 0, 1}, 	// AWB_WEIGHT_R1
	{0x3246, 0x0000, 0, 1}, 	// AWB_WEIGHT_R2
	{0x3248, 0x7F00, 0, 1}, 	// AWB_WEIGHT_R3
	{0x324A, 0xA500, 0, 1}, 	// AWB_WEIGHT_R4
	{0x324C, 0x1540, 0, 1}, 	// AWB_WEIGHT_R5
	{0x324E, 0x01AC, 0, 1}, 	// AWB_WEIGHT_R6
	{0x3250, 0x003E, 0, 1}, 	// AWB_WEIGHT_R7
	{0x8404, 0x06  , 100, 0}, // SEQ_CMD

	{0xAC3C, 0x2E, 0, 0}, 	// AWB_MIN_ACCEPTED_PRE_AWB_R2G_RATIO
	{0xAC3D, 0x84, 0, 0}, 	// AWB_MAX_ACCEPTED_PRE_AWB_R2G_RATIO
	{0xAC3E, 0x11, 0, 0}, 	// AWB_MIN_ACCEPTED_PRE_AWB_B2G_RATIO
	{0xAC3F, 0x63, 0, 0}, 	// AWB_MAX_ACCEPTED_PRE_AWB_B2G_RATIO
	{0xACB0, 0x2B, 0, 0}, 	// AWB_RG_MIN
	{0xACB1, 0x84, 0, 0}, 	// AWB_RG_MAX
	{0xACB4, 0x11, 0, 0}, 	// AWB_BG_MIN
	{0xACB5, 0x63, 0, 0}, 	// AWB_BG_MAX


	//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	{0xD80F, 0x04, 0, 0},// JPEG_QSCALE_0
	{0xD810, 0x08, 0, 0},// JPEG_QSCALE_1
	{0xC8D2, 0x04, 0, 0},// CAM_OUTPUT_1_JPEG_QSCALE_0
	{0xC8D3, 0x08, 0, 0},// CAM_OUTPUT_1_JPEG_QSCALE_1
	{0xC8BC, 0x04, 0, 0},// CAM_OUTPUT_0_JPEG_QSCALE_0
	{0xC8BD, 0x08, 0, 0},// CAM_OUTPUT_0_JPEG_QSCALE_1
	{0x301A, 0x10F4, 0, 1},	// RESET_REGISTER
	{0x301E, 0x0000, 0, 1},	// DATA_PEDESTAL
	{0x301A, 0x10FC, 0, 1},	// RESET_REGISTER
	{0xDC33, 0x00, 0, 0},// SYS_FIRST_BLACK_LEVEL
	{0xDC35, 0x04, 0, 0},// SYS_UV_COLOR_BOOST
	{0x326E, 0x0006, 0, 1},	// LOW_PASS_YUV_FILTER
	{0xDC37, 0x62, 0, 0},// SYS_BRIGHT_COLORKILL
	{0x35A4, 0x0596, 0, 1},	// BRIGHT_COLOR_KILL_CONTROLS
	{0x35A2, 0x0094, 0, 1},	// DARK_COLOR_KILL_CONTROLS
	{0xDC36, 0x23, 0, 0},// SYS_DARK_COLOR_KILL
	{0x8404, 0x06, 300, 0},// SEQ_CMD

	{0xBC18, 0x00, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_0
	{0xBC19, 0x11, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_1
	{0xBC1A, 0x23, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_2
	{0xBC1B, 0x3F, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_3
	{0xBC1C, 0x67, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_4
	{0xBC1D, 0x85, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_5
	{0xBC1E, 0x9B, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_6
	{0xBC1F, 0xAD, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_7
	{0xBC20, 0xBB, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_8
	{0xBC21, 0xC7, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_9
	{0xBC22, 0xD1, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_10
	{0xBC23, 0xDA, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_11
	{0xBC24, 0xE1, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_12
	{0xBC25, 0xE8, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_13
	{0xBC26, 0xEE, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_14
	{0xBC27, 0xF3, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_15
	{0xBC28, 0xF7, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_16
	{0xBC29, 0xFB, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_17
	{0xBC2A, 0xFF, 0, 0}, 	// LL_GAMMA_CONTRAST_CURVE_18
	{0xBC2B, 0x00, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_0
	{0xBC2C, 0x11, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_1
	{0xBC2D, 0x23, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_2
	{0xBC2E, 0x3F, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_3
	{0xBC2F, 0x67, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_4
	{0xBC30, 0x85, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_5
	{0xBC31, 0x9B, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_6
	{0xBC32, 0xAD, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_7
	{0xBC33, 0xBB, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_8
	{0xBC34, 0xC7, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_9
	{0xBC35, 0xD1, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_10
	{0xBC36, 0xDA, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_11
	{0xBC37, 0xE1, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_12
	{0xBC38, 0xE8, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_13
	{0xBC39, 0xEE, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_14
	{0xBC3A, 0xF3, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_15
	{0xBC3B, 0xF7, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_16
	{0xBC3C, 0xFB, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_17
	{0xBC3D, 0xFF, 0, 0}, 	// LL_GAMMA_NEUTRAL_CURVE_18
	{0xBC3E, 0x00, 0, 0}, 	// LL_GAMMA_NR_CURVE_0
	{0xBC3F, 0x18, 0, 0}, 	// LL_GAMMA_NR_CURVE_1
	{0xBC40, 0x25, 0, 0}, 	// LL_GAMMA_NR_CURVE_2
	{0xBC41, 0x3A, 0, 0}, 	// LL_GAMMA_NR_CURVE_3
	{0xBC42, 0x59, 0, 0}, 	// LL_GAMMA_NR_CURVE_4
	{0xBC43, 0x70, 0, 0}, 	// LL_GAMMA_NR_CURVE_5
	{0xBC44, 0x81, 0, 0}, 	// LL_GAMMA_NR_CURVE_6
	{0xBC45, 0x90, 0, 0}, 	// LL_GAMMA_NR_CURVE_7
	{0xBC46, 0x9E, 0, 0}, 	// LL_GAMMA_NR_CURVE_8
	{0xBC47, 0xAB, 0, 0}, 	// LL_GAMMA_NR_CURVE_9
	{0xBC48, 0xB6, 0, 0}, 	// LL_GAMMA_NR_CURVE_10
	{0xBC49, 0xC1, 0, 0}, 	// LL_GAMMA_NR_CURVE_11
	{0xBC4A, 0xCB, 0, 0}, 	// LL_GAMMA_NR_CURVE_12
	{0xBC4B, 0xD5, 0, 0}, 	// LL_GAMMA_NR_CURVE_13
	{0xBC4C, 0xDE, 0, 0}, 	// LL_GAMMA_NR_CURVE_14
	{0xBC4D, 0xE7, 0, 0}, 	// LL_GAMMA_NR_CURVE_15
	{0xBC4E, 0xEF, 0, 0}, 	// LL_GAMMA_NR_CURVE_16
	{0xBC4F, 0xF7, 0, 0}, 	// LL_GAMMA_NR_CURVE_17
	{0xBC50, 0xFF, 0, 0}, 	// LL_GAMMA_NR_CURVE_18
	{0x8404, 0x06, 100, 0}, 	// SEQ_CMD

	{0xB801, 0xE0, 0, 0},// STAT_MODE
	{0xB862, 0x04, 0, 0},// STAT_BMTRACKING_SPEED
	{0xB829, 0x02, 0, 0},// STAT_LL_BRIGHTNESS_METRIC_DIVISOR
	{0xB863, 0x02, 0, 0},// STAT_BM_MUL
	{0xB827, 0x0F, 0, 0},// STAT_AE_EV_SHIFT
	{0xA409, 0x37, 0, 0},// AE_RULE_BASE_TARGET
	{0xBC52, 0x00C8, 0, 1},	// LL_START_BRIGHTNESS_METRIC
	{0xBC54, 0x0A28, 0, 1},	// LL_END_BRIGHTNESS_METRIC
	{0xBC58, 0x00C8, 0, 1},	// LL_START_GAIN_METRIC
	{0xBC5A, 0x12C0, 0, 1},	// LL_END_GAIN_METRIC
	{0xBC5E, 0x00FA, 0, 1},	// LL_START_APERTURE_GAIN_BM
	{0xBC60, 0x0258, 0, 1},	// LL_END_APERTURE_GAIN_BM
	{0xBC66, 0x00FA, 0, 1},	// LL_START_APERTURE_GM
	{0xBC68, 0x0258, 0, 1},	// LL_END_APERTURE_GM
	{0xBC86, 0x00C8, 0, 1},	// LL_START_FFNR_GM
	{0xBC88, 0x0640, 0, 1},	// LL_END_FFNR_GM
	{0xBCBC, 0x0040, 0, 1},	// LL_SFFB_START_GAIN
	{0xBCBE, 0x01FC, 0, 1},	// LL_SFFB_END_GAIN
	{0xBCCC, 0x00C8, 0, 1},	// LL_SFFB_START_MAX_GM
	{0xBCCE, 0x0640, 0, 1},	// LL_SFFB_END_MAX_GM
	{0xBC90, 0x00C8, 0, 1},	// LL_START_GRB_GM
	{0xBC92, 0x0640, 0, 1},	// LL_END_GRB_GM
	{0xBC0E, 0x0001, 0, 1},	// LL_GAMMA_CURVE_ADJ_START_POS
	{0xBC10, 0x0002, 0, 1},	// LL_GAMMA_CURVE_ADJ_MID_POS
	{0xBC12, 0x02BC, 0, 1},	// LL_GAMMA_CURVE_ADJ_END_POS
	{0xBCAA, 0x044C, 0, 1},	// LL_CDC_THR_ADJ_START_POS
	{0xBCAC, 0x00AF, 0, 1},	// LL_CDC_THR_ADJ_MID_POS
	{0xBCAE, 0x0009, 0, 1},	// LL_CDC_THR_ADJ_END_POS
	{0xBCD8, 0x00C8, 0, 1},	// LL_PCR_START_BM
	{0xBCDA, 0x0A28, 0, 1},	// LL_PCR_END_BM
	{0x3380, 0x0504, 0, 1},	// KERNEL_CONFIG
	{0xBC94, 0x0C, 0, 0},// LL_GB_START_THRESHOLD_0
	{0xBC95, 0x08, 0, 0},// LL_GB_START_THRESHOLD_1
	{0xBC9C, 0x3C, 0, 0},// LL_GB_END_THRESHOLD_0
	{0xBC9D, 0x28, 0, 0},// LL_GB_END_THRESHOLD_1
	{0x33B0, 0x2A16, 0, 1},	// FFNR_ALPHA_BETA
	{0xBC8A, 0x02, 0, 0},// LL_START_FF_MIX_THRESH_Y
	{0xBC8B, 0x0F, 0, 0},// LL_END_FF_MIX_THRESH_Y
	{0xBC8C, 0xFF, 0, 0},// LL_START_FF_MIX_THRESH_YGAIN
	{0xBC8D, 0xFF, 0, 0},// LL_END_FF_MIX_THRESH_YGAIN
	{0xBC8E, 0xFF, 0, 0},// LL_START_FF_MIX_THRESH_GAIN
	{0xBC8F, 0x00, 0, 0},// LL_END_FF_MIX_THRESH_GAIN
	{0xBCB2, 0x20, 0, 0},// LL_CDC_DARK_CLUS_SLOPE
	{0xBCB3, 0x3A, 0, 0},// LL_CDC_DARK_CLUS_SATUR
	{0xBCB4, 0x39, 0, 0},// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SLOPE
	{0xBCB7, 0x39, 0, 0},// LL_CDC_BRIGHT_CLUS_LO_LIGHT_SATUR
	{0xBCB5, 0x20, 0, 0},// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SLOPE
	{0xBCB8, 0x3A, 0, 0},// LL_CDC_BRIGHT_CLUS_MID_LIGHT_SATUR
	{0xBCB6, 0x80, 0, 0},// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SLOPE
	{0xBCB9, 0x24, 0, 0},// LL_CDC_BRIGHT_CLUS_HI_LIGHT_SATUR
	{0xBCAA, 0x03E8, 0, 1},	// LL_CDC_THR_ADJ_START_POS
	{0xBCAC, 0x012C, 0, 1},	// LL_CDC_THR_ADJ_MID_POS
	{0xBCAE, 0x0009, 0, 1},	// LL_CDC_THR_ADJ_END_POS
	{0x33BA, 0x0084, 0, 1},	// APEDGE_CONTROL
	{0x33BE, 0x0000, 0, 1},	// UA_KNEE_L
	{0x33C2, 0x8800, 0, 1},	// UA_WEIGHTS
	{0xBC5E, 0x0154, 0, 1},	// LL_START_APERTURE_GAIN_BM
	{0xBC60, 0x0640, 0, 1},	// LL_END_APERTURE_GAIN_BM
	{0xBC62, 0x0E, 0, 0},// LL_START_APERTURE_KPGAIN
	{0xBC63, 0x14, 0, 0},// LL_END_APERTURE_KPGAIN
	{0xBC64, 0x0E, 0, 0},// LL_START_APERTURE_KNGAIN
	{0xBC65, 0x14, 0, 0},// LL_END_APERTURE_KNGAIN
	{0xBCE2, 0x0A, 0, 0},// LL_START_POS_KNEE
	{0xBCE3, 0x2B, 0, 0},// LL_END_POS_KNEE
	{0xBCE4, 0x0A, 0, 0},// LL_START_NEG_KNEE
	{0xBCE5, 0x2B, 0, 0},// LL_END_NEG_KNEE
	{0x3210, 0x49B8, 0, 1},	// COLOR_PIPELINE_CONTROL
	{0xBCC0, 0x1F, 0, 0},// LL_SFFB_RAMP_START
	{0xBCC1, 0x03, 0, 0},// LL_SFFB_RAMP_STOP
	{0xBCC2, 0x2C, 0, 0},// LL_SFFB_SLOPE_START
	{0xBCC3, 0x10, 0, 0},// LL_SFFB_SLOPE_STOP
	{0xBCC4, 0x07, 0, 0},// LL_SFFB_THSTART
	{0xBCC5, 0x0B, 0, 0},// LL_SFFB_THSTOP
	{0xBCBA, 0x0009, 0, 1},	// LL_SFFB_CONFIG
	{0x8404, 0x06, 100, 0},// SEQ_CMD

	//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

	//{0x098E, 0x3C14 	// LOGICAL_ADDRESS_ACCESS [LL_GAMMA_FADE_TO_BLACK_START_POS]
	{0xBC14, 0xFFFE, 0, 1}, 	// LL_GAMMA_FADE_TO_BLACK_START_POS
	{0xBC16, 0xFFFE, 0, 1}, 	// LL_GAMMA_FADE_TO_BLACK_END_POS
	{0xBC66, 0x0154, 0, 1}, 	// LL_START_APERTURE_GM
	{0xBC68, 0x07D0, 0, 1}, 	// LL_END_APERTURE_GM
	{0xBC6A, 0x04, 0, 0},// LL_START_APERTURE_INTEGER_GAIN
	{0xBC6B, 0x00, 0, 0},// LL_END_APERTURE_INTEGER_GAIN
	{0xBC6C, 0x00, 0, 0},// LL_START_APERTURE_EXP_GAIN
	{0xBC6D, 0x00, 0, 0},// LL_END_APERTURE_EXP_GAIN
	{0xA81C, 0x0040, 0, 1},	// AE_TRACK_MIN_AGAIN
	{0xA820, 0x01FC, 0, 1},	// AE_TRACK_MAX_AGAIN
	{0xA822, 0x0080, 0, 1},	// AE_TRACK_MIN_DGAIN
	{0xA824, 0x0100, 0, 1},	// AE_TRACK_MAX_DGAIN
	{0xBC56, 0x64, 0, 0},// LL_START_CCM_SATURATION
	{0xBC57, 0x1E, 0, 0},// LL_END_CCM_SATURATION
	{0xBCDE, 0x03, 0, 0},// LL_START_SYS_THRESHOLD
	{0xBCDF, 0x50, 0, 0},// LL_STOP_SYS_THRESHOLD
	{0xBCE0, 0x08, 0, 0},// LL_START_SYS_GAIN
	{0xBCE1, 0x03, 0, 0},// LL_STOP_SYS_GAIN
	{0xBCD0, 0x000A, 0, 1},	// LL_SFFB_SOBEL_FLAT_START
	{0xBCD2, 0x00FE, 0, 1},	// LL_SFFB_SOBEL_FLAT_STOP
	{0xBCD4, 0x001E, 0, 1},	// LL_SFFB_SOBEL_SHARP_START
	{0xBCD6, 0x00FF, 0, 1},	// LL_SFFB_SOBEL_SHARP_STOP
	{0xBCC6, 0x00, 0, 0},// LL_SFFB_SHARPENING_START
	{0xBCC7, 0x00, 0, 0},// LL_SFFB_SHARPENING_STOP
	{0xBCC8, 0x20, 0, 0},// LL_SFFB_FLATNESS_START
	{0xBCC9, 0x40, 0, 0},// LL_SFFB_FLATNESS_STOP
	{0xBCCA, 0x04, 0, 0},// LL_SFFB_TRANSITION_START
	{0xBCCB, 0x00, 0, 0},// LL_SFFB_TRANSITION_STOP
	{0xBCE6, 0x03, 0, 0},// LL_SFFB_ZERO_ENABLE
	{0xBCE6, 0x03, 0, 0},// LL_SFFB_ZERO_ENABLE
	{0xA410, 0x04, 0, 0},// AE_RULE_TARGET_AE_6
	{0xA411, 0x06, 0, 0},// AE_RULE_TARGET_AE_7




	{0x8404, 0x06, 100, 0}, 	// SEQ_CMD

	//delay = 100
	//++++++++++++++++++++++++++++++++++

	//{0x098E, 0xC8BC 	// LOGICAL_ADDRESS_ACCESS [CAM_OUTPUT_0_JPEG_QSCALE_0]
	{0xC8BC, 0x04, 0, 0},  	// CAM_OUTPUT_0_JPEG_QSCALE_0
	{0xC8BD, 0x0A, 0, 0}, 	// CAM_OUTPUT_0_JPEG_QSCALE_1
	{0xC8D2, 0x04, 0, 0}, 	// CAM_OUTPUT_1_JPEG_QSCALE_0
	{0xC8D3, 0x0A, 0, 0}, 	// CAM_OUTPUT_1_JPEG_QSCALE_1
	{0xDC3A, 0x23, 0, 0}, 	// SYS_SEPIA_CR
	{0xDC3B, 0xB2, 0, 0}, 	// SYS_SEPIA_CB

	//++++++++++++++++++++++++++++++++++++++++

	{0x0018, 0x2008, 200, 1},
	{0xA818, 0x0795, 0, 1},
	{0xA81A, 0x0795, 0, 1},
	{0xA409, 0x37, 0, 0},
	{0x098E, 0xBC56, 0, 1},
	{0xBC56, 0xA0, 0, 0}, 	// LL_START_CCM_SATURATION
	{0xBC57, 0x32, 0, 0}, 	// LL_END_CCM_SATURATION

	{0x8404, 0x06, 0, 0},	//Refresh Sequencer Mode = 6
};

static struct reg_value mt9p111_setting_15fps_QSXGA_2592_1944[] = {
	{0x098E, 0x843C, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_5_MAX_FRAME_CNT]
	{0x843C, 0xFF, 0, 0}, 	// SEQ_STATE_CFG_5_MAX_FRAME_CNT
	{0x8404, 0x02, 0, 0}, 	// SEQ_CMD
};

static struct reg_value mt9p111_setting_30fps_VGA_640_480[] = {
	{0x098E, 0x843C, 0, 1}, 	// LOGICAL_ADDRESS_ACCESS [SEQ_STATE_CFG_5_MAX_FRAME_CNT]
	{0x843C, 0x01, 0, 0},	// SEQ_STATE_CFG_5_MAX_FRAME_CNT
	{0x8404, 0x01, 0, 0}, 	// SEQ_CMD
	{0x0016, 0x0447, 0, 1},	// CLOCKS_CONTROL
};

static struct mt9p111_mode_info mt9p111_mode_info_data[2][mt9p111_mode_MAX + 1] = {
	{
		{mt9p111_mode_VGA_640_480, 0, 0, NULL, 0},
		{mt9p111_mode_QSXGA_2592_1944, 2592, 1944,
		mt9p111_setting_15fps_QSXGA_2592_1944,
		ARRAY_SIZE(mt9p111_setting_15fps_QSXGA_2592_1944)},
	},
	{
		{mt9p111_mode_VGA_640_480,    640,  480,
		mt9p111_setting_30fps_VGA_640_480,
		ARRAY_SIZE(mt9p111_setting_30fps_VGA_640_480)},
		{mt9p111_mode_QSXGA_2592_1944, 0, 0, NULL, 0},
	},
};

static struct regulator *io_regulator;
static struct regulator *core_regulator;
static struct regulator *analog_regulator;
static struct regulator *gpo_regulator;
static struct mxc_camera_platform_data *camera_plat;

static int mt9p111_probe(struct i2c_client *adapter,
				const struct i2c_device_id *device_id);
static int mt9p111_remove(struct i2c_client *client);

static s32 mt9p111_read_reg(u16 reg, u16 *val, bool double_bytes);
static s32 mt9p111_write_reg(u16 reg, u16 val, bool double_bytes);

static const struct i2c_device_id mt9p111_id[] = {
	{"mt9p111", 0},
	{},
};

MODULE_DEVICE_TABLE(i2c, mt9p111_id);

static struct i2c_driver mt9p111_i2c_driver = {
	.driver = {
		  .owner = THIS_MODULE,
		  .name  = "mt9p111",
		  },
	.probe  = mt9p111_probe,
	.remove = mt9p111_remove,
	.id_table = mt9p111_id,
};

extern void gpio_sensor_active(unsigned int csi_index);
extern void gpio_sensor_inactive(unsigned int csi);

static s32 mt9p111_read_reg(u16 reg, u16 *val, bool double_bytes)
{
	u8 au8RegBuf[2] = {0};
	u8 u8RdVal[2];

	au8RegBuf[0] = reg >> 8;
	au8RegBuf[1] = reg & 0xff;

	if (2 != i2c_master_send(mt9p111_data.i2c_client, au8RegBuf, 2)) {
		pr_err("%s:write reg error:reg=%x\n",
				__func__, reg);
		return -1;
	}
	if (double_bytes) {
		if (2 != i2c_master_recv(mt9p111_data.i2c_client, u8RdVal, 2)) {
			pr_err("%s:read reg error:reg=%x,val=%x\n",
					__func__, reg, u8RdVal);
			return -1;
		}

		printk("######### val 0 %x , val 1 %x\n", u8RdVal[0], u8RdVal[1]);
		*val = ((u16)u8RdVal[1]) | ((u16)u8RdVal[0] << 8);
	} else {
		if (1 != i2c_master_recv(mt9p111_data.i2c_client, u8RdVal, 1)) {
			pr_err("%s:read reg error:reg=%x,val=%x\n",
					__func__, reg, u8RdVal);
			return -1;
		}

		printk("######### val 0 %x , val 1 %x\n", u8RdVal[0], u8RdVal[1]);
		*val = (u16)u8RdVal[0];
	}

	return *val;
}

static s32 mt9p111_write_reg(u16 reg, u16 val, bool double_bytes)
{
	u8 au8Buf[4] = {0};

	if (double_bytes) {
		au8Buf[0] = reg >> 8;
		au8Buf[1] = reg & 0xff;
		au8Buf[2] = val >> 8;
		au8Buf[3] = val & 0xff;

		if (i2c_master_send(mt9p111_data.i2c_client, au8Buf, 4)
			< 0) {
			pr_err("%s:write reg error:reg=%x,val=%x\n",
				__func__, reg, val);
			return -1;
		}
	} else {
		au8Buf[0] = reg >> 8;
		au8Buf[1] = reg & 0xff;
		au8Buf[2] = (u8)val;

		if (i2c_master_send(mt9p111_data.i2c_client, au8Buf, 3)
			< 0) {
			pr_err("%s:write reg error:reg=%x,val=%x\n",
				__func__, reg, val);
			return -1;
		}
	}

	return 0;
}

static int mt9p111_init_mode(enum mt9p111_frame_rate frame_rate,
			    enum mt9p111_mode mode)
{
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u16 Val = 0;
	int retval = 0;

	if (mode > mt9p111_mode_MAX || mode < mt9p111_mode_MIN) {
		pr_err("Wrong mt9p111 mode detected!\n");
		return -1;
	}

	pModeSetting = mt9p111_mode_info_data[frame_rate][mode].init_data_ptr;
	iModeSettingArySize =
		mt9p111_mode_info_data[frame_rate][mode].init_data_size;

	mt9p111_data.pix.width = mt9p111_mode_info_data[frame_rate][mode].width;
	mt9p111_data.pix.height = mt9p111_mode_info_data[frame_rate][mode].height;

	if (mt9p111_data.pix.width == 0 || mt9p111_data.pix.height == 0 ||
	    pModeSetting == NULL || iModeSettingArySize == 0)
		return -EINVAL;

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = pModeSetting->u32Delay_ms;
		RegAddr = pModeSetting->u16RegAddr;
		Val = pModeSetting->u16Val;

		retval = mt9p111_write_reg(RegAddr, Val, pModeSetting->double_bytes);
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
	p->u.bt656.clock_curr = mt9p111_data.mclk;
	pr_debug("   clock_curr=mclk=%d\n", mt9p111_data.mclk);
	p->if_type = V4L2_IF_TYPE_BT656;
	p->u.bt656.mode = V4L2_IF_TYPE_BT656_MODE_NOBT_8BIT;
	p->u.bt656.clock_min = MT9P111_XCLK_MIN;
	p->u.bt656.clock_max = MT9P111_XCLK_MAX;
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
		gpio_sensor_active(mt9p111_data.csi);
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
		gpio_sensor_inactive(mt9p111_data.csi);
		/* Make sure power down */
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
	enum mt9p111_frame_rate frame_rate;
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
			frame_rate = mt9p111_15_fps;
		else if (tgt_fps == 30)
			frame_rate = mt9p111_30_fps;
		else {
			pr_err(" The camera frame rate is not supported!\n");
			return -EINVAL;
		}

		sensor->streamcap.timeperframe = *timeperframe;
		sensor->streamcap.capturemode =
				(u32)a->parm.capture.capturemode;

		ret = mt9p111_init_mode(frame_rate,
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
		vc->value = mt9p111_data.brightness;
		break;
	case V4L2_CID_HUE:
		vc->value = mt9p111_data.hue;
		break;
	case V4L2_CID_CONTRAST:
		vc->value = mt9p111_data.contrast;
		break;
	case V4L2_CID_SATURATION:
		vc->value = mt9p111_data.saturation;
		break;
	case V4L2_CID_RED_BALANCE:
		vc->value = mt9p111_data.red;
		break;
	case V4L2_CID_BLUE_BALANCE:
		vc->value = mt9p111_data.blue;
		break;
	case V4L2_CID_EXPOSURE:
		vc->value = mt9p111_data.ae_mode;
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

	pr_debug("In mt9p111:ioctl_s_ctrl %d\n",
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
	if (fsize->index > mt9p111_mode_MAX)
		return -EINVAL;

	fsize->pixel_format = mt9p111_data.pix.pixelformat;
	fsize->discrete.width =
			max(mt9p111_mode_info_data[0][fsize->index].width,
			    mt9p111_mode_info_data[1][fsize->index].width);
	fsize->discrete.height =
			max(mt9p111_mode_info_data[0][fsize->index].height,
			    mt9p111_mode_info_data[1][fsize->index].height);
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
	strcpy(((struct v4l2_dbg_chip_ident *)id)->match.name, "mt9p111_camera");

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
	if (fmt->index > mt9p111_mode_MAX)
		return -EINVAL;

	fmt->pixelformat = mt9p111_data.pix.pixelformat;

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
	enum mt9p111_frame_rate frame_rate;
	struct reg_value *pModeSetting = NULL;
	s32 i = 0;
	s32 iModeSettingArySize = 0;
	register u32 Delay_ms = 0;
	register u16 RegAddr = 0;
	register u16 Val = 0;
	int retval = 0;
	u16 myVal = 0;

	gpio_sensor_active(mt9p111_data.csi);
	mt9p111_data.on = true;

	/* mclk */
	tgt_xclk = mt9p111_data.mclk;
	tgt_xclk = min(tgt_xclk, (u32)MT9P111_XCLK_MAX);
	tgt_xclk = max(tgt_xclk, (u32)MT9P111_XCLK_MIN);
	mt9p111_data.mclk = tgt_xclk;

	pr_debug("   Setting mclk to %d MHz\n", tgt_xclk / 1000000);
	set_mclk_rate(&mt9p111_data.mclk, mt9p111_data.csi);

	/* Default camera frame rate is set in probe */
	tgt_fps = sensor->streamcap.timeperframe.denominator /
		  sensor->streamcap.timeperframe.numerator;

	if (tgt_fps == 15)
		frame_rate = mt9p111_15_fps;
	else if (tgt_fps == 30)
		frame_rate = mt9p111_30_fps;
	else
		return -EINVAL; /* Only support 15fps or 30fps now. */

	msleep(10);
	mt9p111_read_reg(0x0000, &myVal, 1);
	printk("####### 1 read chip id 0x%x\n", myVal);

	msleep(10);

	pModeSetting = mt9p111_init_setting;
	iModeSettingArySize = ARRAY_SIZE(mt9p111_init_setting);

	for (i = 0; i < iModeSettingArySize; ++i, ++pModeSetting) {
		Delay_ms = pModeSetting->u32Delay_ms;
		RegAddr = pModeSetting->u16RegAddr;
		Val = pModeSetting->u16Val;

		retval = mt9p111_write_reg(RegAddr, Val, pModeSetting->double_bytes);
		if (retval < 0)
			goto err;

		if (Delay_ms)
			msleep(Delay_ms);
	}
	printk("########### here 1\n");
	msleep(300);
	mt9p111_read_reg(0x0000, &myVal, 1);
	printk("####### 2 read chip id 0x%x\n", myVal);

	mt9p111_write_reg(0x098E, 0x8405, 1);

	mt9p111_read_reg(0x8405, &myVal, 0);
	printk("####### read 0x8405 0x%x\n", myVal);

	mt9p111_read_reg(0xA818, &myVal, 1);
	printk("####### read 0xA818 0x%x\n", myVal);

	mt9p111_read_reg(0xA409, &myVal, 0);
	printk("####### read 0xA409 0x%x\n", myVal);
//	mt9p111_write_reg(0x3070, 0x0002, 1);
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
	gpio_sensor_inactive(mt9p111_data.csi);

	return 0;
}

/*!
 * This structure defines all the ioctls for this module and links them to the
 * enumeration.
 */
static struct v4l2_int_ioctl_desc mt9p111_ioctl_desc[] = {
	{vidioc_int_dev_init_num, (v4l2_int_ioctl_func*)ioctl_dev_init},
	{vidioc_int_dev_exit_num, ioctl_dev_exit},
	{vidioc_int_s_power_num, (v4l2_int_ioctl_func*)ioctl_s_power},
	{vidioc_int_g_ifparm_num, (v4l2_int_ioctl_func*)ioctl_g_ifparm},
/*	{vidioc_int_g_needs_reset_num,
				(v4l2_int_ioctl_func *)ioctl_g_needs_reset}, */
/*	{vidioc_int_reset_num, (v4l2_int_ioctl_func *)ioctl_reset}, */
	{vidioc_int_init_num, (v4l2_int_ioctl_func*)ioctl_init},
	{vidioc_int_enum_fmt_cap_num,
				(v4l2_int_ioctl_func *)ioctl_enum_fmt_cap},
/*	{vidioc_int_try_fmt_cap_num,
				(v4l2_int_ioctl_func *)ioctl_try_fmt_cap}, */
	{vidioc_int_g_fmt_cap_num, (v4l2_int_ioctl_func*)ioctl_g_fmt_cap},
/*	{vidioc_int_s_fmt_cap_num, (v4l2_int_ioctl_func *)ioctl_s_fmt_cap}, */
	{vidioc_int_g_parm_num, (v4l2_int_ioctl_func*)ioctl_g_parm},
	{vidioc_int_s_parm_num, (v4l2_int_ioctl_func*)ioctl_s_parm},
/*	{vidioc_int_queryctrl_num, (v4l2_int_ioctl_func *)ioctl_queryctrl}, */
	{vidioc_int_g_ctrl_num, (v4l2_int_ioctl_func*)ioctl_g_ctrl},
	{vidioc_int_s_ctrl_num, (v4l2_int_ioctl_func*)ioctl_s_ctrl},
	{vidioc_int_enum_framesizes_num,
				(v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{vidioc_int_g_chip_ident_num,
				(v4l2_int_ioctl_func *)ioctl_g_chip_ident},
};

static struct v4l2_int_slave mt9p111_slave = {
	.ioctls = mt9p111_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(mt9p111_ioctl_desc),
};

static struct v4l2_int_device mt9p111_int_device = {
	.module = THIS_MODULE,
	.name = "mt9p111",
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &mt9p111_slave,
	},
};

/*!
 * mt9p111 I2C probe function
 *
 * @param adapter            struct i2c_adapter *
 * @return  Error code indicating success or failure
 */
static int mt9p111_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	int retval;
	struct mxc_camera_platform_data *plat_data = client->dev.platform_data;

	/* Set initial values for the sensor struct. */
	memset(&mt9p111_data, 0, sizeof(mt9p111_data));
	mt9p111_data.mclk = 24000000; /* 6 - 54 MHz, typical 24MHz */
	mt9p111_data.mclk = plat_data->mclk;
	mt9p111_data.csi = plat_data->csi;

	mt9p111_data.i2c_client = client;
	mt9p111_data.pix.pixelformat = V4L2_PIX_FMT_UYVY;
	mt9p111_data.pix.width = 640;
	mt9p111_data.pix.height = 480;
	mt9p111_data.streamcap.capability = V4L2_MODE_HIGHQUALITY |
					   V4L2_CAP_TIMEPERFRAME;
	mt9p111_data.streamcap.capturemode = 0;
	mt9p111_data.streamcap.timeperframe.denominator = DEFAULT_FPS;
	mt9p111_data.streamcap.timeperframe.numerator = 1;

	if (plat_data->io_regulator) {
		io_regulator = regulator_get(&client->dev,
					     plat_data->io_regulator);
		if (!IS_ERR(io_regulator)) {
			regulator_set_voltage(io_regulator,
					      MT9P111_VOLTAGE_DIGITAL_IO,
					      MT9P111_VOLTAGE_DIGITAL_IO);
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
					      MT9P111_VOLTAGE_DIGITAL_CORE,
					      MT9P111_VOLTAGE_DIGITAL_CORE);
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
					      MT9P111_VOLTAGE_ANALOG,
					      MT9P111_VOLTAGE_ANALOG);
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

	mt9p111_int_device.priv = &mt9p111_data;
	retval = v4l2_int_device_register(&mt9p111_int_device);

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
 * mt9p111 I2C detach function
 *
 * @param client            struct i2c_client *
 * @return  Error code indicating success or failure
 */
static int mt9p111_remove(struct i2c_client *client)
{
	v4l2_int_device_unregister(&mt9p111_int_device);

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
 * mt9p111 init function
 * Called by insmod mt9p111_camera.ko.
 *
 * @return  Error code indicating success or failure
 */
static __init int mt9p111_init(void)
{
	u8 err;

	err = i2c_add_driver(&mt9p111_i2c_driver);
	if (err != 0)
		pr_err("%s:driver registration failed, error=%d \n",
			__func__, err);

	return err;
}

/*!
 * MT9P111 cleanup function
 * Called on rmmod mt9p111_camera.ko
 *
 * @return  Error code indicating success or failure
 */
static void __exit mt9p111_clean(void)
{
	i2c_del_driver(&mt9p111_i2c_driver);
}

module_init(mt9p111_init);
module_exit(mt9p111_clean);

MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_DESCRIPTION("MT9P111 Camera Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_ALIAS("CSI");
