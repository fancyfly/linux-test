/*
 * Copyright 2012-2015 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */


#include <linux/errno.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/poll.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <asm/io.h>
#include <linux/usb.h>
#include <linux/string.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <asm/fcntl.h>


int zvf_open(unsigned long *pdwHandle, unsigned char *pFName, unsigned int dwReadOnly) 
{
    int Err = 0;
    if (dwReadOnly) 
    {
        *pdwHandle = (unsigned long)filp_open(pFName, 0, 0); 
    } else 
    {
        *pdwHandle = (unsigned long)filp_open(pFName, O_CREAT | O_RDWR | O_TRUNC, 0); 
    }
    if (IS_ERR((void*)*pdwHandle)) 
    {
        Err= *pdwHandle;
        *pdwHandle= 0;
    } 
    printk("zvdrv: open %s. Err=%d \n", pFName, Err);
    return (Err);
}



int zvf_close(unsigned long dwHandle) 
{
    if ((NULL != (struct file*)dwHandle) && (!(IS_ERR((struct file*)dwHandle))))
    {
        return (filp_close((struct file*)dwHandle, 0));
    }
    else 
    {
        return (0); 
    }
}

int zvf_write(unsigned long dwHandle, unsigned char *pBuff, unsigned int dwLen) 
{
    mm_segment_t    orgfs;
    int             Err = 0;

    orgfs = get_fs();
    set_fs(get_ds());

    if ((NULL != (struct file*)dwHandle) && (!(IS_ERR((struct file*)dwHandle))))
    {
#ifndef __LINUX24__
        Err = vfs_write((void*)dwHandle, pBuff, dwLen, &((struct file*)dwHandle)->f_pos);
#else //__LINUX24__
        Err = ((struct file*)dwHandle)->f_op->write((void*)dwHandle, pBuff, dwLen, &((struct file*)dwHandle)->f_pos);
#endif //!__LINUX24__
    }
    else 
    {
        Err = -EINVAL;
    }

    set_fs(orgfs);
    return (Err);
}

int zvf_read(unsigned long dwHandle, unsigned char *pBuff, unsigned int dwLen) 
{
    mm_segment_t    orgfs;
    int             Err = 0;

    if ((struct file*)dwHandle == NULL)
    {
        return (-ENOENT);
    }
#ifdef __LINUX24__
    if (((struct file*)dwHandle)->f_op->read == NULL)
    {
        return (-ENOSYS);
    }
#endif //__LINUX24__

    orgfs = get_fs();
    set_fs(get_ds());

    if((NULL != (struct file*)dwHandle) && (!(IS_ERR((struct file*)dwHandle))))
    {
#ifndef __LINUX24__
        Err = vfs_read((struct file*)dwHandle, pBuff, dwLen, &((struct file*)dwHandle)->f_pos);
#else //__LINUX24__
        Err = ((struct file*)dwHandle)->f_op->read((void*)dwHandle, pBuff, dwLen, &((struct file*)dwHandle)->f_pos);
#endif //!__LINUX24__
    }
    else
    {
        Err = -EINVAL;
    }

    set_fs(orgfs);
    return (Err);
}



int zvf_load(char *pszFileName, 
			  unsigned char **ppucaBuff, 
			  unsigned int *pulBuffLen
			  )
{
	struct file		*filp = 0;
	int				l=0, curRead;
    int				lErr = 0;
    unsigned char	*pucaBuff = NULL;

	*ppucaBuff = NULL;
	*pulBuffLen = 0;

	lErr = zvf_open((unsigned long *)&filp, 
					pszFileName, 
					1
					);

    if (!lErr && 
		filp
		) 
	{
		l = filp->f_path.dentry->d_inode->i_size;

		// allocate buffer
		pucaBuff = (unsigned char *)vmalloc(l);

		if (pucaBuff)
		{
			curRead = zvf_read((unsigned long)filp, pucaBuff, l);

			if ((curRead < 0) || 
				(curRead != l)
				)
			{
				printk("Failed to read '%s' expected(%d) read(%d).\n", pszFileName, l, curRead);
				vfree(pucaBuff);
				lErr = -EINVAL;
			}
		}
		else
		{
			printk("Failed to allocate (%d) bytes.\n", l);
			lErr = -ENOMEM;
		}
		zvf_close((unsigned long)filp);
	}

	if (!lErr)
	{
		*ppucaBuff = pucaBuff;
		*pulBuffLen = l;
	}

	printk("%s()-> file %s, size %d\n", __FUNCTION__, pszFileName, *pulBuffLen);

	return (lErr);
}






