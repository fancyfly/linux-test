/*
 * Copyright 2004-2009 Freescale Semiconductor, Inc. All Rights Reserved.
 */

/*
 * The code contained herein is licensed under the GNU General Public
 * License. You may obtain a copy of the GNU General Public License
 * Version 2 or later at the following locations:
 *
 * http://www.opensource.org/licenses/gpl-license.html
 * http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/ctype.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>

#define MSC_CLASS_NAME "usb_gadget_msc"
#define MSC_NAME "usr_msc"
#define MMAP_SIZE PAGE_SIZE

/* shard with app */
typedef struct {
	unsigned int pa;	/* physical address of map mem */
	unsigned int map_size;	/* map size */
} usr_msc_info;

/* let usr msc run */
#define USR_MSC_RUN    _IOW('g', 1, unsigned short)

/* tell msc thread that disconnect event happened*/
#define USR_MSC_DISCONNECT    _IO('g', 2)

/* tell usr app some info*/
#define USR_MSC_INFO	_IOW('g', 3, usr_msc_info)

/* call 'fsg_bind' when open this inode */
#define USR_MSC_BIND    _IO('g', 4)

/* call 'fsg_unbind' when close this inode */
#define USR_MSC_UNBIND    _IO('g', 5)

/* call FSG_STATE_INTERFACE_CHANGE */
#define USC_MSC_SET_INTERFACE _IO('g', 6)

#define MAX_FILE_NAME_LEN 256

struct ShareInfo {
/* these two members will be accessed both kernel and app */
	int scsi;
	unsigned int lun;
/* the following members only filled during init phase
   app fill them and kernel read them.
	MscOpen will fill them;
	MscRun will let kernel run, kernel will read them.
*/
	unsigned char szFile[MAX_LUNS][MAX_FILE_NAME_LEN];
	int readOnly[MAX_LUNS];
	unsigned int numFileNames;
	int removable;
	int isHighSpeed;

	struct usb_endpoint_descriptor bulk_in;
	struct usb_endpoint_descriptor bulk_out;
};
/* end of shard with app */

static int msc_major;
static int msc_minor;
static struct cdev msc_cdev;
static struct class *msc_class;
static struct device *msc_dev;
static struct usb_gadget *save_gadget;

static dma_addr_t shm_dma;	/* physical address of share mem */
static void *shm_vir;		/* virtual address of share mem */

static void get_para_from_shm(struct ShareInfo *pShi);
static int __init fsg_bind(struct usb_gadget *gadget);
static int __init fsg_alloc(void);

static int msc_open(struct inode *inode, struct file *file)
{
	int rc = 0;
	struct fsg_dev *fsg;

	shm_dma = 0;
	shm_vir = dma_alloc_coherent(NULL, MMAP_SIZE, &shm_dma, GFP_KERNEL);
	if (shm_vir == NULL) {
		return -ENOMEM;
	}

	rc = fsg_alloc();
	if (rc != 0) {
		dma_free_coherent(NULL, MMAP_SIZE, shm_vir, shm_dma);
		return rc;
	}

	fsg = the_fsg;
	fsg->gadget = save_gadget;
	fsg->ep0 = save_gadget->ep0;

	/* INFO(the_fsg, "open physical addr is 0x%x,size is %ld bytes \n",
	   the_fsg->shm_dma, MMAP_SIZE); */
	return 0;
}

static int msc_release(struct inode *inode, struct file *file)
{
	if (shm_dma)
		dma_free_coherent(NULL, MMAP_SIZE, shm_vir, shm_dma);
	return 0;
}

static inline void set_scsi_cmd(u8 *cmnd, bool true)
{
	struct ShareInfo *pShi = (struct ShareInfo *)shm_vir;
	if (true) {
		pShi->scsi = cmnd[0];
		pShi->lun = cmnd[1] >> 5;
	} else {
		pShi->scsi = -1;
	}
}

static int msc_mmap(struct file *file, struct vm_area_struct *vma)
{
	size_t size = vma->vm_end - vma->vm_start;
	struct fsg_dev *fsg = the_fsg;

	if (size != MMAP_SIZE) {
		ERROR(fsg, "mmap size must be %ld\n ", MMAP_SIZE);
		return -EINVAL;
	}
	if (vma->vm_pgoff != (shm_dma >> PAGE_SHIFT)) {
		ERROR(fsg, "offset should be %d,now is %ld \n",
		      shm_dma >> PAGE_SHIFT, vma->vm_pgoff);
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff, size, vma->vm_page_prot)) {
		return -EAGAIN;
	}

	return 0;
}

/* same as fsg_cleanup */
static void fsg_exit(struct fsg_dev *fsg)
{
	test_and_clear_bit(REGISTERED, &fsg->atomic_bitflags);
	fsg_unbind(fsg->gadget);
	/* Wait for the thread to finish up */
	wait_for_completion(&fsg->thread_notifier);
	close_all_backing_files(fsg);
	kref_put(&fsg->ref, fsg_release);
}

static int
msc_ioctl(struct inode *inode, struct file *filp, u_int cmd, u_long arg)
{
	int err = 0;
	struct fsg_dev *fsg = the_fsg;
	usr_msc_info msc_info;

	switch (cmd) {
	case USR_MSC_INFO:
		msc_info.pa = (u32) shm_dma;
		msc_info.map_size = (u32) MMAP_SIZE;
		if (copy_to_user((void *)arg, &msc_info, sizeof(usr_msc_info))) {
			err = -EFAULT;
		}
		break;
	case USR_MSC_BIND:
		get_para_from_shm(shm_vir);
		fsg_bind(fsg->gadget);
		break;
	case USR_MSC_UNBIND:
		INFO(fsg, "recv ioctl USR_MSC_UNBIND");
		fsg_exit(fsg);
		break;
	case USR_MSC_RUN:
		/* USB_REQ_SET_CONFIGURATION of function standard_setup_req */
		fsg->new_config = arg;
		/* Raise an exception to wipe out previous transaction
		 * state (queued bufs, etc) and set the new config. */
		raise_exception(fsg, FSG_STATE_CONFIG_CHANGE);
		break;
	case USR_MSC_DISCONNECT:
		/* call ->disconnect() */
		fsg_disconnect(fsg->gadget);
		break;
	case USC_MSC_SET_INTERFACE:
		INFO(fsg, "set interface \n");
		raise_exception(fsg, FSG_STATE_INTERFACE_CHANGE);
		break;
	default:
		ERROR(fsg, "IOCTL_MSC_UNKNOWN cmd is 0x%x \n", cmd);
		err = -ENOIOCTLCMD;
		break;
	}

	return err;
}

static struct file_operations msc_fops = {
	.owner = THIS_MODULE,
	.open = msc_open,
	.release = msc_release,
	.ioctl = msc_ioctl,
	.mmap = msc_mmap,
};

static int create_msc_inode(void)
{
	int rc = -1;
	dev_t dev;

	msc_major = 0;
	msc_minor = 0;

	/* first, create a dev node */
	if (msc_major) {
		dev = MKDEV(msc_major, msc_minor);
		rc = register_chrdev_region(dev, 1, MSC_NAME);
	} else {
		rc = alloc_chrdev_region(&dev, msc_minor, 1, MSC_NAME);
		msc_major = MAJOR(dev);
	}

	if (rc) {
		goto out;
	}

	/* 2th, register this device */
	cdev_init(&msc_cdev, &msc_fops);
	rc = cdev_add(&msc_cdev, dev, 1);
	if (rc) {
		printk("can't add this device /dev/%s \n", MSC_NAME);
		goto unregister_chrdev;
	}

	rc = -1;
	/* 3th create this device */
	msc_class = class_create(THIS_MODULE, MSC_CLASS_NAME);
	if (IS_ERR(msc_class)) {
		goto unregister_cdev;
	}

	msc_dev = device_create(msc_class, NULL, dev, MSC_NAME);
	if (IS_ERR(msc_dev)) {
		goto unregister_class;
	}
	return 0;

      unregister_class:
	class_destroy(msc_class);
      unregister_cdev:
	cdev_del(&msc_cdev);
      unregister_chrdev:
	unregister_chrdev_region(dev, 1);
      out:
	return rc;
}

static void destroy_msc_inode(void)
{
	dev_t dev;

	dev = MKDEV(msc_major, msc_minor);
	device_destroy(msc_class, dev);
	class_destroy(msc_class);
	cdev_del(&msc_cdev);
	unregister_chrdev_region(dev, 1);
}

/* only for saving the gadget pointer */
static int fsg_bind_fail(struct usb_gadget *gadget)
{
	save_gadget = gadget;
	return -1;
}
static void fsg_unbind_fail(struct usb_gadget *gadget)
{
	save_gadget = NULL;
}

static void get_para_from_shm(struct ShareInfo *pShi)
{
	int i;

	mod_data.removable = pShi->removable;
	mod_data.nluns = pShi->numFileNames;
	for (i = 0; i < mod_data.nluns; i++) {
		mod_data.ro[i] = pShi->readOnly[i];
		mod_data.file[i] = &pShi->szFile[i][0];
	}
	memcpy(&fs_bulk_out_desc, &pShi->bulk_out, USB_DT_ENDPOINT_SIZE);
	memcpy(&fs_bulk_in_desc, &pShi->bulk_in, USB_DT_ENDPOINT_SIZE);
	/* memcpy(&fs_intr_in_desc, &pShi->intr_in,USB_DT_ENDPOINT_SIZE); */

}
