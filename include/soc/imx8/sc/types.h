/*==========================================================================*/
/*!
 * @file  main/types.h
 *
 * Header file containing types used across multiple service APIs.
 */
/*==========================================================================*/

#ifndef _SC_TYPES_H
#define _SC_TYPES_H

/* Includes */

#include <linux/types.h>

/* Defines */

/*!
 * @name Defines for common frequencies
 */
/*@{*/
#define SC_10MHZ         10000000   //!< 10MHz
#define SC_12MHZ         24000000   //!< 12MHz
#define SC_24MHZ         24000000   //!< 24MHz
#define SC_40MHZ         40000000   //!< 40MHz
#define SC_66MHZ         66666666   //!< 66MHz
#define SC_80MHZ         80000000   //!< 80MHz
#define SC_100MHZ       100000000   //!< 100MHz
#define SC_120MHZ       120000000   //!< 120MHz
#define SC_125MHZ       125000000   //!< 125MHz
#define SC_132MHZ       132000000   //!< 132MHz
#define SC_133MHZ       133333333   //!< 133MHz
#define SC_160MHZ       160000000   //!< 160MHz
#define SC_192MHZ       192000000   //!< 192MHz
#define SC_200MHZ       200000000   //!< 200MHz
#define SC_240MHZ       240000000   //!< 240MHz
#define SC_250MHZ       250000000   //!< 250MHz
#define SC_266MHZ       266666666   //!< 266MHz
#define SC_320MHZ       320000000   //!< 320MHz
#define SC_375MHZ       375000000   //!< 375MHz
#define SC_384MHZ       384000000   //!< 384MHz
#define SC_400MHZ       400000000   //!< 400MHz
#define SC_480MHZ       480000000   //!< 480MHz
#define SC_500MHZ       500000000   //!< 500MHz
#define SC_750MHZ       750000000   //!< 750MHz
#define SC_800MHZ       800000000   //!< 800MHz
#define SC_1000MHZ     1000000000   //!< 1GHz
#define SC_1200MHZ     1200000000   //!< 1.2GHz
#define SC_1400MHZ     1400000000   //!< 1.4GHz
#define SC_BYPASS      (INT32_MAX)  //!< Bypass Source, rate is unknown
/*@}*/

/*!
 * @name Defines for type widths
 */
/*@{*/
#define SC_FADDR_W      36      //!< Width of sc_faddr_t
#define SC_BOOL_W       1       //!< Width of bool
#define SC_ERR_W        4       //!< Width of sc_err_t
#define SC_RSRC_W       9       //!< Width of sc_rsrc_t
/*@}*/

#define SC_R_ALL        UINT16_MAX  //!< All resources
#define SC_P_ALL        UINT16_MAX  //!< All pins

/*!
 * This type is used to store a system (full-size) address.
 */
typedef uint64_t sc_faddr_t;

/*!
 * This type is used to indicate error response for most functions.
 */
typedef enum sc_err_e
{
    SC_ERR_NONE         = 0,    //!< Success
    SC_ERR_VERSION      = 1,    //!< Incompatible API version  
    SC_ERR_CONFIG       = 2,    //!< Configuration error  
    SC_ERR_PARM         = 3,    //!< Bad parameter 
    SC_ERR_NOACCESS     = 4,    //!< Permission error (no access)
    SC_ERR_LOCKED       = 5,    //!< Permission error (locked)
    SC_ERR_UNAVAILABLE  = 6,    //!< Unavailable (out of resources)
    SC_ERR_NOTFOUND     = 7,    //!< Not found
    SC_ERR_NOPOWER      = 8,    //!< No power
    SC_ERR_IPC          = 9,    //!< Generic IPC error
    SC_ERR_LAST
} sc_err_t;

/*!
 * This type is used to indicate a resource. Resources include peripherals
 * and bus masters (but not memory regions). Note items from list should
 * never be changed or removed (only added to at the end of the list).
 */
typedef enum sc_rsrc_e
{   
    SC_R_A53                = 0,
    SC_R_A53_0              = 1,
    SC_R_A53_1              = 2,
    SC_R_A53_2              = 3,
    SC_R_A53_3              = 4,
    SC_R_A72                = 5,
    SC_R_A72_0              = 6,
    SC_R_A72_1              = 7,
    SC_R_A72_2              = 8,
    SC_R_A72_3              = 9,
    SC_R_CCI                = 10,
    SC_R_SSI_MST_LSIO       = 11,
    SC_R_PG0_SSI_MST_DRC0   = 12,
    SC_R_PG0_SSI_MST_DRC1   = 13,
    SC_R_PG0_SSI_MST_DRC2   = 14,
    SC_R_PG0_SSI_MST_DRC3   = 15,
    SC_R_PG0_SSI_SLV_SS0    = 16,
    SC_R_PG0_SSI_SLV_SS1    = 17,
    SC_R_PG0_SSI_SLV_SS2    = 18,
    SC_R_PG0_SSI_SLV_SS3    = 19,
    SC_R_PG1_SSI_MST_DRC0   = 20,
    SC_R_PG1_SSI_MST_DRC1   = 21,
    SC_R_PG1_SSI_MST_DRC2   = 22,
    SC_R_PG1_SSI_MST_DRC3   = 23,
    SC_R_PG1_SSI_SLV_SS0    = 24,
    SC_R_PG1_SSI_SLV_SS1    = 25,
    SC_R_PG1_SSI_SLV_SS2    = 26,
    SC_R_PG1_SSI_SLV_SS3    = 27,
    SC_R_PG2_SSI_MST_DRC0   = 28,
    SC_R_PG2_SSI_MST_DRC1   = 29,
    SC_R_PG2_SSI_MST_DRC2   = 30,
    SC_R_PG2_SSI_MST_DRC3   = 31,
    SC_R_PG2_SSI_SLV_SS0    = 32,
    SC_R_PG2_SSI_SLV_SS1    = 33,
    SC_R_PG2_SSI_SLV_SS2    = 34,
    SC_R_PG2_SSI_SLV_SS3    = 35,
    SC_R_PG3_SSI_MST_DRC0   = 36,
    SC_R_PG3_SSI_MST_DRC1   = 37,
    SC_R_PG3_SSI_MST_DRC2   = 38,
    SC_R_PG3_SSI_MST_DRC3   = 39,
    SC_R_PG3_SSI_SLV_SS0    = 40,
    SC_R_PG3_SSI_SLV_SS1    = 41,
    SC_R_PG3_SSI_SLV_SS2    = 42,
    SC_R_PG3_SSI_SLV_SS3    = 43,
    SC_R_DRC_0              = 44,
    SC_R_DRC_1              = 45,
    SC_R_GIC_SMMU           = 46,
    SC_R_SSI_MST_DVM        = 47,
    SC_R_SSI_MST_DBLOG      = 48,
    SC_R_IRQSTR_SCU         = 49,
    SC_R_IRQSTR_M4_0        = 50,
    SC_R_IRQSTR_M4_1        = 51,
    SC_R_IRQSTR_CTI         = 52,
    SC_R_SMMU_GR0           = 53,
    SC_R_SMMU_GR1           = 54,
    SC_R_SMMU_INT           = 55,
    SC_R_SMMU_PM            = 56,
    SC_R_SMMU_SSD0          = 57,
    SC_R_SMMU_SSD1          = 58,
    SC_R_SMMU_CB0           = 59,
    SC_R_SMMU_CB1           = 60,
    SC_R_SMMU_CB2           = 61,
    SC_R_SMMU_CB3           = 62,
    SC_R_SMMU_CB4           = 63,
    SC_R_SMMU_CB5           = 64,
    SC_R_SMMU_CB6           = 65,
    SC_R_SMMU_CB7           = 66,
    SC_R_SMMU_CB8           = 67,
    SC_R_SMMU_CB9           = 68,
    SC_R_SMMU_CB10          = 69,
    SC_R_SMMU_CB11          = 70,
    SC_R_SMMU_CB12          = 71,
    SC_R_SMMU_CB13          = 72,
    SC_R_SMMU_CB14          = 73,
    SC_R_SMMU_CB15          = 74,
    SC_R_SMMU_CB16          = 75,
    SC_R_SMMU_CB17          = 76,
    SC_R_SMMU_CB18          = 77,
    SC_R_SMMU_CB19          = 78,
    SC_R_SMMU_CB20          = 79,
    SC_R_SMMU_CB21          = 80,
    SC_R_SMMU_CB22          = 81,
    SC_R_SMMU_CB23          = 82,
    SC_R_SMMU_CB24          = 83,
    SC_R_SMMU_CB25          = 84,
    SC_R_SMMU_CB26          = 85,
    SC_R_SMMU_CB27          = 86,
    SC_R_SMMU_CB28          = 87,
    SC_R_SMMU_CB29          = 88,
    SC_R_SMMU_CB30          = 89,
    SC_R_SMMU_CB31          = 90,
    SC_R_GIC_DIST           = 91,
    SC_R_GIC_DISTS          = 92,
    SC_R_GIC_ITSC           = 93,
    SC_R_GIC_ITST           = 94,
    SC_R_GIC_RDCL0          = 95,
    SC_R_GIC_RDSP0          = 96,
    SC_R_GIC_RDCL1          = 97,
    SC_R_GIC_RDSP1          = 98,
    SC_R_GIC_RDCL2          = 99,
    SC_R_GIC_RDSP2          = 100,
    SC_R_GIC_RDCL3          = 101,
    SC_R_GIC_RDSP3          = 102,
    SC_R_GIC_RDCL4          = 103,
    SC_R_GIC_RDSP4          = 104,
    SC_R_GIC_RDCL5          = 105,
    SC_R_GIC_RDSP5          = 106,
    SC_R_SPI_0              = 107,
    SC_R_SPI_1              = 108,
    SC_R_SPI_2              = 109,
    SC_R_SPI_3              = 110,
    SC_R_SPI_4              = 111,
    SC_R_SPI_5              = 112,
    SC_R_UART_0             = 113,
    SC_R_UART_1             = 114,
    SC_R_UART_2             = 115,
    SC_R_UART_3             = 116,
    SC_R_UART_4             = 117,
    SC_R_UART_5             = 118,
    SC_R_UART_6             = 119,
    SC_R_EMVSIM_0           = 120,
    SC_R_EMVSIM_1           = 121,
    SC_R_DMA_0              = 122,
    SC_R_DMA_0_CH0          = 123,
    SC_R_DMA_0_CH1          = 124,
    SC_R_DMA_0_CH2          = 125,
    SC_R_DMA_0_CH3          = 126,
    SC_R_DMA_0_CH4          = 127,
    SC_R_DMA_0_CH5          = 128,
    SC_R_DMA_0_CH6          = 129,
    SC_R_DMA_0_CH7          = 130,
    SC_R_DMA_0_CH8          = 131,
    SC_R_DMA_0_CH9          = 132,
    SC_R_DMA_0_CH10         = 133,
    SC_R_DMA_0_CH11         = 134,
    SC_R_DMA_0_CH12         = 135,
    SC_R_DMA_0_CH13         = 136,
    SC_R_DMA_0_CH14         = 137,
    SC_R_DMA_0_CH15         = 138,
    SC_R_DMA_0_CH16         = 139,
    SC_R_DMA_0_CH17         = 140,
    SC_R_DMA_0_CH18         = 141,
    SC_R_DMA_0_CH19         = 142,
    SC_R_DMA_0_CH20         = 143,
    SC_R_DMA_0_CH21         = 144,
    SC_R_DMA_0_CH22         = 145,
    SC_R_DMA_0_CH23         = 146,
    SC_R_DMA_0_CH24         = 147,
    SC_R_DMA_0_CH25         = 148,
    SC_R_DMA_0_CH26         = 149,
    SC_R_DMA_0_CH27         = 150,
    SC_R_DMA_0_CH28         = 151,
    SC_R_DMA_0_CH29         = 152,
    SC_R_DMA_0_CH30         = 153,
    SC_R_DMA_0_CH31         = 154,
    SC_R_I2C_0              = 155,
    SC_R_I2C_1              = 156,
    SC_R_I2C_2              = 157,
    SC_R_I2C_3              = 158,
    SC_R_I2C_4              = 159,
    SC_R_I2C_5              = 160,
    SC_R_I2C_6              = 161,
    SC_R_I2C_7              = 162,
    SC_R_ADC_0              = 163,
    SC_R_ADC_1              = 164,
    SC_R_FTM_0              = 165,
    SC_R_FTM_1              = 166,
    SC_R_FTM_2              = 167,
    SC_R_FTM_3              = 168,
    SC_R_CAN_0              = 169,
    SC_R_CAN_1              = 170,
    SC_R_CAN_2              = 171,
    SC_R_DMA_1              = 172,
    SC_R_DMA_1_CH0          = 173,
    SC_R_DMA_1_CH1          = 174,
    SC_R_DMA_1_CH2          = 175,
    SC_R_DMA_1_CH3          = 176,
    SC_R_DMA_1_CH4          = 177,
    SC_R_DMA_1_CH5          = 178,
    SC_R_DMA_1_CH6          = 179,
    SC_R_DMA_1_CH7          = 180,
    SC_R_DMA_1_CH8          = 181,
    SC_R_DMA_1_CH9          = 182,
    SC_R_DMA_1_CH10         = 183,
    SC_R_DMA_1_CH11         = 184,
    SC_R_DMA_1_CH12         = 185,
    SC_R_DMA_1_CH13         = 186,
    SC_R_DMA_1_CH14         = 187,
    SC_R_DMA_1_CH15         = 188,
    SC_R_DMA_1_CH16         = 189,
    SC_R_DMA_1_CH17         = 190,
    SC_R_DMA_1_CH18         = 191,
    SC_R_DMA_1_CH19         = 192,
    SC_R_DMA_1_CH20         = 193,
    SC_R_DMA_1_CH21         = 194,
    SC_R_DMA_1_CH22         = 195,
    SC_R_DMA_1_CH23         = 196,
    SC_R_DMA_1_CH24         = 197,
    SC_R_DMA_1_CH25         = 198,
    SC_R_DMA_1_CH26         = 199,
    SC_R_DMA_1_CH27         = 200,
    SC_R_DMA_1_CH28         = 201,
    SC_R_DMA_1_CH29         = 202,
    SC_R_DMA_1_CH30         = 203,
    SC_R_DMA_1_CH31         = 204,
    SC_R_DRC_0_V            = 205,
    SC_R_DRC_0_H            = 206,
    SC_R_DRC_1_V            = 207,
    SC_R_DRC_1_H            = 208,
    SC_R_PWM_0              = 209,
    SC_R_PWM_1              = 210,
    SC_R_PWM_2              = 211,
    SC_R_PWM_3              = 212,
    SC_R_PWM_4              = 213,
    SC_R_PWM_5              = 214,
    SC_R_PWM_6              = 215,
    SC_R_PWM_7              = 216,
    SC_R_GPIO_0             = 217,
    SC_R_GPIO_1             = 218,
    SC_R_GPIO_2             = 219,
    SC_R_GPIO_3             = 220,
    SC_R_GPIO_4             = 221,
    SC_R_GPIO_5             = 222,
    SC_R_GPIO_6             = 223,
    SC_R_GPIO_7             = 224,
    SC_R_GPT_0              = 225,
    SC_R_GPT_1              = 226,
    SC_R_GPT_2              = 227,
    SC_R_GPT_3              = 228,
    SC_R_GPT_4              = 229,
    SC_R_KPP                = 230,
    SC_R_MU_0A              = 231,
    SC_R_MU_1A              = 232,
    SC_R_MU_2A              = 233,
    SC_R_MU_3A              = 234,
    SC_R_MU_4A              = 235,
    SC_R_MU_5A              = 236,
    SC_R_MU_6A              = 237,
    SC_R_MU_7A              = 238,
    SC_R_MU_8A              = 239,
    SC_R_MU_9A              = 240,
    SC_R_MU_10A             = 241,
    SC_R_MU_11A             = 242,
    SC_R_MU_12A             = 243,
    SC_R_MU_13A             = 244,
    SC_R_MU_5B              = 245,
    SC_R_MU_6B              = 246,
    SC_R_MU_7B              = 247,
    SC_R_MU_8B              = 248,
    SC_R_MU_9B              = 249,
    SC_R_MU_10B             = 250,
    SC_R_MU_11B             = 251,
    SC_R_MU_12B             = 252,
    SC_R_MU_13B             = 253,
    SC_R_M4_0_PID0          = 254,
    SC_R_M4_0_PID1          = 255,
    SC_R_M4_0_PID2          = 256,
    SC_R_M4_0_PID3          = 257,
    SC_R_M4_0_PID4          = 258,
    SC_R_M4_0_RGPIO_P1      = 259,
    SC_R_M4_0_SEMA42        = 260,
    SC_R_M4_0_TPM           = 261,
    SC_R_M4_0_PIT           = 262,
    SC_R_M4_0_UART          = 263,
    SC_R_M4_0_I2C           = 264,
    SC_R_M4_0_INTMUX        = 265,
    SC_R_M4_0_SIM           = 266,
    SC_R_M4_0_WDOG          = 267,
    SC_R_M4_0_MU_0B         = 268,
    SC_R_M4_0_MU_0A0        = 269,
    SC_R_M4_0_MU_0A1        = 270,
    SC_R_M4_0_MU_0A2        = 271,
    SC_R_M4_0_MU_0A3        = 272,
    SC_R_M4_0_MU_1A         = 273,
    SC_R_M4_0_RGPIO_P0      = 274,
    SC_R_M4_1_PID0          = 275,
    SC_R_M4_1_PID1          = 276,
    SC_R_M4_1_PID2          = 277,
    SC_R_M4_1_PID3          = 278,
    SC_R_M4_1_PID4          = 279,
    SC_R_M4_1_RGPIO_P1      = 280,
    SC_R_M4_1_SEMA42        = 281,
    SC_R_M4_1_TPM           = 282,
    SC_R_M4_1_PIT           = 283,
    SC_R_M4_1_UART          = 284,
    SC_R_M4_1_I2C           = 285,
    SC_R_M4_1_INTMUX        = 286,
    SC_R_M4_1_SIM           = 287,
    SC_R_M4_1_WDOG          = 288,
    SC_R_M4_1_MU_0B         = 289,
    SC_R_M4_1_MU_0A0        = 290,
    SC_R_M4_1_MU_0A1        = 291,
    SC_R_M4_1_MU_0A2        = 292,
    SC_R_M4_1_MU_0A3        = 293,
    SC_R_M4_1_MU_1A         = 294,
    SC_R_M4_1_RGPIO_P0      = 295,
    SC_R_SAI_0              = 296,
    SC_R_SAI_1              = 297,
    SC_R_SAI_2              = 298,
    SC_R_SPBA               = 299,
    SC_R_USB_NIC            = 300,
    SC_R_USB_0              = 301,
    SC_R_USB_1              = 302,
    SC_R_USB_2              = 303,
    SC_R_SDHC_0             = 304,
    SC_R_SDHC_1             = 305,
    SC_R_SDHC_2             = 306,
    SC_R_QSPI_0             = 307,
    SC_R_ENET_0             = 308,
    SC_R_ENET_1             = 309,
    SC_R_SDMA               = 310,
    SC_R_SSI_MST_MW         = 311,
    SC_R_IRQSTR_MW          = 312,
    SC_R_SC_PID0            = 313,
    SC_R_SC_PID1            = 314,
    SC_R_SC_PID2            = 315,
    SC_R_SC_PID3            = 316,
    SC_R_SC_PID4            = 317,
    SC_R_SC_RGPIO_P1        = 318,
    SC_R_SC_SEMA42          = 319,
    SC_R_SC_TPM             = 320,
    SC_R_SC_PIT             = 321,
    SC_R_SC_UART            = 322,
    SC_R_SC_I2C             = 323,
    SC_R_SC_INTMUX          = 324,
    SC_R_SC_SIM             = 325,
    SC_R_SC_WDOG            = 326,
    SC_R_SC_MU_0B           = 327,
    SC_R_SC_MU_0A0          = 328,
    SC_R_SC_MU_0A1          = 329,
    SC_R_SC_MU_0A2          = 330,
    SC_R_SC_MU_0A3          = 331,
    SC_R_SC_MU_1A           = 332,
    SC_R_SYSCNT_CTRL        = 333,
    SC_R_SYSCNT_RD          = 334,
    SC_R_SYSCNT_CMP         = 335,
    SC_R_FLEXBUS            = 336,
    SC_R_SNVS               = 337,
    SC_R_OTP                = 338,
    SC_R_ADM                = 339,
    SC_R_SC_RGPIO_P0        = 340,
    SC_R_MSI                = 341,
    SC_R_DEBUG              = 342,
    SC_R_DC_0_BLIT0         = 343,
    SC_R_DC_0_BLIT1         = 344,
    SC_R_DC_0_BLIT2         = 345,
    SC_R_DC_0_BLIT_OUT      = 346,
    SC_R_DC_0_CAPTURE0      = 347,
    SC_R_DC_0_CAPTURE1      = 348,
    SC_R_DC_0_WARP          = 349,
    SC_R_DC_0_INTEGRAL0     = 350,
    SC_R_DC_0_INTEGRAL1     = 351,
    SC_R_DC_0_VIDEO0        = 352,
    SC_R_DC_0_VIDEO1        = 353,
    SC_R_DC_0_FRAC0         = 354,
    SC_R_DC_0_FRAC1         = 355,
    SC_R_DC_0               = 356,
    SC_R_GPU_2_PID0         = 357,
    SC_R_IRQSTR_DC_0        = 358,
    SC_R_DC_0_CRR           = 359,
    SC_R_DC_0_PXL_CMB       = 360,
    SC_R_DC_0_LPCG          = 361,
    SC_R_DC_1_BLIT0         = 362,
    SC_R_DC_1_BLIT1         = 363,
    SC_R_DC_1_BLIT2         = 364,
    SC_R_DC_1_BLIT_OUT      = 365,
    SC_R_DC_1_CAPTURE0      = 366,
    SC_R_DC_1_CAPTURE1      = 367,
    SC_R_DC_1_WARP          = 368,
    SC_R_DC_1_INTEGRAL0     = 369,
    SC_R_DC_1_INTEGRAL1     = 370,
    SC_R_DC_1_VIDEO0        = 371,
    SC_R_DC_1_VIDEO1        = 372,
    SC_R_DC_1_FRAC0         = 373,
    SC_R_DC_1_FRAC1         = 374,
    SC_R_DC_1               = 375,
    SC_R_GPU_3_PID0         = 376,
    SC_R_IRQSTR_DC_1        = 377,
    SC_R_DC_1_CRR           = 378,
    SC_R_DC_1_PXL_CMB       = 379,
    SC_R_DC_1_LPCG          = 380,
    SC_R_PCIE_A             = 381,
    SC_R_PCIE_PHY           = 382,
    SC_R_PCIE_F0            = 383,
    SC_R_PCIE_F1            = 384,
    SC_R_PCIE_F2            = 385,
    SC_R_PCIE_F3            = 386,
    SC_R_PCIE_F4            = 387,
    SC_R_PCIE_F5            = 388,
    SC_R_PCIE_F6            = 389,
    SC_R_PCIE_F7            = 390,
    SC_R_PCIE_F8            = 391,
    SC_R_PCIE_F9            = 392,
    SC_R_PCIE_F10           = 393,
    SC_R_PCIE_F11           = 394,
    SC_R_PCIE_F12           = 395,
    SC_R_PCIE_F13           = 396,
    SC_R_PCIE_F14           = 397,
    SC_R_PCIE_F15           = 398,
    SC_R_LCD_0              = 399,
    SC_R_LCD_0_PWM_0        = 400,
    SC_R_LCD_0_I2C_0        = 401,
    SC_R_LCD_0_I2C_1        = 402,
    SC_R_LVDS_0             = 403,
    SC_R_LVDS_0_PWM_0       = 404,
    SC_R_LVDS_0_I2C_0       = 405,
    SC_R_LVDS_0_I2C_1       = 406,
    SC_R_LVDS_1             = 407,
    SC_R_LVDS_1_PWM_0       = 408,
    SC_R_LVDS_1_I2C_0       = 409,
    SC_R_LVDS_1_I2C_1       = 410,
    SC_R_LVDS_2             = 411,
    SC_R_LVDS_2_PWM_0       = 412,
    SC_R_LVDS_2_I2C_0       = 413,
    SC_R_LVDS_2_I2C_1       = 414,
    SC_R_PI_0               = 415,
    SC_R_PI_0_PWM_0         = 416,
    SC_R_PI_0_PWM_1         = 417,
    SC_R_PI_0_I2C_0         = 418,
    SC_R_PI_1               = 419,
    SC_R_PI_1_PWM_0         = 420,
    SC_R_PI_1_PWM_1         = 421,
    SC_R_PI_1_I2C_0         = 422,
    SC_R_GPU_0_PID0         = 423,
    SC_R_GPU_0_PID1         = 424,
    SC_R_GPU_0_PID2         = 425,
    SC_R_GPU_0_PID3         = 426,
    SC_R_GPU_1_PID0         = 427,
    SC_R_GPU_1_PID1         = 428,
    SC_R_GPU_1_PID2         = 429,
    SC_R_GPU_1_PID3         = 430,
    SC_R_VPUCORE            = 431,
    SC_R_VPUCORE_0          = 432,
    SC_R_VPUCORE_1          = 433,
    SC_R_VPUCORE_2          = 434,
    SC_R_VPUCORE_3          = 435,
    SC_R_VPU_PID0           = 436,
    SC_R_VPU_PID1           = 437,
    SC_R_VPU_PID2           = 438,
    SC_R_VPU_PID3           = 439,
    SC_R_VPU_PID4           = 440,
    SC_R_VPU_PID5           = 441,
    SC_R_VPU_PID6           = 442,
    SC_R_VPU_PID7           = 443,
    SC_R_VPU_UART           = 444,
    SC_R_ROM_0              = 445,
    SC_R_OSPI_0             = 446,
    SC_R_OSPI_1             = 447,
    SC_R_IEE                = 448,
    SC_R_AUD_PLL_0          = 449,
    SC_R_DC_0_PLL_0         = 450,
    SC_R_DC_0_PLL_1         = 451,
    SC_R_DC_1_PLL_0         = 452,
    SC_R_DC_1_PLL_1         = 453,
    SC_R_RTC                = 454,
    SC_R_LAST   
} sc_rsrc_t;

/*!
 * This type is used to indicate a control.
 */
typedef enum sc_ctrl_e
{
    SC_C_OTG_LINESTATE0     = 0,
    SC_C_GPU_SINGLE_MODE    = 1,
    SC_C_GPU_ID             = 2,
    SC_C_LAST
} sc_ctrl_t;

/*!
 * This type is used to indicate a pin. Valid values are SoC specific.
 */
typedef uint16_t sc_pin_t;

/*!
 * This type is used to declare a handle for an IPC communication
 * channel. Its meaning is specific to the IPC implementation.
 */
typedef uint32_t sc_ipc_t;

/* Extra documentation of standard types */

#ifdef DOXYGEN
    /*! 
     * Type used to declare a true/false boolean.
     */
    typedef enum {false = 0, true = 1} bool;

    /*! 
     * Type used to declare an 8-bit integer.
     */
    typedef __INT8_TYPE__ int8_t;

    /*! 
     * Type used to declare a 16-bit integer.
     */
    typedef __INT16_TYPE__ int16_t;

    /*! 
     * Type used to declare a 32-bit integer.
     */
    typedef __INT32_TYPE__ int32_t;

    /*! 
     * Type used to declare a 64-bit integer.
     */
    typedef __INT64_TYPE__ int64_t;

    /*! 
     * Type used to declare an 8-bit unsigned integer.
     */
    typedef __UINT8_TYPE__ uint8_t;

    /*! 
     * Type used to declare a 16-bit unsigned integer.
     */
    typedef __UINT16_TYPE__ uint16_t;

    /*! 
     * Type used to declare a 32-bit unsigned integer.
     */
    typedef __UINT32_TYPE__ uint32_t;

    /*! 
     * Type used to declare a 64-bit unsigned integer.
     */
    typedef __UINT64_TYPE__ uint64_t;
#endif

#endif /* _SC_TYPES_H */

