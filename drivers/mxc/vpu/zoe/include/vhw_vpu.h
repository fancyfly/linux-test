//HEADER
#ifndef __VHW_VPU_H__
#define __VHW_VPU_H__


#include "vlib_common.h"
#include "block_to_linear.h"
#include "vreflist_hevc.h"
#ifdef __MSC_VER
    #include "drvwrapper.h"
#else
    #include "zsys_startup.h"
#endif
class c_dump_hevc_pic_desc;
class c_hevc_pic_desc;
class c_hevc_dec_context;
class c_nal_unit_header;
class c_hevc_decoder;
class c_vreflist_hevc;

extern bool copyglobalsworkaround; // is toggled dynamically
void workaround(uint32_t addr, uint32_t value);

STATIC_INLINE uint32_t xray_reg_rd(uint32_t addr)
{
    uint32_t value = zn_read_reg(zsys_startup_devinstifid, addr);

    return value;
}


STATIC_INLINE void xray_reg_wr(uint32_t addr, uint32_t value)
{
    workaround(addr, value);
    zn_write_reg(zsys_startup_devinstifid, addr, value);
}

STATIC_INLINE void xray_reg_wr_rdback(uint32_t addr, uint32_t value)
{
    workaround(addr, value);
    zn_write_reg(zsys_startup_devinstifid, addr, value);
    if((addr & 0xffff0000) != 0x10950000) {
        zn_read_reg(zsys_startup_devinstifid, addr);
    }
}

STATIC_INLINE void xray_mem_wr(uint32_t addr, uint32_t value)
{
    zn_write_mem(zsys_startup_devinstifid, addr, &value, 4);
}

STATIC_INLINE uint32_t xray_mem_rd(uint32_t addr)
{
    uint32_t value;
    zn_read_mem(zsys_startup_devinstifid, addr, &value, 4);

    return value;
}

class c_xray_diag_dma
{

public:

    void reset_hevc_decoder(uint32_t channel);
    void program(uint32_t channel, uint32_t ddr_addr, uint32_t transfer_size);
    void trim_bits(uint32_t trim_bits_from_start, uint32_t ddr_addr);
};


///Class c_vhw_vpu implements the register and memory read write interface
class c_vhw_vpu
{
	
	uint32_t zsys_startup_devinstifid_;
	

public:
	c_vhw_vpu()
	{
		zsys_startup_devinstifid_ = 0;//ZN_MAKE_DEV_INTERFACE_ID(ZN_SHARED_MEM, ZN_INSTANCE_1);
	}

	uint32_t vpu_reg_rd(uint32_t addr)
	{
		uint32_t value = zn_read_reg(zsys_startup_devinstifid_, addr);

		return value;
	}

	void vpu_reg_wr(uint32_t addr, uint32_t value)
	{

		zn_write_reg(zsys_startup_devinstifid_, addr, value);
	}

	void vpu_reg_wr_rdback(uint32_t addr, uint32_t value)
	{
		zn_write_reg(zsys_startup_devinstifid_, addr, value);

	}

	void  vpu_mem_wr(uint32_t addr, uint32_t value)
	{
		zn_write_mem(zsys_startup_devinstifid_, addr, &value, 4);
	}

	uint32_t  vpu_mem_rd(uint32_t addr)
	{
		uint32_t value;
		zn_read_mem(zsys_startup_devinstifid_, addr, &value, 4);

		return value;
	}

};

/// Enum \ref vpu_addr_flags specifies chroma, bottom field and 10 bit pixel flags.
typedef enum vpu_addr_flags
{
    VPU_ADDR_CHROMA = 1 << 6,       ///< The address is in a chroma buffer.
    VPU_ADDR_BOT_FIELD = 1 << 7, ///< The address is in a bottom field.
    VPU_ADDR_10_BIT_PIXEL = 1 << 14,///< The address points to 10 bit pixel data.
} vpu_addr_flags_t;

/// Function \ref vpu_ddr_address calculates a ddr address with additional flags used by the hardware, like stride, 
/// luma/chroma, top/bottom and 10bit/8bit pixel.
/// \param[in] base_addr Address to which additional information is added.
/// \param[in] stride The stride in pixels up to 8K (8196); note 8K is stored as a 0 and stride must be a multiple of 256;
///                   also colocated data must have half the luma stride.
/// \param[in] addr_flags OR'd value of \ref vpu_addr_flags_t specifying luma/chroma, top/bottom and 10/8 bit pixel.
/// \returns Base address with specified values added to it, which then can be used for address programming.
STATIC_INLINE uint32_t vpu_ddr_address(uint32_t base_addr, uint32_t stride, uint32_t addr_flags)
{
    uint32_t stride_val = stride & 0x3f00;
    uint32_t addr = base_addr | stride_val | addr_flags;
    return addr;
}

#endif
