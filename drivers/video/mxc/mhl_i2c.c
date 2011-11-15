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

#include <linux/wait.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <mach/hardware.h>
#include <mach/gpio.h>

#include "sii_92326_api.h"
#include "sii_92326_driver.h"

//------------------------------------------------------------------------------
#define I2C_DELAY_VALUE	5
//------------------------------------------------------------------------------
// Local macros to simplify bit operations
//------------------------------------------------------------------------------
bool GET_SDA(void)	{
	int rdVal;
	gpio_request(MX53_PCBA_MHL_SW_I2C_SDA, "mhl-sw-i2c-sda");
	rdVal = gpio_direction_input(MX53_PCBA_MHL_SW_I2C_SDA);
	gpio_free(MX53_PCBA_MHL_SW_I2C_SDA);
	return rdVal;
}

bool GET_SCL(void)	{
	int rdVal;
	gpio_request(MX53_PCBA_MHL_SW_I2C_SCL, "mhl-sw-i2c-scl");
	rdVal = gpio_direction_input(MX53_PCBA_MHL_SW_I2C_SCL);
	gpio_free(MX53_PCBA_MHL_SW_I2C_SCL);
	return rdVal;
}

void SET_SDA(void)	{
	int rdVal;
	gpio_request(MX53_PCBA_MHL_SW_I2C_SDA, "mhl-sw-i2c-sda");
	rdVal = gpio_direction_output(MX53_PCBA_MHL_SW_I2C_SDA, 1);
	udelay(I2C_DELAY_VALUE);
	gpio_free(MX53_PCBA_MHL_SW_I2C_SDA);
}

void SET_SCL(void)	{
	int rdVal;
	gpio_request(MX53_PCBA_MHL_SW_I2C_SCL, "mhl-sw-i2c-scl");
	rdVal = gpio_direction_output(MX53_PCBA_MHL_SW_I2C_SCL, 1);
	udelay(I2C_DELAY_VALUE);
	gpio_free(MX53_PCBA_MHL_SW_I2C_SCL);
}

void CLEAR_SDA(void)	{
	int rdVal;
	gpio_request(MX53_PCBA_MHL_SW_I2C_SDA, "mhl-sw-i2c-sda");
	rdVal = gpio_direction_output(MX53_PCBA_MHL_SW_I2C_SDA, 0);
	udelay(I2C_DELAY_VALUE);
	gpio_free(MX53_PCBA_MHL_SW_I2C_SDA);
}

void CLEAR_SCL(void)	{
	int rdVal;
	gpio_request(MX53_PCBA_MHL_SW_I2C_SCL, "mhl-sw-i2c-scl");
	rdVal = gpio_direction_output(MX53_PCBA_MHL_SW_I2C_SCL, 0);
	udelay(I2C_DELAY_VALUE);
	gpio_free(MX53_PCBA_MHL_SW_I2C_SCL);
}

//------------------------------------------------------------------------------
// Local constants (function parameters)
//------------------------------------------------------------------------------
#define READ   1
#define WRITE  0

#define LAST_BYTE      1
#define NOT_LAST_BYTE  0

#define IIC_OK 			 0
#define IIC_NOACK     		 1
#define IIC_SCL_TIMEOUT  2

//------------------------------------------------------------------------------
// Function: SetSCLHigh
// Description:
//------------------------------------------------------------------------------
static uint8_t SetSCLHigh(void)
{
    volatile uint8_t x = 0;    //delay variable
    uint16_t timeout = 0;
	
    // set SCL high, and wait for it to go high in case slave is clock stretching
    SET_SCL();
    x++;
    x++;
    x++;
    x++;
    while (!GET_SCL())
    {
        /* do nothing - just wait */
	mdelay(1);
	timeout++;
	if( timeout == 10 )   // about 1s is enough
	{
		printk("\n ************IIC SCL low timeout...\n");
		return IIC_SCL_TIMEOUT;
	}
    }
	
    return IIC_OK;
}

//------------------------------------------------------------------------------
// Function: SendByte
// Description:
//------------------------------------------------------------------------------
static uint8_t I2CSendByte(uint8_t abyte)
{
    uint8_t error = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)
    {
        if (abyte & 0x80)
            SET_SDA();      //send each bit, MSB first
        else
            CLEAR_SDA();

        if (SetSCLHigh())       //do clock cycle
            return IIC_SCL_TIMEOUT;
		
        CLEAR_SCL();

        abyte <<= 1;        //shift over to get next bit
    }

    SET_SDA();              //listen for ACK
    if (SetSCLHigh())
	return IIC_SCL_TIMEOUT;

    if (GET_SDA())          //error if no ACK
        error = IIC_NOACK;

    CLEAR_SCL();

    return (error);     //return 0 for no ACK, 1 for ACK received
}


//------------------------------------------------------------------------------
// Function: SendAddr
// Description:
//------------------------------------------------------------------------------
static uint8_t I2CSendAddr(uint8_t addr, uint8_t read)
{
    volatile uint8_t x = 0;    //delay variable

    //generate START condition
    SET_SCL();
    x++;            //short delay to keep setup times in spec
    CLEAR_SDA();
    x++;
    x++;
    x++;
    CLEAR_SCL();
    x++;

    return (I2CSendByte(addr|read));  //send address uint8_t with read/write bit
}

static uint8_t I2CGetByte(uint8_t lastbyte, uint8_t * Data)
{
    uint8_t abyte = 0;
    uint8_t i;

    for (i = 0; i < 8; i++)  //get each bit, MSB first
    {
        if (SetSCLHigh())
	     return IIC_SCL_TIMEOUT;
		
        abyte <<= 1;    //shift result over to make room for next bit

        if (GET_SDA())
            abyte++;    //same as 'abyte |= 1', only faster

        CLEAR_SCL();
    }

    if (lastbyte)
        SET_SDA();      //do not ACK last uint8_t read
    else
        CLEAR_SDA();

    if (SetSCLHigh())
	 return IIC_SCL_TIMEOUT;
	
    CLEAR_SCL();
    SET_SDA();
	
    *Data = abyte;
		
    return IIC_OK;
}

//------------------------------------------------------------------------------
// Function: SendStop
// Description:
//------------------------------------------------------------------------------
static uint8_t I2CSendStop(void)
{
    CLEAR_SDA();
    if (SetSCLHigh())
	return IIC_SCL_TIMEOUT;
	
    SET_SDA();
    return IIC_OK;
}

//-------------------------------------------------------------------
uint8_t I2C_ReadByte ( uint8_t SlaveAddr, uint8_t RegAddr )
{
	 uint8_t Data = 0;

        I2CSendAddr(SlaveAddr,WRITE);
        I2CSendByte(RegAddr);
        I2CSendAddr (SlaveAddr,READ);
	 I2CGetByte(LAST_BYTE, &Data);
        I2CSendStop();
        return Data;
}

//-------------------------------------------------------------------
void I2C_WriteByte ( uint8_t SlaveAddr, uint8_t RegAddr, uint8_t Data )
{
        I2CSendAddr(SlaveAddr,WRITE);
        I2CSendByte(RegAddr);
        I2CSendByte(Data);
        I2CSendStop();
}

//------------------------------------------------------------------------------
// Function Name: I2C_ReadBlock
// Function Description: Reads block of data from I2C Device
//------------------------------------------------------------------------------
uint8_t I2C_ReadBlock( uint8_t SlaveAddr, uint8_t RegAddr, uint8_t NBytes, uint8_t * Data )
{
	uint8_t i, bState;

       bState = I2CSendAddr(SlaveAddr,WRITE);
       if(bState){
       	I2CSendStop();
        	return IIC_NOACK;
     	}
	
	bState = I2CSendByte(RegAddr);
  	if(bState){
    		I2CSendStop();
     		return IIC_NOACK;
	}

	bState = I2CSendAddr (SlaveAddr,READ);
       if(bState){
		I2CSendStop();
      		return IIC_NOACK;
	}

    	for (i = 0; i < NBytes - 1; i++)
   		if (I2CGetByte(NOT_LAST_BYTE, &Data[i]))
			return IIC_SCL_TIMEOUT;

        if (I2CGetByte(LAST_BYTE, &Data[i]))
		return IIC_SCL_TIMEOUT;
		
        I2CSendStop();
     
        return IIC_OK;
}

//------------------------------------------------------------------------------
// Function Name:  I2C_WriteBlock
// Function Description: Writes block of data from I2C Device
//------------------------------------------------------------------------------
uint8_t I2C_WriteBlock( uint8_t SlaveAddr, uint8_t RegAddr, uint8_t NBytes, uint8_t * Data )
{
	uint8_t i, bState;

	bState = I2CSendAddr (SlaveAddr,WRITE);
 	if( bState ){
		I2CSendStop();
		return IIC_NOACK;
	}
	
	bState = I2CSendByte(RegAddr);
	if(bState){
		I2CSendStop();
		return IIC_NOACK;
	}

	for (i=0; i<NBytes; i++)
		I2CSendByte(Data[i]);

	I2CSendStop();
	
	return IIC_OK;

}

//------------------------------------------------------------------------------
// Function Name:  I2C_ReadSegmentBlockEDID
// Function Description: Reads segment block of EDID from HDMI Downstream Device
//------------------------------------------------------------------------------
uint8_t I2C_ReadSegmentBlockEDID(uint8_t SlaveAddr, uint8_t Segment, uint8_t Offset, uint8_t *Buffer, uint8_t Length)
{
	uint8_t i, bState;

	bState = I2CSendAddr(EDID_SEG_ADDR, WRITE);
       if(bState){
       	I2CSendStop();
        	return IIC_NOACK;
     	}
	   
	bState = I2CSendByte(Segment);
       if(bState){
       	I2CSendStop();
        	return IIC_NOACK;
     	}
	
       bState = I2CSendAddr(SlaveAddr,WRITE);
       if(bState){
       	I2CSendStop();
        	return IIC_NOACK;
     	}

	bState = I2CSendByte(Offset);
  	if(bState){
    		I2CSendStop();
     		return IIC_NOACK;
	}

	bState = I2CSendAddr (SlaveAddr,READ);
       if(bState){
		I2CSendStop();
      		return IIC_NOACK;
	}

    	for (i = 0; i < Length - 1; i++)
   		if (I2CGetByte(NOT_LAST_BYTE, &Buffer[i]))
			return IIC_SCL_TIMEOUT;

        if (I2CGetByte(LAST_BYTE, &Buffer[i]))
		return IIC_SCL_TIMEOUT;
		
        I2CSendStop();
     
        return IIC_OK;
}
