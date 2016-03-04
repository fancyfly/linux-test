/*
 * Copyright (c) 2012-2015, Freescale Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of Freescale Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


///////////////////////////////////////////////////////////////////////////////
//
// loader.c
//
// Description: 
//
//	firmware image loader
// 
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////

#include "czvavlib.h"
#include "zoe_dbg.h"
#include "zoe_sosal.h"
#ifdef ZOE_LINUXKER_BUILD
#include <linux/string.h>
#else //!ZOE_LINUXKER_BUILD
#include <string.h>
#endif //ZOE_LINUXKER_BUILD


#define DMA_BUFFER_MODE                     DMA_BUFFER_MODE_KERNEL/*DMA_BUFFER_MODE_COMMON*/


/////////////////////////////////////////////////////////////////////////////
//
//
// zn200
//

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

#define SFW_DISABLE_LOAD_FIRMWARE           0xABBAF001

#define HOST_HANDSHAKE_LOAD_SEC_BOOTCODE	0xa0000001
#define HOST_HANDSHAKE_LOAD_FIRMWARE        0xa0000004
#define HOST_RESPONSE_LOAD_COMPLETE         0xc0000001
#define ZN200_READY_FOR_OPERATION			0xabcd5a5a

#define SYS_HW_RESETS_DONE_INDICATOR        (0x608)
#define ZSYS_USB_DEVICE_ACTIVE              (0xE50)


#define HPU_CPU_ID                          0
#define SPU_CPU_ID                          1
#define DMA_CPU_ID                          2
#define AUD0_CPU_ID                         3
#define AUD1_CPU_ID                         4
#define ED_CPU_ID                           7
#define EE_CPU_ID                           8
#define ME_CPU_ID                           9
#define MAX_CPU_ID                          10

#define XREG_BASE                           0x70000000

static uint32_t nios_xreg_int_out_address[] = 
{
    (XREG_BASE + 0x02300000 + 0x0040),
    (XREG_BASE + 0x00000000 + 0x5000),
    (XREG_BASE + 0x02200000 + 0x5000),
    (XREG_BASE + 0x02800000 + 0x5000),
    (XREG_BASE + 0x02800000 + 0x45000),
    0,
    0,
    (XREG_BASE + 0x04000000 + 0x95000),
    (XREG_BASE + 0x06000000 + 0x95000),
    (XREG_BASE + 0x02000000 + 0x5000),

};

#define CLKRST_SCRATCH_1_ADDR               (0x71800000 + 0x304)
#define CLKRST_SCRATCH_2_ADDR               (0x71800000 + 0x308)
#define PAUSE_WORD_UPPER                    (0x1a320000)

void generate_halt_interrupt(c_zv_av_lib *This, 
                             uint32_t cpus
                             )
{
	uint32_t    i, cpus_to_gen = cpus;

    ZOEHAL_REG_WRITE(&This->m_iHal, CLKRST_SCRATCH_1_ADDR, PAUSE_WORD_UPPER | 0xFF);

    for (i = 0; i < 10; i++, cpus_to_gen >>= 1)
	{
		if (cpus_to_gen & 1)
		{
            ZOEHAL_REG_WRITE(&This->m_iHal, (nios_xreg_int_out_address[ED_CPU_ID] + 4 * i), 1);
		}
	}
    zoe_sosal_thread_sleep_ms(500);
	ZOEHAL_REG_WRITE(&This->m_iHal, CLKRST_SCRATCH_1_ADDR, 0); 
}


/////////////////////////////////////////////////////////////////////////////
//
//
// FPGA
//

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

#include "zoe_xreg.h"

// Simple MIPS 'parking' code that puts the PU in an infinite loop
// incrementing a count location at every pass.
const uint32_t MIPSILoopCode[] = 
{
	//0x1000ffff,			// b		.
	0x04110002,			// bal		<.+0xc> (ra <- &count)
	0x00000000,			// nop
	0x00000000,			// (count storage)
	0x8ff00000, 		// lw		s0,0(ra)
	0x26100001, 		// addiu	s0,s0,1
	0x1000fffd, 		// b		<.-0xc>
	0xaff00000 			// sw		s0,0(ra) (delay slot)
};


//
// Write to a given memory area a parking code sequence from the wordsA array.
// The given address is a physical one.
//
static zoe_errs_t setParkingCode(c_zv_av_lib *This, 
                                 zoe_dev_mem_t addr, 
                                 const uint32_t * wordsA, 
                                 int words
                                 )
{
	zoe_errs_t  err;

    err = ZOEHAL_MEM_WRITE_EX(&This->m_iHal, 
                              addr, 
                              (uint8_t *)wordsA, 
                              words * sizeof(uint32_t), 
                              ZOE_TRUE
                              );

    if (ZOE_FAIL(err))
    {
	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "%s() addr(0x%x) size(%d) err(%d)!!!!\n",
                       __FUNCTION__,
                       addr,
                       words * 4,
                       err
		               );
    }

	zoe_sosal_thread_sleep_ms(10);
    return (err);
}



void ZOEHAL_REG_WRITE_SYNC(IZOEHALAPI * This, 
                           uint32_t dw_reg, 
                           uint32_t data
                           )
{
    uint32_t    reg;

	ZOEHAL_REG_WRITE(This, dw_reg, data);
	zoe_sosal_thread_sleep_ms(1);
	ZOEHAL_REG_READ(This, dw_reg, &reg);

	zoe_dbg_printf_nc_e("%s() (0x%x) <= (0x%x) read(0x%x)\n",
                        __FUNCTION__,
                        dw_reg,
                        data,
                        reg
	                    );
}



static void CPUStopSPU(c_zv_av_lib *This)
{
	// disable SPU TOP modules
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_BLK_CTRL + MED_SPU_BLK_CTRL_SPU_RESET_CLR, 0x1 /*0x18*/);
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_BLK_CTRL + MED_SPU_BLK_CTRL_SPU_CLOCK_ENABLE_CLR, 0x1 /*0x18*/);
	// Put SPU TOP in reset
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + CLKRST_XREG_SLV_BASE + MED_CLKRST_CLK_AND_RST + MED_CLKRST_CLK_AND_RST_BLOCK_RESET_CLR, 1<<0);
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + CLKRST_XREG_SLV_BASE + MED_CLKRST_CLK_AND_RST + MED_CLKRST_CLK_AND_RST_CLOCK_ENABLE_CLR, 1<<0);
	zoe_sosal_thread_sleep_ms(10);
}



static void CPUStartSPU(c_zv_av_lib *This,
                        uint32_t resetAddr
                        )
{
	// Enable SPU TOP block
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + CLKRST_XREG_SLV_BASE + MED_CLKRST_CLK_AND_RST + MED_CLKRST_CLK_AND_RST_CLOCK_ENABLE_SET, 1<<0);
	// Take SPU TOP out of reset
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + CLKRST_XREG_SLV_BASE + MED_CLKRST_CLK_AND_RST + MED_CLKRST_CLK_AND_RST_BLOCK_RESET_SET, 1<<0);
	zoe_sosal_thread_sleep_ms(1);

	// enable SPU TOP modules
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_BLK_CTRL + MED_SPU_BLK_CTRL_SPU_CLOCK_ENABLE_SET, 0x5E);
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_BLK_CTRL + MED_SPU_BLK_CTRL_SPU_RESET_SET, 0x5E);
	zoe_sosal_thread_sleep_ms(1);

	// set reset vector
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_CPU_REGS + MED_SPU_CPU_REGS_BOOT_ADDRESS, (uint32_t)resetAddr);
	zoe_sosal_thread_sleep_ms(1);

	// release SPU (spu_cpu)
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_BLK_CTRL + MED_SPU_BLK_CTRL_SPU_CLOCK_ENABLE_SET, (1<<0));
	ZOEHAL_REG_WRITE_SYNC(&This->m_iHal, XREG_ARB_BASE_PHY + SPU_XREG_SLV_BASE + MED_SPU_SPU_BLK_CTRL + MED_SPU_BLK_CTRL_SPU_RESET_SET, (1<<0));

	zoe_sosal_thread_sleep_ms(10);
}



/////////////////////////////////////////////////////////////////////////////
//
//
// MX8
//

#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

typedef struct 
{
	uint32_t	jump;				// instruction to jump past this ID structure
	uint32_t	delaySlot;			// MIPS needs a nop after a jump
	uint32_t	signature;			// fourCC code with check value (ZOE_IMAGE_SIGNATURE)
	uint8_t		chipID;				// code to identify the target chip
	uint8_t		chipVarID;			// code to identify the target chip variant
	uint16_t	osID;				// code to identify the target OS
	uint32_t	puset;				// bitmap of PUs in this target
	uint32_t	part_size4k;		// size in 4kB pages used by the firmware partition (code, data, heap, ...)
	// Make sure these are 64-bit aligned
	uint64_t	link_addr;			// virtual address at which the binary image has been linked
	uint64_t	load_addr;			// phy address at which the binary image has been loaded
	uint64_t	move_addr;			// phy address at which the binary image has been moved
    char		strings[1];			// NULL-terminated strings with application name and revision numbers
} zoe_image_start_t, *zoe_image_start_ptr_t;


#ifdef _NO_SCU_API

typedef enum sc_pm_power_mode_e
{
    SC_PM_PW_MODE_OFF           = 0,    //!< Power off
    SC_PM_PW_MODE_STBY          = 1,    //!< Power in standby
    SC_PM_PW_MODE_LP            = 2,    //!< Power in low-power
    SC_PM_PW_MODE_ON            = 3     //!< Power on
} sc_pm_power_mode_t;



static int setCorePwr(c_zv_av_lib *This, 
                      uint32_t coreNum, 
                      sc_pm_power_mode_t pm
                      )
{
    return (0);
}


static int start_fwpu(c_zv_av_lib *This, 
                      uint64_t zfw_start
                      )
{
    return (0);
}

#else //!_NO_SCU_API

#define ARM_BOOT_ADDR   0x80000000

const uint32_t ARM64JumpToCode[] = 
{
    0x58000040,		//     ldr	x0, jumpTo
    0xd61f0000,		//     br	x0
			// jumpTo:
    0x00000000,		//     .quad	addressToJumpTo
    0x00000000
};


//
// Changes the power mode of coreNum to pm
// Return 0 if there were no errors
//
static int setCorePwr(c_zv_av_lib *This, 
                      uint32_t coreNum, 
                      sc_pm_power_mode_t pm
                      )
{
	int			rv = -1;
    sc_err_t 	sciErr;

    if (!This->m_ipcHndl)
    {
        zoe_dbg_printf_nc_e("--- setCorePwr no IPC handle\n");
        goto setCorePwrExit;
    }

    sciErr = sc_pm_set_resource_power_mode(This->m_ipcHndl, SC_R_VPUCORE_0 + coreNum, pm);
    if (sciErr != SC_ERR_NONE) 
    {
        zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPUCORE_%lu,%d) SCI error! (%d)\n", coreNum, pm, sciErr);
        goto setCorePwrExit;
    }

    rv = 0;

setCorePwrExit:
    return (rv);
}


//
// Return the power mode of core coreNum
// Return <0 if there were errors
//
static sc_pm_power_mode_t getCorePwr(sc_ipc_t ipcHndl, 
                                     uint32_t coreNum
                                     )
{
	sc_pm_power_mode_t	rv = -1;
    sc_err_t 	sciErr;

    if (!ipcHndl)
    {
        zoe_dbg_printf_nc_e("--- getCorePwr no IPC handle\n");
        return (rv);
    }

    sciErr = sc_pm_get_resource_power_mode(ipcHndl, SC_R_VPUCORE_0 + coreNum, &rv);
    if (sciErr != SC_ERR_NONE) 
    {
        zoe_dbg_printf_nc_e("--- sc_pm_get_resource_power_mode(SC_R_VPUCORE_%lu) SCI error! (%d)\n", coreNum, sciErr);
        rv = -1;
    }
	return (rv);
}


//
// Changes the power mode of the VPU to pm
// Return 0 if there were no errors
//
static int setVPUPwr(sc_ipc_t ipcHndl, 
                     sc_pm_power_mode_t pm
                     )
{
	int			rv = -1;
    sc_err_t 	sciErr;

    if (!ipcHndl)
    {
        zoe_dbg_printf_nc_e("--- setVPUPwr no IPC handle\n");
		goto setVPUPwrExit;
    }

	// Power on PID0-7, VPUCORE, UART
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID0, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID0,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID1, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID1,%d) SCI error! (%d)\n", sciErr,pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl,SC_R_VPU_PID2, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID2,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID3, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID3,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID4, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID4,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID5, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID5,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID6, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID6,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
	sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_PID7, pm);
	if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_PID7,%d) SCI error! (%d)\n", sciErr, pm);
		goto setVPUPwrExit;
	}
    sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPUCORE, pm);
    if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPUCORE,%d) SCI error! (%d)\n", sciErr, pm);
        goto setVPUPwrExit;
    }
    sciErr = sc_pm_set_resource_power_mode(ipcHndl, SC_R_VPU_UART, pm);
    if (sciErr != SC_ERR_NONE) 
    {
		zoe_dbg_printf_nc_e("--- sc_pm_set_resource_power_mode(SC_R_VPU_UART,%d) SCI error! (%d)\n", sciErr, pm);
        goto setVPUPwrExit;
    }

    rv = 0;

setVPUPwrExit:
	return (rv);
}



//
// Return the power mode of the VPU
// Return <0 if there were errors
//
static sc_pm_power_mode_t getVPUPwr(sc_ipc_t ipcHndl)
{
	sc_pm_power_mode_t	rv = -1;
    sc_err_t 	        sciErr;

    if (!ipcHndl)
    {
        zoe_dbg_printf_nc_e("--- getVPUPwr no IPC handle\n");
        return (rv);
    }

    sciErr = sc_pm_get_resource_power_mode(ipcHndl, SC_R_VPU_PID0, &rv);
    if (sciErr != SC_ERR_NONE) 
    {
        zoe_dbg_printf_nc_e("--- sc_pm_get_resource_power_mode(SC_R_VPU_PID0) SCI error! (%d)\n", sciErr);
        rv = -1;
    }

	return (rv);
}



//
// start the FWPU
// take the given core out of reset and run the firmware image
// Return 0 if there were no errors
//
static int start_fwpu(c_zv_av_lib *This, 
                      uint64_t zfw_start
                      )
{
	int						rv = -1;
	uint32_t				coreNum = 0;
    sc_err_t				sciErr;
    sc_pm_power_mode_t		pwrMode_vpu;
    sc_pm_power_mode_t		pwrMode_core[4];

    if (!This->m_ipcHndl)
    {
        zoe_dbg_printf_nc_e("--- start_fwpu no IPC handle\n");
        goto start_fwpuExit;
    }

    pwrMode_vpu = getVPUPwr(This->m_ipcHndl);
    if (pwrMode_vpu < 0) 
    {
        goto start_fwpuExit;
    }
    
    if (pwrMode_vpu != SC_PM_PW_MODE_ON) 
    {
		zoe_dbg_printf_nc_e("--- VPU is powered off; use 'vpu start' to turn it on\n");
		goto start_fwpuExit;
	}

    // Check if it is powered on
	for (coreNum = 0; coreNum < 4; coreNum++) 
    {
        pwrMode_core[coreNum] = getCorePwr(This->m_ipcHndl, coreNum);
		if (pwrMode_core[coreNum] < 0) 
        {
			goto start_fwpuExit;
		}
	}

    // Check if any core is already powered on
	for (coreNum = 0; coreNum < 4; coreNum++) 
    {
	    if (pwrMode_core[coreNum] == SC_PM_PW_MODE_ON) 
        {
	        zoe_dbg_printf_nc_e("+++ VPU core %u already ON\n", coreNum);
			goto start_fwpuExit;
	    }
	}

	// Power up the cores
	for (coreNum = 0; coreNum < 4; coreNum++)
    {
		(void)setCorePwr(This, coreNum, SC_PM_PW_MODE_ON);
    }

    ZOEHAL_MEM_WRITE(&This->m_iHal, ARM_BOOT_ADDR, ARM64JumpToCode[0], ZOE_FALSE);
    ZOEHAL_MEM_WRITE(&This->m_iHal, ARM_BOOT_ADDR + 4, ARM64JumpToCode[1], ZOE_FALSE);
    ZOEHAL_MEM_WRITE_EX(&This->m_iHal, ARM_BOOT_ADDR + 8, (uint8_t *)&zfw_start, sizeof(uint64_t), ZOE_FALSE);
	asm volatile("dsb sy");

    // Start the cores
	for (coreNum = 0; coreNum < 4; coreNum++) 
    {
		sciErr = sc_pm_cpu_start(This->m_ipcHndl, SC_R_VPUCORE_0 + coreNum, 1, (sc_faddr_t)ARM_BOOT_ADDR);
		if (sciErr != SC_ERR_NONE) 
        {
			zoe_dbg_printf_nc_e("--- sc_pm_cpu_start(SC_R_VPUCORE_%lu,on) SCI error! (%d)\n", coreNum, sciErr);
			goto start_fwpuExit;
		}
	}

	rv = 0;

start_fwpuExit:
	return (rv);
}

#endif //_NO_SCU_API




/////////////////////////////////////////////////////////////////////////////
//
//
// ???
//
#else //ZOE_TARGET_CHIP == ???
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP

/////////////////////////////////////////////////////////////////////////////
//
//

static uint32_t c_zv_av_lib_load_bin(c_zv_av_lib *This,
                                     uint8_t * pBin, 
                                     uint32_t lenBin,
                                     uint32_t load_addr
                                     )
{
    uint32_t    write_size;
    uint32_t    remain = lenBin;
    uint32_t    offset = 0;
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
    uint32_t    i;
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL
	zoe_errs_t	err;
//    uint32_t    max_dma = ZOEHAL_GetMaxDMASize(&This->m_iHal);
    uint32_t    max_dma = (64 * 1024); //ZOEHAL_GetMaxDMASize(&This->m_iHal);

        
	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
		           "image(0x%x), size(%d), load_addr(0x%x) max_dma(0x%x)\n",
                   pBin,
                   lenBin,
                   load_addr,
                   max_dma
		           );

    while (remain)
    {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
        if (load_addr & 0x70000000)
        {
            write_size = remain;

            for (i = 0; i < write_size; i += 4)
            {
                ZOEHAL_REG_WRITE(&This->m_iHal, load_addr + i, *((uint32_t *)(pBin + offset + i)));
            }
        }
        else
        {
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL
            write_size = ZOE_MIN(max_dma, remain);
#ifdef USE_MEMWR_4_DMA
            err = ZOEHAL_MEM_WRITE_EX(&This->m_iHal, 
                                      load_addr, 
                                      pBin + offset, 
                                      write_size, 
                                      ZOE_TRUE
                                      );
#else //!USE_MEMWR_4_DMA
            err = c_zv_codec_dma_write(This->m_pZVCodec, 
                                       load_addr, 
                                       pBin + offset, 
                                       write_size, 
                                       ZOE_FALSE,
                                       ZOE_TRUE,
                                       DMA_BUFFER_MODE, 
                                       ZOE_NULL
                                       );
#endif //USE_MEMWR_4_DMA
            if (!ZOE_SUCCESS(err))
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zv_codec_dma_write failed err(%d)!!!\n",
                               err
			                   );
                return (0);
            }
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
        }
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL
        remain -= write_size;
        load_addr += write_size;
        offset += write_size;
    }
    return (offset);
}


#define MAX_READ_SIZE   (64 * 1024)
#define MAX_VERIFY_SIZE (4 * 1024)


static zoe_bool_t c_zv_av_lib_verify_bin(c_zv_av_lib *This,
                                         uint8_t * pBin, 
                                         uint32_t lenBin,
                                         uint32_t load_addr
                                         )   
{
    uint32_t    read_size;
    uint32_t    remain = lenBin;
    uint32_t    offset = 0;
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
    uint32_t    i;
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL
#ifndef USE_MEMRD_4_DMA
    uint32_t    verify_remain;
    uint32_t    verify_offset;
    uint32_t    verify_size;
#endif // !USE_MEMRD_4_DMA
	zoe_errs_t	err;
    uint8_t     *readBuf;
    zoe_bool_t  bRet = ZOE_TRUE;

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
		           "image(0x%x), size(%d), load_addr(0x%x)\n",
                   pBin,
                   lenBin,
                   load_addr
		           );

    readBuf = (uint8_t *)zoe_sosal_memory_local_alloc(MAX_READ_SIZE);
    if (!readBuf)
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "zoe_sosal_memory_local_alloc failed!!!\n"
		               );
        return (ZOE_FALSE);
    }

    while (remain)
    {
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
        if (load_addr & 0x70000000)
        {
            read_size = remain;

            for (i = 0; i < read_size; i += 4)
            {
                ZOEHAL_REG_READ(&This->m_iHal, load_addr + i, (uint32_t *)(readBuf + i));
            }

		    if (0 != memcmp(pBin, readBuf, read_size))
		    {
                bRet = ZOE_FALSE;
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
		                       "verify failed at load_addr=%08x size=%d!!!\n",
                               load_addr, 
                               read_size
		                       );
		    }
        }
        else
        {
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL
#ifdef USE_MEMRD_4_DMA
            read_size = ZOE_MIN(MAX_READ_SIZE, remain);

            err = ZOEHAL_MEM_READ_EX(&This->m_iHal, 
                                     load_addr, 
                                     readBuf, 
                                     read_size, 
                                     ZOE_TRUE
                                     );
#else // !USE_MEMRD_4_DMA
            verify_remain = read_size = ZOE_MIN(MAX_READ_SIZE, remain);
            verify_offset = 0;

            err = c_zv_codec_dma_read(This->m_pZVCodec, 
                                      load_addr, 
                                      readBuf, 
                                      read_size, 
                                      ZOE_FALSE,
                                      ZOE_TRUE,
                                      DMA_BUFFER_MODE, 
                                      ZOE_NULL
                                      );
#endif //USE_MEMRD_4_DMA
            if (!ZOE_SUCCESS(err))
            {
                bRet = ZOE_FALSE;
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zv_codec_dma_read failed err(%d)!!!\n",
                               err
			                   );
                break;
            }
#ifdef USE_MEMRD_4_DMA
            // check mem wr buffer
		    if (0 != memcmp(pBin + offset, readBuf, read_size))
		    {
                bRet = ZOE_FALSE;
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
		                       "verify failed at load_addr=%08x size=%d!!!\n",
                               load_addr, 
                               read_size
		                       );
            }
#else // !USE_MEMRD_4_DMA
            while (verify_remain)
            {
                verify_size = ZOE_MIN(MAX_VERIFY_SIZE, verify_remain);

		        if (0 != memcmp(pBin + offset + verify_offset, readBuf + verify_offset, verify_size))
		        {
		            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
		                           "verify failed at load_addr=%08x size=%d!!!\n",
                                   load_addr + verify_offset, 
                                   verify_size
		                           );
                }
                verify_remain -= verify_size;
                verify_offset += verify_size;
		    }
#endif //USE_MEMRD_4_DMA
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
        }
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL
        remain -= read_size;
        load_addr += read_size;
        offset += read_size;
    }

    zoe_sosal_memory_free(readBuf);
    return (bRet);
}



/////////////////////////////////////////////////////////////////////////////
//
//

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
#define MAX_HANDSHAKE_WAIT      50
#define HANDSHAKE_WAIT_DELAY_MS 500//100 //500
#else // ZOE_TARGET_CHIP != ZOE_TARGET_CHIP_DAWN
#define MAX_HANDSHAKE_WAIT      200
#define HANDSHAKE_WAIT_DELAY_MS 25
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN


static zoe_bool_t c_zv_av_lib_wait_firmware_running(c_zv_av_lib *This, 
                                                    uint32_t cpu_id
                                                    )
{
    uint32_t    timeout = 0;

    // wait for for firmware init to complete
    while (!c_zoe_ipc_service_target_opened(This->m_pZoeIPCService, 
                                            cpu_id
                                            )) 
    {
        timeout++;
        if (timeout >= MAX_HANDSHAKE_WAIT)
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
				           "Wait for firmware ready Timeout!!!\n"
				           );
            return (ZOE_FALSE);
        }
        zoe_sosal_thread_sleep_ms(HANDSHAKE_WAIT_DELAY_MS); 
    }

	zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                   This->m_dbgID,
		           "firmware running after %d ms....\n",
                   timeout * HANDSHAKE_WAIT_DELAY_MS
		           );

    // give it some more time
    if (timeout)
    {
        zoe_sosal_thread_sleep_ms((timeout >> 2) * HANDSHAKE_WAIT_DELAY_MS); 

	    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
		               "wait another %d ms....\n",
                       (timeout >> 2) * HANDSHAKE_WAIT_DELAY_MS
		               );
    }
    return (ZOE_TRUE);
}



zoe_errs_t c_zv_av_lib_firmware_download(c_zv_av_lib *This)
{
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)
    uint32_t                        firmware_load_addr;
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN
    uint32_t                        ret;
    zoe_bool_t                      bVerify;
    zoe_errs_t                      err = ZOE_ERRS_SUCCESS;
    ZVAVLIB_CMD_REQ_FIRMWARE_DATA   fws;
#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)
    uint32_t                        timeout;
	uint32_t	                    spu_start_addr;
	uint32_t	                    spu_load_addr;
	uint32_t	                    sfw_load_addr;
    uint32_t                        firmware_load_addr;
    zoe_bool_t                      skip_sfw_load;
    uint32_t                        dwReg;
#endif // ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL

    // request firmware
    //
    err = c_zv_av_lib_device_callback(This, 
                                      ZVAVLIB_CMD_REQ_FIRMWARE, 
                                      (zoe_void_ptr_t)&fws
                                      );
    if (ZOE_FAIL(err))
    {
        goto firmware_download_exit;
    }

#if (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_CHISEL)

    skip_sfw_load = !fws.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] &&
                    !fws.bDownloadFW[ZVAVLIB_IMAGE_SFW]
                    ;

    // boot code
    //
    if (fws.bDownloadFW[ZVAVLIB_IMAGE_BOOT_CODE])
    {
        if (This->m_InitData.bSecureMode) 
        {
            ZOEHAL_REG_READ(&This->m_iHal, 0x75910004, &dwReg);
            if (HOST_HANDSHAKE_LOAD_SEC_BOOTCODE == dwReg) 
            {
                // Cold boot in secure mode
                //
                ZOEHAL_REG_READ(&This->m_iHal, 0x75910008, &dwReg);
                // load boot code into SRAM
                ret = c_zv_av_lib_load_bin(This, 
                                           fws.pFW[ZVAVLIB_IMAGE_BOOT_CODE], 
                                           fws.FWSize[ZVAVLIB_IMAGE_BOOT_CODE], 
                                           dwReg
                                           );
                if (!ret)
                {
			        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
						           "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_BOOT_CODE) Failed!!!\n"
						           );
                    err = ZOE_ERRS_FAIL;
                    goto firmware_download_exit;
                }

                bVerify = c_zv_av_lib_verify_bin(This,
                                                 fws.pFW[ZVAVLIB_IMAGE_BOOT_CODE], 
                                                 fws.FWSize[ZVAVLIB_IMAGE_BOOT_CODE], 
                                                 dwReg
                                                 );
                if (!bVerify)
                {
		            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                                   This->m_dbgID,
				                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_BOOT_CODE) Failed!!!\n"
				                   );
                    err = ZOE_ERRS_FAIL;
                    goto firmware_download_exit;
                }
                else
                {
		            zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                                   This->m_dbgID,
				                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_BOOT_CODE) Succeed\n"
				                   );
                }

                // clear MISC SCRATCH 0-1
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910004, 0);
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910008, 0);
                // set size
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910010, fws.FWSize[ZVAVLIB_IMAGE_BOOT_CODE]);
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x7591000C, HOST_RESPONSE_LOAD_COMPLETE);

                zoe_sosal_thread_sleep_ms(300);
            }
            else
            {
                // Assume it was a warm boot: just reload firmware
                //
                skip_sfw_load = ZOE_TRUE;

                // reset cpu and cpu space except for SPU and HPU(mips_sdk)
                // 
                if (ZOEHAL_BUS_HPU == This->m_InitData.codecInitData.BusType)
                {
                    ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800040, 3);
                    ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 3);
                }
                else
                {
                    ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800040, 2);
                    ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 2);
                }

                zoe_sosal_thread_sleep_ms(30);

                // reset hardware modules, this will be done again when we actually
                // loading the image. this is to prevent any modules still actively
                // assess memory buffers that they no longer own
                //
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x71802000,
                    0x00000800 | 0x00001000 | 
                    0x00000004 | 
                    0x00002000 | 0x00004000 | 0x00008000 | 
                    0x00400000 | 0x00800000 | 
                    0x01000000 | 0x02000000 | 
                    0x10000000
                    );
                zoe_sosal_thread_sleep_ms(30);
            }
        }
        else
        {
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x71801800, 1 << 22);
            ZOEHAL_REG_READ(&This->m_iHal, 0x70f10820, &dwReg);
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x70f10820, dwReg & ~0x06);
            zoe_sosal_thread_sleep_ms(1); 
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x70f10820, dwReg | 0x06);

            // reset SPU and SPU space
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800040, 0);
            if (ZOEHAL_BUS_HPU == This->m_InitData.codecInitData.BusType)
            {
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 1);
                // take SPU space ouf of reset so we can load boot code
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 3);
            }
            else
            {
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 0);
                // take SPU space ouf of reset so we can load boot code
                ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 2);
            }

            // load boot code into SRAM
            ret = c_zv_av_lib_load_bin(This, 
                                       fws.pFW[ZVAVLIB_IMAGE_BOOT_CODE], 
                                       fws.FWSize[ZVAVLIB_IMAGE_BOOT_CODE], 
                                       0x70010000
                                       );
            if (!ret)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
					           "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_BOOT_CODE) Failed!!!\n"
					           );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }

            bVerify = c_zv_av_lib_verify_bin(This,
                                             fws.pFW[ZVAVLIB_IMAGE_BOOT_CODE], 
                                             fws.FWSize[ZVAVLIB_IMAGE_BOOT_CODE], 
                                             0x70010000
                                             );
            if (!bVerify)
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_BOOT_CODE) Failed!!!\n"
			                   );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }
            else
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
			                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_BOOT_CODE) Succeed\n"
			                   );
            }

            // set spu boot mode
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800604, 0);
            // set to boot from sram
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x7008c200, 0x60014000);
            // clear MISC SCRATCH 0-3 -- the handshake register
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910004, 0);
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910008, 0);
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x7591000c, 0);
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910010, 0);
            // take SPU out of reset request
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800040, 2);
        }
    }

    // SPU code
    //
	if (!skip_sfw_load) 
    {
		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
			           "begin loading SFW firmware protocol\n"
			           );
        // Wait for boot code to be ready (SCRATCH_0 == HOST_HANDSHAKE_LOAD_SFW_CODE (0xABBA0001))
        timeout = 0;
        do {
            timeout++;
            if (timeout >= MAX_HANDSHAKE_WAIT)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
					           "Wait for boot code to be ready Timeout (0x%x)!!!\n",
                               dwReg
					           );
                err = ZOE_ERRS_TIMEOUT;
                goto firmware_download_exit;
            }
            dwReg = 0;
            ZOEHAL_REG_READ(&This->m_iHal, 0x75910004, &dwReg);
            zoe_sosal_thread_sleep_ms(10); 
        } while (dwReg != 0xABBA0001);


		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
			           "ready to load SFW firmware\n"
			           );

		// the boot code returns an address in SCRATCH_1: it is 0x900000 in secure mode
        spu_start_addr = fws.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT] ? 0x800000 : 0x100000;
        ZOEHAL_REG_READ(&This->m_iHal, 0x75910008, &dwReg);
        spu_load_addr = This->m_InitData.bSecureMode ? dwReg : spu_start_addr;

        // custom boot code
        //
        if (fws.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT])
        {
            sfw_load_addr = 0x4000010;
            ret = c_zv_av_lib_load_bin(This, 
                                       fws.pFW[ZVAVLIB_IMAGE_CUSTOM_BOOT], 
                                       fws.FWSize[ZVAVLIB_IMAGE_CUSTOM_BOOT], 
                                       spu_load_addr
                                       );
            if (!ret)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
					           "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_CUSTOM_BOOT) Failed!!!\n"
					           );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }

            bVerify = c_zv_av_lib_verify_bin(This,
                                             fws.pFW[ZVAVLIB_IMAGE_CUSTOM_BOOT], 
                                             fws.FWSize[ZVAVLIB_IMAGE_CUSTOM_BOOT], 
                                             spu_load_addr
                                             );
            if (!bVerify)
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_CUSTOM_BOOT) Failed!!!\n"
			                   );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }
            else
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
			                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_CUSTOM_BOOT) Succeed\n"
			                   );
            }
        }
        else
        {
            sfw_load_addr = spu_load_addr;
        }

        // SFW code
        //
        if (fws.bDownloadFW[ZVAVLIB_IMAGE_SFW])
        {
            ret = c_zv_av_lib_load_bin(This, 
                                       fws.pFW[ZVAVLIB_IMAGE_SFW], 
                                       fws.FWSize[ZVAVLIB_IMAGE_SFW], 
                                       sfw_load_addr
                                       );
            if (!ret)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
					           "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_SFW) Failed!!!\n"
					           );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }

            bVerify = c_zv_av_lib_verify_bin(This,
                                             fws.pFW[ZVAVLIB_IMAGE_SFW], 
                                             fws.FWSize[ZVAVLIB_IMAGE_SFW], 
                                             sfw_load_addr
                                             );
            if (!bVerify)
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_SFW) Failed!!!\n"
			                   );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }
            else
            {
		        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                               This->m_dbgID,
			                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_SFW) Succeed\n"
			                   );
            }

            if (fws.bDownloadFW[ZVAVLIB_IMAGE_CUSTOM_BOOT])
            {
				// put the SFW bin in DDR with the correct header for the custom boot
                ZOEHAL_MEM_WRITE(&This->m_iHal, 0x4000000, fws.FWSize[ZVAVLIB_IMAGE_SFW], ZOE_FALSE);
                ZOEHAL_MEM_WRITE(&This->m_iHal, 0x4000004, 0xABBACAFE, ZOE_FALSE);  // signature
                ZOEHAL_MEM_WRITE(&This->m_iHal, 0x4000008, 0x5A534657, ZOE_FALSE);  // 'ZSFW'
                ZOEHAL_MEM_WRITE(&This->m_iHal, 0x400000C, 0, ZOE_FALSE);	        // reserved
            }
        }

        // write the actual address in SCRATCH_3
        ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910010, spu_start_addr);
        // signal it's done: HOST_RESPONSE_LOAD_SFW_COMPLETE (0xABBA0002) -> MISC_CTRL_SCRATCH_2
        ZOEHAL_REG_WRITE(&This->m_iHal, 0x7591000C, 0xABBA0002);
        // give it some time
        zoe_sosal_thread_sleep_ms(500);

		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
			           "SFW firmware loaded\n"
			           );
    }

    // firmware
    //
    if (fws.bDownloadFW[ZVAVLIB_IMAGE_FW])
    {
        // wait for SPU to ask for firmware
        timeout = 0;
        do {
            timeout++;
            if (timeout >= MAX_HANDSHAKE_WAIT)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
					           "Wait for SPU to ask for firmware Timeout!!!\n"
					           );
                err = ZOE_ERRS_TIMEOUT;
                goto firmware_download_exit;
            }
            ZOEHAL_REG_READ(&This->m_iHal, 0x75910004, &dwReg);
            zoe_sosal_thread_sleep_ms(10); 
        } while (dwReg != HOST_HANDSHAKE_LOAD_FIRMWARE);

		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
			           "ready to download firmware\n"
			           );


        ZOEHAL_REG_READ(&This->m_iHal, 0x71800048, &dwReg);
        if (ZOEHAL_BUS_HPU == This->m_InitData.codecInitData.BusType)
        {
            dwReg = dwReg & 0x39e;
        }
        generate_halt_interrupt(This, dwReg);

        // clear firmware ready indicator
        ZOEHAL_MEM_WRITE(&This->m_iHal, SYS_HW_RESETS_DONE_INDICATOR, 0, ZOE_FALSE);
        // clear USB active flag
        if (ZOEHAL_BUS_USB != This->m_InitData.codecInitData.BusType)
        {
            ZOEHAL_MEM_WRITE(&This->m_iHal, ZSYS_USB_DEVICE_ACTIVE, 0, ZOE_FALSE);
        }

        firmware_load_addr = 0x4000000;

        ret = c_zv_av_lib_load_bin(This, 
                                   fws.pFW[ZVAVLIB_IMAGE_FW], 
                                   fws.FWSize[ZVAVLIB_IMAGE_FW], 
                                   firmware_load_addr
                                   );
        if (!ret)
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
				           "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
				           );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }

        // give it some time
        zoe_sosal_thread_sleep_ms(30);

        bVerify = c_zv_av_lib_verify_bin(This,
                                         fws.pFW[ZVAVLIB_IMAGE_FW], 
                                         fws.FWSize[ZVAVLIB_IMAGE_FW], 
                                         firmware_load_addr
                                         );
        if (!bVerify)
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
				           "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
				           );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }
        else
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
		                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Succeed\n"
		                   );
        }

        ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910008, firmware_load_addr);

		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
			           "firmware load done\n"
			           );
        if (ZOEHAL_BUS_HPU == fws.codecInitData.BusType)
        {
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 0x39f);
        }
        else
        {
            ZOEHAL_REG_WRITE(&This->m_iHal, 0x71800048, 0x39e);
        }

        ZOEHAL_REG_WRITE(&This->m_iHal, 0x75910010, fws.FWSize[ZVAVLIB_IMAGE_FW]);
        // tell boot code we have completed loading firmware
        ZOEHAL_REG_WRITE(&This->m_iHal, 0x7591000c, HOST_RESPONSE_LOAD_COMPLETE);

        // wait for for firmware init to complete
        timeout = 0;
        do {
            timeout++;
            if (timeout >= MAX_HANDSHAKE_WAIT)
            {
			    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
					           "Wait for firmware ready Timeout!!!\n"
					           );
                err = ZOE_ERRS_TIMEOUT;
                goto firmware_download_exit;
            }
            ZOEHAL_MEM_READ(&This->m_iHal, SYS_HW_RESETS_DONE_INDICATOR, &dwReg, ZOE_FALSE);
            zoe_sosal_thread_sleep_ms(10); 
        } while (dwReg != ZN200_READY_FOR_OPERATION);

		zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                       This->m_dbgID,
			           "firmware running....\n"
			           );
    }
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_DAWN)

    // firmware
    //
    if (fws.bDownloadFW[ZVAVLIB_IMAGE_FW])
    {
		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "ready to download firmware\n"
			           );

	    // Reset first
	    CPUStopSPU(This);

        c_zoe_ipc_service_target_open_clr(This->m_pZoeIPCService, 
                                          ZOE_IPC_SPU
                                          );
        firmware_load_addr = 0x100000;

#if 0
        setParkingCode(This, firmware_load_addr, MIPSILoopCode, SIZEOF_ARRAY(MIPSILoopCode));
#else

        ret = c_zv_av_lib_load_bin(This, 
                                   fws.pFW[ZVAVLIB_IMAGE_FW], 
                                   fws.FWSize[ZVAVLIB_IMAGE_FW], 
                                   firmware_load_addr
                                   );
        if (!ret)
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
				           "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
				           );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }

        // give it some time
        zoe_sosal_thread_sleep_ms(30);

        bVerify = c_zv_av_lib_verify_bin(This,
                                         fws.pFW[ZVAVLIB_IMAGE_FW], 
                                         fws.FWSize[ZVAVLIB_IMAGE_FW], 
                                         firmware_load_addr
                                         );
        if (!bVerify)
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
				           "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
				           );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }
        else
        {
		    zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
		                   "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Succeed\n"
		                   );
        }
#endif

        CPUStartSPU(This, 
                    firmware_load_addr
                    );

		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "firmware load done\n"
			           );

	    //zoe_sosal_thread_sleep_ms(100);

        // wait for for firmware init to complete
        if (!c_zv_av_lib_wait_firmware_running(This, 
                                               ZOE_IPC_SPU
                                               ))
        {
            err = ZOE_ERRS_TIMEOUT;
            goto firmware_download_exit;
        }
    }
#elif (ZOE_TARGET_CHIP == ZOE_TARGET_CHIP_MX8)

#define MISC_SCRATCH_MEMORY (MISC_XREG_SLV_BASE + MED_MISC_CTRL_MISC_CTRL_SCRATCH_MEMORY)

    if (fws.bDownloadFW[ZVAVLIB_IMAGE_FW])
    {
        zoe_errs_t              err;
        int                     rv;
	    uint32_t                coreNum;
        zoe_image_start_ptr_t	isP;
        uint32_t                fw_checksum, fw;

        fw = (uint32_t)This->m_p_fw_space_phy;
        fw_checksum = ~fw;
        // save the firmware location in the misc scratch register
        ZOEHAL_REG_WRITE(&This->m_iHal, MISC_SCRATCH_MEMORY + 8, fw);
        ZOEHAL_REG_WRITE(&This->m_iHal, MISC_SCRATCH_MEMORY + 0xC, fw_checksum);

        // make sure the vpu is powered up
        //
        err = c_zv_av_lib_do_vpu_start(This);
        if (ZOE_FAIL(err))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
			               "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_FW) c_zv_av_lib_do_vpu_start Failed (%d)!!!\n",
                           err
			               );
            goto firmware_download_exit;
        }

        // stop the cpu first
        //
		for (coreNum = 0; coreNum < 4; coreNum++) 
        {
            rv = setCorePwr(This,
                            coreNum, 
                            SC_PM_PW_MODE_OFF
                            );
            if (0 != rv)
            {
	            zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                               This->m_dbgID,
			                   "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_FW) setCorePwr Failed!!!\n"
			                   );
                err = ZOE_ERRS_FAIL;
                goto firmware_download_exit;
            }
        }

        c_zoe_ipc_service_target_open_clr(This->m_pZoeIPCService, 
                                          ZOE_IPC_FWPU
                                          );

        // print the embedded firmware info
        //
		isP = (zoe_image_start_ptr_t)fws.pFW[ZVAVLIB_IMAGE_FW];
		if (isP->signature == ZOE_IMAGE_SIGNATURE) 
        {
			char    *appNameP = isP->strings;
			char    *revidP = isP->strings + strlen(appNameP) + 1;

			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, " Image info:\n");
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, "       APP: %s\n", appNameP);
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, "  CHIP_VAR: %s", ZOE_TARGET_CHIP_STRING(isP->chipID));
			if (isP->chipVarID != 0) 
            {
				zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, "_%s", ZOE_TARGET_CHIP_VAR_STRING(isP->chipVarID));
			}
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, "\n");
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, "        OS: %s\n", ZOE_TARGET_OS_STRING(isP->osID));
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, "   FW size: %u 4k pages (%u kB)\n",(unsigned int) isP->part_size4k, (unsigned int)(4 * isP->part_size4k));
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, " Revision: %s\n", revidP);
		}
        else
        {
			zoe_dbg_printf(ZOE_DBG_LVL_ERROR, This->m_dbgID, " Invalid image signature 0x%08X, expecting 0x%08X\n",(unsigned int)isP->signature, ZOE_IMAGE_SIGNATURE);
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
		}

        // load the firmware to the specified location
        //
#if 1
        memcpy(This->m_p_fw_space_vir, 
               fws.pFW[ZVAVLIB_IMAGE_FW], 
               fws.FWSize[ZVAVLIB_IMAGE_FW]
               );
        if (0 != memcmp(This->m_p_fw_space_vir, 
                        fws.pFW[ZVAVLIB_IMAGE_FW], 
                        fws.FWSize[ZVAVLIB_IMAGE_FW]
                        ))
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
			               "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
			               );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
	                       "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Succeed\n"
	                       );
        }
#else
        ret = c_zv_av_lib_load_bin(This, 
                                   fws.pFW[ZVAVLIB_IMAGE_FW], 
                                   fws.FWSize[ZVAVLIB_IMAGE_FW], 
                                   This->m_p_fw_space_vir
                                   );
        if (!ret)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
			               "c_zv_av_lib_load_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
			               );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }
        bVerify = c_zv_av_lib_verify_bin(This,
                                         fws.pFW[ZVAVLIB_IMAGE_FW], 
                                         fws.FWSize[ZVAVLIB_IMAGE_FW], 
                                         This->m_p_fw_space_phy
                                         );
        if (!bVerify)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
			               "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Failed!!!\n"
			               );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }
        else
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_WARNING,
                           This->m_dbgID,
	                       "c_zv_av_lib_verify_bin(ZVAVLIB_IMAGE_FW) Succeed\n"
	                       );
        }
#endif
        // start the firmware
        //
        rv = start_fwpu(This, 
                        (uint64_t)This->m_p_fw_space_phy
                        );
        if (0 != rv)
        {
	        zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                           This->m_dbgID,
			               "start_fwpu Failed!!!\n"
			               );
            err = ZOE_ERRS_FAIL;
            goto firmware_download_exit;
        }

		zoe_dbg_printf(ZOE_DBG_LVL_ERROR,
                       This->m_dbgID,
			           "firmware load done\n"
			           );

	    //zoe_sosal_thread_sleep_ms(100);

        // wait for for firmware init to complete
        if (!c_zv_av_lib_wait_firmware_running(This, 
                                               ZOE_IPC_FWPU
                                               ))
        {
            err = ZOE_ERRS_TIMEOUT;
            goto firmware_download_exit;
        }
    }
#else //ZOE_TARGET_CHIP == ???
#error Unsupported CHIP
#endif //ZOE_TARGET_CHIP

    err = ZOE_ERRS_SUCCESS;

firmware_download_exit:

    // release firmware
    //
    c_zv_av_lib_device_callback(This, 
                                ZVAVLIB_CMD_REL_FIRMWARE, 
                                ZOE_NULL
                                );
    return (err);
}


/////////////////////////////////////////////////////////////////////////////
//
//


#ifdef _NO_SCU_API

zoe_errs_t c_zv_av_lib_open_scu(c_zv_av_lib *This)
{
    return (ZOE_ERRS_SUCCESS);
}


zoe_errs_t c_zv_av_lib_close_scu(c_zv_av_lib *This)
{
    return (ZOE_ERRS_SUCCESS);
}


zoe_errs_t c_zv_av_lib_do_vpu_start(c_zv_av_lib *This)
{
    return (ZOE_ERRS_SUCCESS);
}


zoe_errs_t c_zv_av_lib_do_vpu_stop(c_zv_av_lib *This)
{
    return (ZOE_ERRS_SUCCESS);
}

#else //!_NO_SCU_API

// open ipc to scu
//
zoe_errs_t c_zv_av_lib_open_scu(c_zv_av_lib *This)
{
    sc_err_t    sciErr;
    uint32_t    mu_id;

    if (0 == This->m_ipcHndl)
    {
        sciErr = sc_ipc_getMuID(&mu_id);
        if (sciErr != SC_ERR_NONE) 
        {
            zoe_dbg_printf_nc_e("--- sc_ipc_getMuID() cannot obtain mu id SCI error! (%d)\n", sciErr);
            return (ZOE_ERRS_FAIL);
        }

        sciErr = sc_ipc_open(&This->m_ipcHndl, mu_id);
        if (sciErr != SC_ERR_NONE) 
        {
            zoe_dbg_printf_nc_e("--- sc_ipc_getMuID() cannot open MU channel to SCU error! (%d)\n", sciErr);
            This->m_ipcHndl = 0;
            return (ZOE_ERRS_FAIL);
        }
    }
    return (ZOE_ERRS_SUCCESS);
}



// close ipc channel to scu
//
zoe_errs_t c_zv_av_lib_close_scu(c_zv_av_lib *This)
{
    if (0 != This->m_ipcHndl)
    {
        sc_ipc_close(This->m_ipcHndl);
        This->m_ipcHndl = 0;
    }
    return (ZOE_ERRS_SUCCESS);
}



zoe_errs_t c_zv_av_lib_do_vpu_start(c_zv_av_lib *This)
{
	zoe_errs_t          err = ZOE_ERRS_FAIL;
    int                 rv = -1;
    sc_pm_power_mode_t	pwrMode;

    zoe_dbg_printf_nc_e("%s\n", __FUNCTION__);

    // Check if it is already powered on
    pwrMode = getVPUPwr(This->m_ipcHndl);
    if (pwrMode < 0) 
    {
        goto do_vpu_startExit;
    }
    if (pwrMode == SC_PM_PW_MODE_ON) 
    {
        zoe_dbg_printf_nc_e("+++ VPU already ON\n");
        err = ZOE_ERRS_SUCCESS;
    }
    else
    {
        rv = setVPUPwr(This->m_ipcHndl, SC_PM_PW_MODE_ON);
        if (0 == rv)
        {
            err = ZOE_ERRS_SUCCESS;
        }
    }

do_vpu_startExit:
    return (err);
}



zoe_errs_t c_zv_av_lib_do_vpu_stop(c_zv_av_lib *This)
{
	zoe_errs_t          err = ZOE_ERRS_FAIL;
    int                 rv = -1;
    sc_pm_power_mode_t	pwrMode;

    zoe_dbg_printf_nc_e("%s\n", __FUNCTION__);

    // Check if it is already powered off
    pwrMode = getVPUPwr(This->m_ipcHndl);
    if (pwrMode < 0) 
    {
        goto do_vpu_stopExit;
    }
    if (pwrMode == SC_PM_PW_MODE_OFF) 
    {
        zoe_dbg_printf_nc_e("+++ VPU already OFF\n");
        err = ZOE_ERRS_SUCCESS;
    }
    else
    {
		rv = setVPUPwr(This->m_ipcHndl, SC_PM_PW_MODE_OFF);
        if (0 == rv)
        {
            err = ZOE_ERRS_SUCCESS;
        }
    }

do_vpu_stopExit:
	return (err);
}

#endif //_NO_SCU_API






