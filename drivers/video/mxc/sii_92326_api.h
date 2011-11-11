/*
 * SiIxxxx <Firmware or Driver>
 *
 * Copyright (C) 2011 Silicon Image Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
 * PURPOSE.  See the
 * GNU General Public License for more details.
*/

//#include <stdio.h>
//#include <linux/types.h>

#ifndef __SII_92326_API_H__
#define __SII_92326_API_H__

    /* C99 defined data types.  */
/*
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;

typedef signed char    int8_t;
typedef signed short   int16_t;
typedef signed long    int32_t;

typedef enum
{
    false   = 0,
    true    = !(false)
} bool_t;
*/

typedef	bool	bool_t;

#define LOW                     0
#define HIGH                    1

// MHL Related
#define MX53_PCBA_MHL_RST_N		(0*32 + 29)	/* GPIO1_29 */
#define MX53_PCBA_MHL_1V3_ON		(2*32 + 21)	/* GPIO2_7 */
#define MX53_PCBA_MHL_WAKE		(1*32 + 26)	/* GPIO2_26 */
#define MX53_PCBA_MHL_INT		(4*32 + 0)	/* GPIO5_0 */
#define MX53_PCBA_MHL_3V3_ON	(6*32 + 1)	/* GPIO7_1 */
#define MX53_PCBA_MHL_SW_I2C_SCL	(5 * 32 + 8)	/* GPIO6_8 */
#define MX53_PCBA_MHL_SW_I2C_SDA	(5 * 32 + 10)	/* GPIO6_10 */

// Generic Masks
//==============
#define _ZERO		    0x00
#define BIT0                   0x01
#define BIT1                   0x02
#define BIT2                   0x04
#define BIT3                   0x08
#define BIT4                   0x10
#define BIT5                   0x20
#define BIT6                   0x40
#define BIT7                   0x80

// Timers - Target system uses these timers
#define ELAPSED_TIMER               0xFF
#define ELAPSED_TIMER1             0xFE	// For from discovery to MHL est timeout

typedef enum TimerId
{
    TIMER_FOR_MONITORING= 0,		// HalTimerWait() is implemented using busy waiting
    TIMER_POLLING,		// Reserved for main polling loop
    TIMER_2,			// Available
    TIMER_SWWA_WRITE_STAT,
    TIMER_TO_DO_RSEN_CHK,
    TIMER_TO_DO_RSEN_DEGLITCH,
    TIMER_COUNT			// MUST BE LAST!!!!
} timerId_t;

//
// This is the time in milliseconds we poll what we poll.
//
#define MONITORING_PERIOD		50

#define SiI_DEVICE_ID			0xB0

#define TX_HW_RESET_PERIOD	10

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Debug Definitions
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#define DISABLE 0x00
#define ENABLE  0xFF

// Compile debug prints inline or not
#define CONF__TX_API_PRINT   	(ENABLE)
#define CONF__TX_DEBUG_PRINT   (ENABLE)//(DISABLE)
#define CONF__TX_EDID_PRINT   	(DISABLE)

/*\
| | Debug Print Macro
| |
| | Note: TX_DEBUG_PRINT Requires double parenthesis
| | Example:  TX_DEBUG_PRINT(("hello, world!\n"));
\*/

#if (CONF__TX_DEBUG_PRINT == ENABLE)
    #define TX_DEBUG_PRINT(x)	printk x
#else
    #define TX_DEBUG_PRINT(x)
#endif

#if (CONF__TX_API_PRINT == ENABLE)
    #define TX_API_PRINT(x)	printk x
#else
    #define TX_API_PRINT(x)
#endif

#if (CONF__TX_EDID_PRINT == ENABLE)
    #define TX_EDID_PRINT(x)	printk x
#else
    #define TX_EDID_PRINT(x)
#endif

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

extern bool_t	vbusPowerState;
extern uint16_t Int_count;
//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------

extern uint16_t g_timerCounters[TIMER_COUNT];

extern uint16_t g_timerElapsed;
extern uint16_t g_elapsedTick;
extern uint16_t g_timerElapsedGranularity;

extern uint16_t g_timerElapsed1;
extern uint16_t g_elapsedTick1;
extern uint16_t g_timerElapsedGranularity1;

void 	HalTimerSet ( uint8_t index, uint16_t m_sec );
uint8_t 	HalTimerExpired ( uint8_t index );
void		HalTimerWait ( uint16_t m_sec );
uint16_t	HalTimerElapsed( uint8_t index );


///////////////////////////////////////////////////////////////////////////////
//
// AppMhlTxDisableInterrupts
//
// This function or macro is invoked from MhlTx driver to secure the processor
// before entering into a critical region.
//
// Application module must provide this function.
//
extern	void	AppMhlTxDisableInterrupts( void );


///////////////////////////////////////////////////////////////////////////////
//
// AppMhlTxRestoreInterrupts
//
// This function or macro is invoked from MhlTx driver to secure the processor
// before entering into a critical region.
//
// Application module must provide this function.
//
extern	void	AppMhlTxRestoreInterrupts( void );


///////////////////////////////////////////////////////////////////////////////
//
// AppVbusControl
//
// This function or macro is invoked from MhlTx driver to ask application to
// control the VBUS power. If powerOn is sent as non-zero, one should assume
// peer does not need power so quickly remove VBUS power.
//
// if value of "powerOn" is 0, then application must turn the VBUS power on
// within 50ms of this call to meet MHL specs timing.
//
// Application module must provide this function.
//
extern	void	AppVbusControl( bool_t powerOn );

extern 	bool_t Sii92326_mhl_reset(void);

#endif  // __SII_92326_API_H__
