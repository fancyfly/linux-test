#include "../../../arch/arm/mach-mx5/mx51_pins.h"
#include "../../../arch/arm/mach-mx5/iomux.h"
////////////////touch I2C////////////////
#define UCHAR unsigned char
#define UINT8 unsigned char
#define UINT32 unsigned int
#define BYTE unsigned char
#define UINT16	unsigned short
#define BOOL bool
#define TRUE	true
#define FALSE   false
#define I2C_TRIES 3
#define I2C_CLK_STRETCH_LIMIT 500000 
void ConfigSDA(bool bOut);
void SetSDAHigh(void);
void SetSDALow(void);
void SetSCLHigh();
void SetSCLLow();
void IICStart(void);
void IICStop(void);
void IICSendByte(BYTE ch);
BYTE IICReceiveByte(void);
bool WaitAck(void);
void SendAck(void);
UINT32 GetSDA_In(void);

bool gpio_i2c_write(UCHAR slave_address, UINT8 reg_address, UCHAR * pData, UCHAR byLength)
{
    UCHAR *pData_temp = pData;
    bool  bRc;
    UINT8 i = 0;
    
    printk("++gpio_i2c_write!\r\n");
    // Set to Transmit and start to send.
    IICStart();

    // \u0161º¡?\u0161\u0161¡\u20ac¡é?\u0161ª¡ä\u0161®\u0161\u0160\u0161\u0161¡À?\u0160??¡\u20ac LSB: 1=R,0=W;  
    IICSendByte((slave_address << 1) & 0xFE); 
    bRc = WaitAck();
    if (bRc == false)
    {
        IICStop();
        printk("  ERROR! No ACK!\r\n");
        printk("--gpio_i2c_write!\r\n");
        return false;
    }

         IICSendByte((UCHAR)(reg_address & 0xFF));
         bRc = WaitAck();
         if (bRc == false)
         {
                   IICStop();
        printk("  ERROR! No ACK!\r\n");
        printk("--gpio_i2c_write!\r\n");
                   return false;
         }

         // ¡\u20ac¡é?\u0161??¡ä??¡Â\u0161ºy?Y
    for (i=0; i<byLength; i++)
    {
        IICSendByte( *pData_temp);
        bRc = WaitAck();
        if (bRc == false)
        {
            IICStop();
            printk("  ERROR! No ACK!\r\n");
            printk("--gpio_i2c_write!\r\n");
            return false;
        }
        pData_temp++;
    }
    //
    IICStop();
    //printk("--gpio_i2c_write!\r\n");
    return true;
}

bool I2CWriteForTimes(UCHAR slave_address, UINT8 reg_address,  UCHAR * pData, UCHAR byLength)
{
    bool  bRet = false;
    UCHAR Tries = I2C_TRIES;
//    printk("++I2CWriteForTimes!\r\n");

    do
    {
        bRet = gpio_i2c_write(slave_address, reg_address, pData, byLength);
        Tries--;
    }while((!bRet) && (Tries != 0));

//    printk("--I2CWriteForTimes!\r\n");
    return bRet;
}


void IICStart(void)
{
//    printk("++IICStart!\r\n");

    // ????SDA?aOutput
    ConfigSDA(true);
    // SDA?a??|\u0161???
    SetSDAHigh();
    udelay(36);	
 
    // SCL?a??|\u0161???
    SetSCLHigh();
    udelay(36);

    // 
    SetSDALow();
    udelay(36);
    SetSCLLow();
    //printk("--IICStart!\r\n");
}

void IICStop(void)
{
//    printk("++IICStop!\r\n");
    // SDA?????aoutput
    ConfigSDA(true);

    SetSCLLow();
    udelay(36);
    SetSDALow();
    udelay(36);
    SetSCLHigh();
    udelay(36);
    SetSDAHigh();
//    printk("--IICStop!\r\n");
}

bool WaitAck(void)
{
    UINT32  errtime = 20;
    bool    bRc = false;

//    printk("++WaitAck!\r\n");
    SetSCLLow();
    ConfigSDA(false);
    udelay(36);
    SetSCLHigh();

    // \u0160Ì\u0161\u0161¡äy\u0161®|¡äeD?o?
    while (--errtime)
    {
        if (!GetSDA_In())
        { 
            bRc = true;
//            printk(KERN_INFO"Got ACK!\n");
            break;
        }
    }
    SetSCLLow();

//    printk("--WaitAck!\r\n");
    return bRc;
}


void SendAck(void)
{
//    printk("++SendAck!\r\n");

         // ????SDA?a\u0161?3?¡ê?2¡é\u0161?3?\u0160Ì\u0161ª\u0160???
    SetSDALow();
    udelay(36);

         // \u0161?SCL\u0161?3???\u0160???
    SetSCLHigh();
    udelay(36);

         // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
    SetSCLLow();
    //printk("--SendAck!\r\n");
}

void SendNoAck(void)
{
//    printk("++SendNoAck!\r\n");
         // ????SDA?a\u0161?3?¡ê?2¡é\u0161?3???\u0160???
    SetSDAHigh();
    udelay(36);

         // \u0161?SCL\u0161?3???\u0160???
    SetSCLHigh();
    udelay(36);

         // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
    SetSCLLow();
//    printk("--SendNoAck!\r\n");
}

void IICSendByte(BYTE ch)
{
    BYTE i = 8;
//    printk("++IICSendByte!\r\n");

         // ????SDA?a\u0161?3?
    ConfigSDA(true); 
    // ?-?¡\u20ac8¡ä?¡ê???¡ä?¡\u20ac¡é?\u0161ª\u0161???\u0161ºy?Y
    while (i--)
    {
                   // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
        SetSCLLow();
        udelay(10);

        if (ch & 0x80)
        {
            SetSDAHigh();
        }
        else
        {
            SetSDALow();
        }
                   udelay(27);

                   // \u0161?SCL\u0161?3???\u0160???
                   SetSCLHigh();
        udelay(35);
                   // ??¡\u20ac¡é?\u0161ª\u0161ª\u0161º\u0161?bit??¡äy¡\u20ac¡é?\u0161ª\u0161ºy?Y¡Á\u0161®\u0161?\u0161???¡ê???¡äy¡\u20ac¡é?\u0161ªbit\u0161?\u0160?¡Á?????
                   ch <<= 1;
    }
         // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
    SetSCLLow();
    //printk("--IICSendByte!\r\n");
}


BYTE IICReceiveByte(void)
{
    BYTE i = 8;
    BYTE byData = 0;

//    printk("++IICReceiveByte!\r\n"); 

    // SDA?????ainput
    ConfigSDA(false);
    udelay(36); 

    // ?-?¡\u20ac8¡ä?¡ê???¡ä??\u0161®\u0161?\u0161???\u0161ºy?Y
    while (i--)
    {
      byData <<= 1;

                   // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
        SetSCLLow();
        udelay(40);
        SetSCLHigh();
        udelay(35);
		
        if (GetSDA_In())
        {
            byData |= 1;
        }
    }

         // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
    SetSCLLow();

//    printk("--IICReceiveByte!\r\n");

    return byData;
}

void ConfigSDA(bool bOut)
{
    if (bOut)
    {
        // output
        gpio_direction_output(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK), 0);
    }
    else
    {
        // input
        gpio_direction_input(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK));
    }
}


void ConfigSCL(void)
{
    gpio_direction_output(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO), 0);  
}

void SetSDA(bool bHiLow)
{
    gpio_direction_output(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK), 0);
    if (bHiLow)
    {
    gpio_set_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK), 1);
    }
    else
    {
		gpio_set_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK), 0);
    }
}

void SetSDAHigh()
{
    UINT32  dwData;
    UINT32  dwDelay = I2C_CLK_STRETCH_LIMIT;

    // ????SDA?a\u0161?\u0161\u0161?
	gpio_direction_input(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK));
    // \u0160Ì\u0161\u0161¡äySDA¡À?\u0161\u20ac-??
    do{
		dwData = gpio_get_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK));
        --dwDelay;
    }while(!dwData && dwDelay);
}

void SetSDALow()
{
    // ????SDA?a\u0161?3?¡ê?2¡é\u0161?3?\u0160Ì\u0161ª\u0160???   
   gpio_direction_output(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK), 0);
   gpio_set_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK), 0);
}


UINT32 GetSDA_In(void)
{
    UINT32 uData = 0;
	uData = gpio_get_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_CLK));
    return uData; //((uData == 1) ? true : false);
}


void SetSCL(bool bHiLow)
{
    gpio_direction_output(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO), 0);
    if (bHiLow)
    {
        gpio_set_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO), 1);
    }
    else
    {
        gpio_set_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO), 1);
    }
}

void SetSCLHigh()
{
    UINT32  dwData;
    UINT32  dwDelay = I2C_CLK_STRETCH_LIMIT;
    // ????SCL?a\u0161?\u0161\u0161?
    gpio_direction_input(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO));
    // \u0160Ì\u0161\u0161¡äySCL¡À?\u0161\u20ac-??
    do{
        dwData = gpio_get_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO));
        --dwDelay;
    }while(!dwData && dwDelay);
}

void SetSCLLow()
{
    // ????SCL?a\u0161?3?
    gpio_direction_output(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO), 0);  
    // \u0161?SCL\u0161?3?\u0160Ì\u0161ª\u0160???
    gpio_set_value(IOMUX_TO_GPIO(MX51_PIN_DISPB2_SER_DIO), 0);
}
bool gpio_i2c_read(UCHAR slave_address, UINT8 reg_address, UCHAR *pData, UCHAR byLength)
{
    UCHAR *pData_temp = pData;
    UINT8 i = 0;
    bool  bRc = true;
    UINT8 ReadData = 0;
    // First set to Transmit and start to send the slave address.
    IICStart();
 
    // \u0161º¡?\u0161\u0161¡\u20ac¡é?\u0161ª¡ä\u0161®\u0161\u0160\u0161\u0161¡À?\u0160??¡\u20ac LSB: 1=R¡ê?0=W
//	printk(" Send slave addr. 0x%02X\r\n", slave_address);
    IICSendByte((slave_address << 1) & 0xFE);
    bRc = WaitAck();
    if (bRc == false)
    {
        // ?TACK
        IICStop();
//        printk("  ERROR! No ACK!\r\n");
//        printk("--gpio_i2c_read!\r\n");
        return false;
    }

    // \u0161\u0161?o\u0161®¡\u20ac¡é?\u0161ª\u0161°a?\u0161¢\u0161\u0161?\u0161ºy?Y\u0160???¡ä??¡Â\u0160??¡\u20ac
//    printk(" Send reg addr. 0x%02X\r\n", reg_address);
    IICSendByte(reg_address);
    bRc = WaitAck();

    if (bRc == false)
    {
        IICStop();
//        printk("  ERROR! No ACK!\r\n");
        //printk("--gpio_i2c_read!\r\n");
        return false;
    }
    // Repeat Start
    IICStart();

    // ?\u0161\u017d¡ä?¡\u20ac¡é?\u0161ª¡ä\u0161®\u0161\u0160\u0161\u0161¡À?\u0160??¡\u20ac¡ê?¡Á?\u0160Ì\u0161??¡ê?1¡ê?¡À\u0161?¡Â????\u0160?2\u0161\u017d¡Á¡Â?a?\u0161?\u0161\u017d¡Á¡Â
    IICSendByte((slave_address << 1) | 0x1);
    bRc = WaitAck();
 
    if (bRc == false)
    {
        IICStop();
//        printk("  ERROR! No ACK!\r\n");
//        printk("--gpio_i2c_read!\r\n");
        return false;
    }
    for (i = byLength; i > 0; --i)
    {
        ReadData = IICReceiveByte();
//        printk(KERN_INFO"I2C data = 0x%x\n",ReadData);  
        if (i==1)
        {
            IICStop();
        }
        else
        {
            SendAck();
        }
        *pData_temp++ = ReadData;
    }
    return true;  
}

 

bool I2CReadForTimes(UCHAR slave_address, UINT8 reg_address,  UCHAR * pData, UCHAR byLength)
{
    bool  bRet = false;
    UCHAR Tries = I2C_TRIES;
//    printk("\r\n++I2CReadForTimes! \r\n  To read %d bytes\r\n", byLength);
    do
    {
        bRet = gpio_i2c_read(slave_address, reg_address, pData, byLength);
        Tries--;
    }while((!bRet) && (Tries != 0));

    if (!bRet)
        printk("    Read Failed!\r\n");

//    printk("--I2CReadForTimes!\r\n");
    return bRet;
}
//////////////////////////////////////////////////////
////////////////////////////////////////////////////

void gpio_i2c_test()
{
    unsigned char DataBuff[0x20] = {0};
    unsigned char DataBuffBack[0x20] = {0};
    UINT32 uPreInt = 1, uCurInt = 0,count = 0, len = 0x14,i;
    printk(KERN_INFO"************************%s\n",__func__);
    I2CWriteForTimes(0x2c,0xFF,DataBuff,1);
//    DataBuff[0] = 0x00; 
//    I2CWriteForTimes(0x2c,0x25,DataBuff,1);
    I2CReadForTimes(0x2c,0x14,DataBuff,len);
    for(i=0;i<len;i++)
    {
        printk(KERN_INFO "Addr(0x%x) = 0x%x\n",i+0x14,DataBuff[i]);
        DataBuffBack[i] = DataBuff[i];
    }
    do{
        uCurInt = gpio_get_value(IOMUX_TO_GPIO(MX51_PIN_DISP2_DAT9));
#if 0
        if(count++ > 0x100000)
        {
            count = 0;
            uPreInt = 1; 
        }
#endif
        //if(uPreInt != uCurInt)
        {
            if(uCurInt == 0)
            {
                printk(KERN_INFO"*****uCurInt = 0x%x\n",uCurInt);
                I2CReadForTimes(0x2c,0x14,DataBuff,len);
                for(i=0;i<len;i++)
                {
                    if(DataBuff[i] != DataBuffBack[i])
                    {
                        if(uPreInt == 1)
                        {
                            printk(KERN_INFO"**************************\n");
                            uPreInt = uCurInt;
                        }
                        DataBuffBack[i] = DataBuff[i];
                        printk(KERN_INFO "Addr(0x%x) = 0x%x\n",i+0x14,DataBuff[i]);
                    }
                }
            }
            uPreInt = uCurInt;
        }
    }while(1);
}
