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
// zoe_cfifo.c
//
// Description: 
//
//	C implementation of the fifo.
//
// Authors: (dtao) David Tao
//
///////////////////////////////////////////////////////////////////////////////


#include "zoe_cfifo.h"
#include "zoe_sosal.h"
#ifndef ZOE_LINUXKER_BUILD
#include <string.h>
#else //ZOE_LINUXKER_BUILD
#include <linux/string.h>
#endif //!ZOE_LINUXKER_BUILD

/////////////////////////////////////////////////////////////////////////////
//
//

#define C_FIFO_AUTO_GROWN_SIZE  16


/////////////////////////////////////////////////////////////////////////////
//
//

c_fifo * c_fifo_constructor(c_fifo *pFifo,
						    c_object *pParent, 
						    uint32_t dwAttributes,
						    uint32_t size,
						    uint32_t sizeEntry
						    )
{
	if (pFifo)
	{
		c_object_constructor(&pFifo->m_Object,
		 					 pParent,
                             OBJECT_FIFO,
							 dwAttributes
		 					 );
		pFifo->m_dwReadPtr = 0;
		pFifo->m_dwWritePtr = 0;
		pFifo->m_dwFifoLevel = 0;
		pFifo->m_size = size;
		pFifo->m_sizeEntry = sizeEntry;

		pFifo->m_Fifo = (int8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                         sizeEntry * size, 
                                                         0
                                                         );
		if (!pFifo->m_Fifo)
		{
			c_fifo_destructor(pFifo);
			pFifo = ZOE_NULL;
		}
	}

	return (pFifo);
}



void c_fifo_destructor(c_fifo *This) 
{
	if (This->m_Fifo)
	{
		zoe_sosal_memory_free((void *)This->m_Fifo);
		This->m_Fifo = ZOE_NULL;
	}

	c_object_destructor(&This->m_Object);
}



zoe_bool_t c_fifo_peek_fifo(c_fifo *This, 
                            zoe_void_ptr_t val
                            )
{
	zoe_bool_t ret;

	if (!This)
	{
		return (ZOE_FALSE);
	}

	ENTER_CRITICAL(&This->m_Object)

	ret = (This->m_dwReadPtr != This->m_dwWritePtr);
	if (ret)
	{
		memcpy(val, 
			   (void *)(This->m_Fifo + (This->m_dwReadPtr * This->m_sizeEntry)),
			   This->m_sizeEntry
			   );
	}

	LEAVE_CRITICAL(&This->m_Object)
	return (ret);
}



zoe_bool_t c_fifo_get_fifo(c_fifo *This, 
                           zoe_void_ptr_t val
                           )
{
	zoe_bool_t ret;

	if (!This)
	{
		return (ZOE_FALSE);
	}

	ENTER_CRITICAL(&This->m_Object)

	ret = (This->m_dwReadPtr != This->m_dwWritePtr);
	if (ret)
	{
		memcpy(val, 
			   (void *)(This->m_Fifo + (This->m_dwReadPtr * This->m_sizeEntry)),
			   This->m_sizeEntry
			   );

		if (This->m_dwReadPtr >= This->m_dwWritePtr)
		{
			if (This->m_dwReadPtr == (This->m_size - 1))
				This->m_dwReadPtr = 0; // wrap around
			else
				This->m_dwReadPtr++;
		}
		else
			This->m_dwReadPtr++;

		// decrease number in fifo
		This->m_dwFifoLevel--;
	}

	LEAVE_CRITICAL(&This->m_Object)
	return (ret);
}



zoe_bool_t c_fifo_set_fifo(c_fifo *This, 
                           zoe_void_ptr_t Data
                           )
{
	if (!This ||
		!This->m_Fifo)
	{
		return (ZOE_FALSE);
	}

	ENTER_CRITICAL(&This->m_Object)

	// sacrifice one fifo entry for efficency
	if (This->m_dwFifoLevel == This->m_size - 1)
	{
        // try to grow the fifo by C_FIFO_AUTO_GROWN_SIZE
        uint32_t    size = This->m_size + C_FIFO_AUTO_GROWN_SIZE;
        int8_t      *buf = (int8_t *)zoe_sosal_memory_alloc(ZOE_SOSAL_MEMORY_POOLS_LOCAL,
                                                            This->m_sizeEntry * size, 
                                                            0
                                                            );
        if (!buf)
        {
		    LEAVE_CRITICAL(&This->m_Object)
		    return (ZOE_FALSE);
        }
        else
        {
            // move the fifo to the new buffer
            //
		    if (This->m_dwReadPtr >= This->m_dwWritePtr)
		    {
                // wrap around
                uint32_t    top, bottom, copy;

                top = This->m_size - This->m_dwReadPtr;
                bottom = This->m_dwWritePtr;
                copy = top * This->m_sizeEntry;

                // copy top
	            memcpy((void *)buf,
		               (void *)(This->m_Fifo + (This->m_dwReadPtr * This->m_sizeEntry)),
		               copy
		               );
                // copy bottom
	            memcpy((void *)(buf + copy),
		               (void *)(This->m_Fifo),
		               bottom * This->m_sizeEntry
		               );
		    }
		    else
            {
	            memcpy((void *)buf,
		               (void *)(This->m_Fifo + (This->m_dwReadPtr * This->m_sizeEntry)),
                       This->m_dwFifoLevel * This->m_sizeEntry
		               );
            }
            This->m_dwReadPtr = 0;
            This->m_dwWritePtr = This->m_dwFifoLevel;
            This->m_size = size;
		    zoe_sosal_memory_free((void *)This->m_Fifo);
            This->m_Fifo = buf;
        }
	}

	// save it in the fifo
	memcpy((void *)(This->m_Fifo + (This->m_dwWritePtr * This->m_sizeEntry)),
		   Data,
		   This->m_sizeEntry
		   );

	// increase number of fifo
	This->m_dwFifoLevel++;

	// update write pointer
	if (This->m_dwWritePtr >= This->m_dwReadPtr)
	{
		if (This->m_dwWritePtr == (This->m_size - 1))
			This->m_dwWritePtr = 0; // wrap around
		else
			This->m_dwWritePtr++;
	}
	else
    {
		This->m_dwWritePtr++;
    }

	LEAVE_CRITICAL(&This->m_Object)
	return (ZOE_TRUE);
}



void c_fifo_flush_fifo(c_fifo *This) 
{
	if (This)
	{
		ENTER_CRITICAL(&This->m_Object)

		This->m_dwReadPtr = This->m_dwWritePtr = This->m_dwFifoLevel = 0;

		LEAVE_CRITICAL(&This->m_Object)
	}
}



void c_fifo_update_read_ptr(c_fifo *This)
{
	if (This)
	{
		ENTER_CRITICAL(&This->m_Object)

		// m_dwReadPtr == m_dwWritePtr should never happen in this case
		if (This->m_dwReadPtr >= This->m_dwWritePtr)
		{
			if (This->m_dwReadPtr == (This->m_size - 1))
				This->m_dwReadPtr = 0; // wrap around
			else
				This->m_dwReadPtr++;
		}
		else
			This->m_dwReadPtr++;

		// decrease number in fifo
		This->m_dwFifoLevel--;

		LEAVE_CRITICAL(&This->m_Object)
	}
}



zoe_bool_t c_fifo_update_entry(c_fifo *This, 
                               zoe_void_ptr_t val
                               )
{
	zoe_bool_t ret;

	if (!This ||
		!This->m_Fifo
		)
	{
		return (ZOE_FALSE);
	}

	ENTER_CRITICAL(&This->m_Object)

	ret = (This->m_dwReadPtr != This->m_dwWritePtr);
	if (ret)
	{
		memcpy((void *)(This->m_Fifo + (This->m_dwReadPtr * This->m_sizeEntry)),
			   val,
			   This->m_sizeEntry
			   );
	}

	LEAVE_CRITICAL(&This->m_Object)
	return (ret);
}



uint32_t c_fifo_get_fifo_level(c_fifo *This)
{
	return (This? This->m_dwFifoLevel : 0);
}



zoe_bool_t c_fifo_is_empty(c_fifo *This)
{
	return (This ? (0 == This->m_dwFifoLevel) : ZOE_TRUE);
}

