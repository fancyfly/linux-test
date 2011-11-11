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

#include <linux/slab.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/mm.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <linux/kobject.h>
#include <linux/io.h>
#include <linux/kthread.h>
#include <linux/fb.h>
#include <linux/mxcfb.h>
#include <linux/ipu.h>

#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>

#include <linux/gpio.h>
#include <linux/delay.h>

#include "sii_92326_driver.h"
#include "sii_92326_api.h"

#include <mach/gpio.h>

#define	MHL_TIMER_BASE_DIVISOR			10		// 1ms MHL timer base divisor

//sbit pinMHLTxVbus_CTRL = P0^4;	// VDD 5V to MHL VBUS switch control

#define	APP_DEMO_RCP_SEND_KEY_CODE 0x41

extern mhlTx_AVSetting  mhlTxAv;

bool_t	vbusPowerState = true;		// false: 0 = vbus output on; true: 1 = vbus output off;

///////////////////////////////////////////////////////////////////////////////
//
// AppRcpDemo
//
// This function is supposed to provide a demo code to elicit how to call RCP
// API function.
//
void	AppRcpDemo( uint8_t event, uint8_t eventParameter)
{
	uint8_t		rcpKeyCode;

//	printk("App:%d Got event = %02X, eventParameter = %02X\n",(int)__LINE__, (int)event, (int)eventParameter);

	switch( event )
	{
		case	MHL_TX_EVENT_DISCONNECTION:
			TX_API_PRINT(("[MHL]App: Got event = MHL_TX_EVENT_DISCONNECTION\n"));
			break;

		case	MHL_TX_EVENT_CONNECTION:
			TX_API_PRINT(("[MHL]App: Got event = MHL_TX_EVENT_CONNECTION\n"));
			break;

		case	MHL_TX_EVENT_RCP_READY:
    			TX_API_PRINT(("[MHL]App: Got event = MHL_TX_EVENT_RCP_READY...\n"));

			if( (0 == (MHL_FEATURE_RCP_SUPPORT & eventParameter)) )
			{
				TX_API_PRINT(("[MHL]App: Peer does NOT support RCP\n"));
			}
            		else
            		{
				TX_API_PRINT(("[MHL]App: Peer supports RCP\n"));
			}
			if( (0 == (MHL_FEATURE_RAP_SUPPORT & eventParameter)) )
			{
				TX_API_PRINT(("[MHL]App: Peer does NOT support RAP\n"));
			}
            		else
            		{
				TX_API_PRINT(("[MHL]App: Peer supports RAP\n"));
			}
			if( (0 == (MHL_FEATURE_SP_SUPPORT & eventParameter)) )
			{
				TX_API_PRINT(("[MHL]App: Peer does NOT support WRITE_BURST\n"));
			}
            		else
			{
				TX_API_PRINT(("[MHL]App: Peer supports WRITE_BURST\n"));
			}

#if 0	//RCP key send demo in here
              	if (MHL_FEATURE_RCP_SUPPORT & eventParameter)
            		{
                		if (!MhlTxCBusBusy())
	                	{
	    				// Demo RCP key code Volume Up
	    				rcpKeyCode = APP_DEMO_RCP_SEND_KEY_CODE;

	        			TX_API_PRINT(("[MHL]App: Sending RCP (%02X)\n",(int) rcpKeyCode));
					//
					// If RCP engine is ready, send one code
					//
					if( SiiMhlTxRcpSend( rcpKeyCode ))
					{
	        				TX_API_PRINT(("[MHL]App: SiiMhlTxRcpSend (%02X)\n",(int) rcpKeyCode));
					}
					else
					{
	        				TX_API_PRINT(("[MHL]App: SiiMhlTxRcpSend (%02X) Returned Failure.\n",(int) rcpKeyCode));
	        			}
				}
			}
#endif
			break;

		case	MHL_TX_EVENT_RCP_RECEIVED:
			//
			// Check if we got an RCP. Application can perform the operation here
			// and send RCPK or RCPE. For now, we send the RCPK
			//
			rcpKeyCode = eventParameter;
			TX_API_PRINT(("[MHL]App: Received an RCP key code = %02X\n",(int)rcpKeyCode));
			SiiMhlTxRcpkSend(rcpKeyCode);

			// Added RCP key printf and interface with UI. //by oscar 20101217
		    	switch ( rcpKeyCode )
		    	{
		        	case MHL_RCP_CMD_SELECT:
					TX_API_PRINT(( "\n[MHL]App: Select received\n\n" ));
					break;
		        	case MHL_RCP_CMD_UP:
					TX_API_PRINT(( "\n[MHL]App: Up received\n\n" ));
					break;
		        	case MHL_RCP_CMD_DOWN:
					TX_API_PRINT(( "\n[MHL]App: Down received\n\n" ));
					break;
		        	case MHL_RCP_CMD_LEFT:
					TX_API_PRINT(( "\n[MHL]App: Left received\n\n" ));
					break;
		        	case MHL_RCP_CMD_RIGHT:
					TX_API_PRINT(( "\n[MHL]App: Right received\n\n" ));
					break;
		        	case MHL_RCP_CMD_ROOT_MENU:
					TX_API_PRINT(( "\n[MHL]App: Root Menu received\n\n" ));
					break;
		        	case MHL_RCP_CMD_EXIT:
					TX_API_PRINT(( "\n[MHL]App: Exit received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_0:
					TX_API_PRINT(( "\n[MHL]App: Number 0 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_1:
					TX_API_PRINT(( "\n[MHL]App: Number 1 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_2:
					TX_API_PRINT(( "\n[MHL]App: Number 2 received\n\n" ));
					break;	
		        	case MHL_RCP_CMD_NUM_3:
					TX_API_PRINT(( "\n[MHL]App: Number 3 received\n\n" ));
					break;	
		        	case MHL_RCP_CMD_NUM_4:
					TX_API_PRINT(( "\n[MHL]App: Number 4 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_5:
					TX_API_PRINT(( "\n[MHL]App: Number 5 received\n\n" ));
					break;	
		        	case MHL_RCP_CMD_NUM_6:
					TX_API_PRINT(( "\n[MHL]App: Number 6 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_7:
					TX_API_PRINT(( "\n[MHL]App: Number 7 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_8:
					TX_API_PRINT(( "\n[MHL]App: Number 8 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_NUM_9:
					TX_API_PRINT(( "\n[MHL]App: Number 9 received\n\n" ));
					break;
		        	case MHL_RCP_CMD_DOT:
					TX_API_PRINT(( "\n[MHL]App: Dot received\n\n" ));
					break;
		        	case MHL_RCP_CMD_ENTER:
					TX_API_PRINT(( "\n[MHL]App: Enter received\n\n" ));
					break;
		        	case MHL_RCP_CMD_CLEAR:
					TX_API_PRINT(( "\n[MHL]App: Clear received\n\n" ));
					break;
				case MHL_RCP_CMD_SOUND_SELECT:
					TX_API_PRINT(( "\n[MHL]App: Sound Select received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PLAY:
					TX_API_PRINT(( "\n[MHL]App: Play received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PAUSE:
					TX_API_PRINT(( "\n[MHL]App: Pause received\n\n" ));
					break;
		        	case MHL_RCP_CMD_STOP:
					TX_API_PRINT(( "\n[MHL]App: Stop received\n\n" ));
					break;
		        	case MHL_RCP_CMD_FAST_FWD:
					TX_API_PRINT(( "\n[MHL]App: Fastfwd received\n\n" ));
					break;
		        	case MHL_RCP_CMD_REWIND:
					TX_API_PRINT(( "\n[MHL]App: Rewind received\n\n" ));
					break;
				case MHL_RCP_CMD_EJECT:
					TX_API_PRINT(( "\n[MHL]App: Eject received\n\n" ));
					break;
				case MHL_RCP_CMD_FWD:
		 			TX_API_PRINT(( "\n[MHL]App: Forward received\n\n" ));
					break;
		   		case MHL_RCP_CMD_BKWD:
		   			TX_API_PRINT(( "\n[MHL]App: Backward received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PLAY_FUNC:
					TX_API_PRINT(( "\n[MHL]App: Play Function received\n\n" ));
					break;
		        	case MHL_RCP_CMD_PAUSE_PLAY_FUNC:
					TX_API_PRINT(( "\n[MHL]App: Pause_Play Function received\n\n" ));
					break;
		   		case MHL_RCP_CMD_STOP_FUNC:
		   			TX_API_PRINT(( "\n[MHL]App: Stop Function received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F1:
		   			TX_API_PRINT(( "\n[MHL]App: F1 received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F2:
		   			TX_API_PRINT(( "\n[MHL]App: F2 received\n\n" ));
					break;
			   	case MHL_RCP_CMD_F3:
		   			TX_API_PRINT(( "\n[MHL]App: F3 received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F4:
		   			TX_API_PRINT(( "\n[MHL]App: F4 received\n\n" ));
					break;
		   		case MHL_RCP_CMD_F5:
		   			TX_API_PRINT(( "\n[MHL]App: F5 received\n\n" ));
					break;
        			default:
        				break;
      			}
				
			break;

		case	MHL_TX_EVENT_RCPK_RECEIVED:
			TX_API_PRINT(("[MHL]App: Received an RCPK = %02X\n", (int)eventParameter));
			break;

		case	MHL_TX_EVENT_RCPE_RECEIVED:
			TX_API_PRINT(("[MHL]App: Received an RCPE = %02X\n", (int)eventParameter));
			break;

		default:
			break;
	}
}

#if (VBUS_POWER_CHK == ENABLE)
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
void	AppVbusControl( bool_t powerOn )
{
	if( powerOn )
	{
		//pinMHLTxVbus_CTRL = 1;
		MHLSinkOrDonglePowerStatusCheck();
		TX_API_PRINT(("[MHL]App: Peer's POW bit is set. Turn the VBUS power OFF here.\n"));
	}
	else
	{
		//pinMHLTxVbus_CTRL = 0;
		TX_API_PRINT(("[MHL]App: Peer's POW bit is cleared. Turn the VBUS power ON here.\n"));
	}
}
#endif



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
////////////////////////// Linux platform related //////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

//Debug test
#undef dev_info
#define dev_info dev_err
#define MHL_DRIVER_NAME "sii92326drv"

/***** public type definitions ***********************************************/

typedef struct {
	struct task_struct	*pTaskStruct;
	uint8_t				pendingEvent;		// event data wait for retrieval
	uint8_t				pendingEventData;	// by user mode application

} MHL_DRIVER_CONTEXT_T, *PMHL_DRIVER_CONTEXT_T;


/***** global variables ********************************************/

MHL_DRIVER_CONTEXT_T gDriverContext;

struct i2c_client *mhl_Sii92326_page0 = NULL;
struct i2c_client *mhl_Sii92326_page1 = NULL;
struct i2c_client *mhl_Sii92326_page2 = NULL;
struct i2c_client *mhl_Sii92326_cbus = NULL;
struct i2c_client *siiEDID = NULL;
struct i2c_client *siiSegEDID = NULL;
struct i2c_client *siiHDCP = NULL;

struct platform_data {
	void (*reset) (void);
};
static struct platform_data *Sii92326_plat_data;

//------------------------------------------------------------------------------
// Array of timer values
//------------------------------------------------------------------------------
uint16_t g_timerCounters[ TIMER_COUNT ];

uint16_t g_timerElapsed;
uint16_t g_elapsedTick;
uint16_t g_timerElapsedGranularity;

uint16_t g_timerElapsed1;
uint16_t g_elapsedTick1;
uint16_t g_timerElapsedGranularity1;

static struct timer_list  g_mhl_1ms_timer;

uint16_t Int_count=0;

static bool_t match_id(const struct i2c_device_id *id, const struct i2c_client *client)
{
	if (strcmp(client->name, id->name) == 0)
		return true;

	return false;
}

bool_t Sii92326_mhl_reset(void)
{
	gpio_direction_output(MX53_PCBA_MHL_RST_N, 1);	
	msleep(10);	
	gpio_direction_output(MX53_PCBA_MHL_RST_N, 0);	
	msleep(10);	
	gpio_direction_output(MX53_PCBA_MHL_RST_N, 1);	
	return true;
}

//------------------------------------------------------------------------------
// Function: TimerTickHandler
// Description:
//------------------------------------------------------------------------------
static void TimerTickHandler ( void )
{
    uint8_t i;
	
    //decrement all active timers in array

    for ( i = 0; i < TIMER_COUNT; i++ )
    {
        if ( g_timerCounters[ i ] > 0 )
        {
            g_timerCounters[ i ]--;
        }
    }
    g_elapsedTick++;
    if ( g_elapsedTick == g_timerElapsedGranularity )
    {
        g_timerElapsed++;
        g_elapsedTick = 0;
    }
    g_elapsedTick1++;
    if ( g_elapsedTick1 == g_timerElapsedGranularity1 )
    {
        g_timerElapsed1++;
        g_elapsedTick1 = 0;
    }

    g_mhl_1ms_timer.expires = jiffies + 1;
    add_timer(&g_mhl_1ms_timer);
}


//------------------------------------------------------------------------------
// Function: HalTimerInit
// Description:
//------------------------------------------------------------------------------
void HalTimerInit ( void )
{
    uint8_t i;

    //initializer timer counters in array
    for ( i = 0; i < TIMER_COUNT; i++ )
    {
        g_timerCounters[ i ] = 0;
    }
    g_timerElapsed  = 0;
    g_timerElapsed1 = 0;
    g_elapsedTick   = 0;
    g_elapsedTick1  = 0;
    g_timerElapsedGranularity   = 0;
    g_timerElapsedGranularity1  = 0;
}

//------------------------------------------------------------------------------
// Function:    HalTimerSet
// Description:
//------------------------------------------------------------------------------
void HalTimerSet (uint8_t index, uint16_t m_sec)
{
    switch (index)
    {
    	case ELAPSED_TIMER:
        	g_timerElapsedGranularity = (int) m_sec/MHL_TIMER_BASE_DIVISOR;
        	g_timerElapsed = 0;
        	g_elapsedTick = 0;
        	break;

    	case ELAPSED_TIMER1:
        	g_timerElapsedGranularity1 = (int) m_sec/MHL_TIMER_BASE_DIVISOR;
        	g_timerElapsed1 = 0;
        	g_elapsedTick1 = 0;
        	break;
        default:
        	g_timerCounters[index] = (int) m_sec/MHL_TIMER_BASE_DIVISOR;
        	break;
    }
}

//------------------------------------------------------------------------------
// Function:    HalTimerExpired
// Description: Returns > 0 if specified timer has expired.
//------------------------------------------------------------------------------
uint8_t HalTimerExpired (uint8_t timer)
{
    if (timer < TIMER_COUNT)
    {
        return(g_timerCounters[timer] == 0);
    }

    return(0);
}

//------------------------------------------------------------------------------
// Function:    HalTimerElapsed
// Description: Returns current timer tick.  Rollover depends on the
//              granularity specified in the SetTimer() call.
//------------------------------------------------------------------------------
uint16_t HalTimerElapsed ( uint8_t index )
{
    uint16_t elapsedTime;

    if ( index == ELAPSED_TIMER )
        elapsedTime = g_timerElapsed;
    else
        elapsedTime = g_timerElapsed1;
	
    return( elapsedTime );
}

//------------------------------------------------------------------------------
// Function:    HalTimerWait
// Description: Waits for the specified number of milliseconds, using timer 0.
//------------------------------------------------------------------------------
void HalTimerWait ( uint16_t ms )
{
	mdelay(ms);
}

//------------------------------------------------------------------------------

struct work_struct	*sii92326work;
static spinlock_t sii92326_lock = SPIN_LOCK_UNLOCKED;

static void work_queue(struct work_struct *work)
{		
	enable_irq(MX53_PCBA_MHL_INT);
	
	Int_count += 15;
	if (Int_count > 30)
		Int_count = 30;
}

static irqreturn_t Sii92326_mhl_interrupt(int irq, void *dev_id)
{
	unsigned long lock_flags = 0;	 
	disable_irq_nosync(irq);
	spin_lock_irqsave(&sii92326_lock, lock_flags);	
	//printk("The sii92326 interrupt handeler is working..\n");  
	//printk("The most of sii92326 interrupt work will be done by following tasklet..\n");  

	schedule_work(sii92326work);

	//printk("The sii92326 interrupt's top_half has been done and bottom_half will be processed..\n");  
	spin_unlock_irqrestore(&sii92326_lock, lock_flags);
	return IRQ_HANDLED;
}

static void SiI92326_mhl_loop(void)
{
	uint8_t	event;
	uint8_t	eventParameter;

	printk("%s EventThread starting up\n", MHL_DRIVER_NAME);

       while (true)
    	{
		/*
			Event loop
		*/
		//
		// Look for any events that might have occurred.
		//
		SiiMhlTxGetEvents( &event, &eventParameter );

		if( MHL_TX_EVENT_NONE != event )
		{
			AppRcpDemo( event, eventParameter);
		}

		mdelay(50);
    	}
}


/*****************************************************************************/
/**
 * @brief Start driver's event monitoring thread.
 *
 *****************************************************************************/
void StartEventThread(void)
{
	gDriverContext.pTaskStruct = kthread_run(SiI92326_mhl_loop,
											 &gDriverContext,
											 MHL_DRIVER_NAME);
}


/*****************************************************************************/
/**
 * @brief Stop driver's event monitoring thread.
 *
 *****************************************************************************/
void  StopEventThread(void)
{
	kthread_stop(gDriverContext.pTaskStruct);

}


static struct i2c_device_id mhl_Sii92326_idtable[] = {
	{"mhl_Sii92326_page0", 0},
	{"mhl_Sii92326_page1", 0},
	{"mhl_Sii92326_page2", 0},
	{"mhl_Sii92326_cbus", 0},
	{"siiEDID", 0},
	{"siiSegEDID", 0},
	{"siiHDCP", 0},
};

static void sii9232_poweron()
{	
	gpio_direction_output(MX53_PCBA_MHL_3V3_ON, 1);
	gpio_direction_output(MX53_PCBA_MHL_1V3_ON, 1);}

static void sii9232_poweroff()
{	
	gpio_direction_output(MX53_PCBA_MHL_3V3_ON, 0);
	gpio_direction_output(MX53_PCBA_MHL_1V3_ON, 0);
}

static void sii9232_setup(struct fb_info *fbi)
{
	mm_segment_t old_fs;
	unsigned int fmt;

	if (fbi->fbops->fb_ioctl) 
	{		
		old_fs = get_fs();
		set_fs(KERNEL_DS);
		fbi->fbops->fb_ioctl(fbi, MXCFB_GET_DIFMT, (unsigned long)&fmt);
		set_fs(old_fs);		
		if (fmt == IPU_PIX_FMT_VYU444) {			
			mhlTxAv.ColorSpace = YCBCR444;			
			printk("input color space YUV\n");		
		} 
		else {
			mhlTxAv.ColorSpace = RGB;
			printk("input color space RGB\n");
		}	

	}
	if (fbi->var.xres/16 == fbi->var.yres/9)
		mhlTxAv.AspectRatio = VMD_ASPECT_RATIO_16x9;
	else
		mhlTxAv.AspectRatio = VMD_ASPECT_RATIO_4x3;
}

static int sii9232_fb_event(struct notifier_block *nb, unsigned long val, void *v)
{	
	struct fb_event *event = v;	
	struct fb_info *fbi = event->info;	
	switch (val) {	
		case FB_EVENT_MODE_CHANGE:		
			sii9232_setup(fbi);		
			break;	
		case FB_EVENT_BLANK:		
			if (*((int *)event->data) == FB_BLANK_UNBLANK)			
				sii9232_poweron();		
			else			
				sii9232_poweroff();		
			break;	
	}	
	return 0;
}

static struct notifier_block nb = {	
	.notifier_call = sii9232_fb_event,
};

static int __init mhl_Sii92326_init(void)
{
	int ret = 0;
	// Announce on RS232c port.
	//
	printk("\n============================================\n");
	printk("SiI92326 Linux Driver V1.24 \n");
	printk("============================================\n");

	//
	// Initialize the registers as required. Setup firmware vars.
	//
	bool_t 	interruptDriven;
	uint8_t 	pollIntervalMs;
	
	if(false == Sii92326_mhl_reset())
		return -EIO;
	
	HalTimerInit ( );
	HalTimerSet (TIMER_POLLING, MONITORING_PERIOD);

	init_timer(&g_mhl_1ms_timer);
	g_mhl_1ms_timer.function = TimerTickHandler;
	g_mhl_1ms_timer.expires = jiffies + 10*HZ;
	add_timer(&g_mhl_1ms_timer);

	sii92326work = kmalloc(sizeof(*sii92326work), GFP_ATOMIC);
	INIT_WORK(sii92326work, work_queue); 

	SiiMhlTxInitialize( interruptDriven = true, pollIntervalMs = MONITORING_PERIOD);
	
	ret = request_irq(MX53_PCBA_MHL_INT, Sii92326_mhl_interrupt, IRQ_TYPE_LEVEL_LOW,
				  "SII9232_det", mhl_Sii92326_page0);
	if (ret){
		printk(KERN_INFO "%s:%d:Sii92326 interrupt failed\n", __func__,__LINE__);	
		free_irq(MX53_PCBA_MHL_INT, "SII9232_det");
	}
	else{
		enable_irq_wake(MX53_PCBA_MHL_INT);	
		//printk(KERN_INFO "%s:%d:Sii92326 interrupt successed\n", __func__,__LINE__);	
	}

	siMhlTx_VideoSel( HDMI_720P60 );	// assume video initialize to 720p60, here should be decided by AP
	siMhlTx_AudioSel( AFS_44K1 );	// assume audio initialize to 44.1K, here should be decided by AP
	
	StartEventThread();		/* begin monitoring for events */
	fb_register_client(&nb);
	return ret;
}

static void __exit mhl_Sii92326_exit(void)
{
	/*
	 * 1. Free IRQ.
	 * 2. Power off MHL
	 * 3. Release MHL pins
	 */
	free_irq(MX53_PCBA_MHL_INT, NULL);	
	fb_unregister_client(&nb);
	sii9232_poweroff();	/* Release HDMI pins */	
	return 0;
}

module_init(mhl_Sii92326_init);
module_exit(mhl_Sii92326_exit);

MODULE_VERSION("1.24");
MODULE_AUTHOR("gary <qiang.yuan@siliconimage.com>, Silicon image SZ office, Inc.");
MODULE_DESCRIPTION("sii92326 transmitter Linux driver");
MODULE_ALIAS("platform:MHL_sii92326");
