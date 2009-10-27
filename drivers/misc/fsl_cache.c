#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fsl_cache.h>

#include <asm/cacheflush.h>


static int fsl_cache_ioctl(struct inode *ip, struct file *fp,
			   unsigned int cmd, unsigned long arg)
{
	struct fsl_cache_addr addr;
	copy_from_user(&addr, (void __user *)arg,
			sizeof(struct fsl_cache_addr));

	switch (cmd) {
	case FSLCACHE_IOCINV:
		dmac_inv_range(addr.start, addr.end);
		break;
	case FSLCACHE_IOCCLEAN:
		dmac_clean_range(addr.start, addr.end);
		break;
	case FSLCACHE_IOCFLUSH:
		dmac_flush_range(addr.start, addr.end);
		break;
	default:
		printk(KERN_ERR "fsl_cache ioctl: error cmd!\n");
		return -1;
	}
	return 0;
}

static const struct file_operations misc_fops = {
	.ioctl	= fsl_cache_ioctl,
};

static struct miscdevice fsl_cache_device  = {
	.minor	= MISC_DYNAMIC_MINOR,
	.name	= "fsl_cache",
	.fops	= &misc_fops,
};

static int __init fsl_cache_init(void)
{
	int error;

	error = misc_register(&fsl_cache_device);
	if (error) {
		printk(KERN_ERR "fsl_cache: misc_register failed!\n");
		return error;
	}
	return 0;
}

static void __exit fsl_cache_exit(void)
{
	misc_deregister(&fsl_cache_device);
}

module_init(fsl_cache_init);
module_exit(fsl_cache_exit);
