#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <asm/ioctls.h>
#include <asm/memory.h>
#include <linux/mm.h>
#include <linux/namei.h>
#include <linux/file.h>
#include <linux/delay.h>

#include <sound/driver.h>
#include <sound/core.h>
#include <sound/control.h>
#include <sound/info.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/timer.h>
#include <sound/minors.h>

static atomic_t open_count = ATOMIC_INIT(0);

static struct file *_file;
static struct snd_pcm_file *_pcm_file;
static struct snd_pcm_substream *_substream;

extern int snd_pcm_hw_params(struct snd_pcm_substream *substream, struct snd_pcm_hw_params *params);
extern int snd_pcm_sw_params(struct snd_pcm_substream *substream, struct snd_pcm_sw_params *params);
extern int snd_pcm_prepare(struct snd_pcm_substream *substream, struct file *file);
extern int snd_pcm_sync_ptr(struct snd_pcm_substream *substream, struct snd_pcm_sync_ptr *_sync_ptr);
extern int snd_pcm_stop(struct snd_pcm_substream *substream, int state);

static struct timer_list timer;
static int stopped;

static void eac_timer(unsigned long data)
{
	snd_pcm_stop(_substream, SNDRV_PCM_STATE_PREPARED);
	stopped = 1;
}

static ssize_t eac_read(struct file *fp, char __user *buf, size_t count, loff_t *pos)
{
	return -ENODEV;
}

static ssize_t eac_write(struct file *fp, const char __user *buf, size_t count, loff_t *pos)
{
	int res = count;
	int off = 0;

	del_timer(&timer);
	if (stopped) {
		snd_pcm_prepare(_substream, _file);
		stopped = 0;
	}
	while (res > 0) {
		int size = res;
		if (size > 4096) {
			size = 4096;
		}
		snd_pcm_lib_write(_substream, buf + off, size / 4);

		msleep(1);

		res -= size;
		off += size;
	}

	mod_timer(&timer, jiffies + HZ * count * 8 / 32 / 44100 + 1);

	return count;
}

static struct file *_open(const char *name)
{
	struct nameidata nd;
	int err;
	struct file *file;

	err = path_lookup_open(AT_FDCWD, name, LOOKUP_FOLLOW, &nd, FMODE_READ| FMODE_WRITE);
	file = ERR_PTR(err);

	if (!err) {
		int err = vfs_permission(&nd, MAY_WRITE);

		file = ERR_PTR(err);
		if (!err) {
			file = nameidata_to_filp(&nd, O_RDWR);
out:
			return file;
		}
		release_open_intent(&nd);
		path_put(&nd.path);
	}
	goto out;
}

uint intervals[] = {
	0x00000010, 0x00000010, 0x00000004,
	0x00000020, 0x00000020, 0x00000004,
	0x00000002, 0x00000002, 0x00000004, /* 2 channels */
	0x0000ac44, 0x0000ac44, 0x00000004, /* 44.1 Khz */
	0x0000880C, 0x0000880E, 0x00000003,
	0x00000600, 0x00000600, 0x00000004, /* period size 1536B */
	0x00001800, 0x00001800, 0x00000004, /* period bytes 6k */
	0x00000004, 0x00000004, 0x00000004, /* 4 periods */
	0x00022037, 0x00022039, 0x00000003, /* btime = bsize * 10^6 / rate */
	0x00001800, 0x00001800, 0x00000004, /* buffer size 6k */
	0x00006000, 0x00006000, 0x00000004, /* buffer bytes 24k */
	0x00000000, 0x00000000, 0x00000004,
};

struct snd_pcm_hw_params hw_params = {
	.flags = 0x00000000,
	.masks[0].bits[0] = 0x00000008,
	.masks[1].bits[0] = 0x00000004,
	.masks[2].bits[0] = 0x00000001,
	.rmask = 0x00000000,
	.cmask = 0x000fff07,
	.info = 0x000d0103,
	.msbits = 0x00000010,
	.rate_num = 0x0000ac44,
	.rate_den = 0x00000001,
	.rate_den = 0x00000001,
	.fifo_size = 0x00000000,
};

struct snd_pcm_sw_params sw_params = {
	.period_step = 0x00000001,
	.avail_min = 0x00000600,
	.xfer_align = 0x00000600,
	.start_threshold = 0x00001800,
	.stop_threshold = 0x40000000,
	.boundary = 0x40000000,
};

static int eac_open(struct inode *ip, struct file *fp)
{
	int i;

	if (atomic_inc_return(&open_count) == 1)
	{
		_file = _open("/dev/pcmC0D0p");
		if (_file == NULL) {
			return -ENODEV;
		}
		_pcm_file = _file->private_data;
		_substream = _pcm_file->substream;
		for (i = 0; i < sizeof(intervals)/sizeof(intervals[0]); ++i) {
			((uint *)hw_params.intervals)[i] = intervals[i];
		}
		snd_pcm_hw_params(_substream, &hw_params);
		snd_pcm_sw_params(_substream, &sw_params);
		snd_pcm_prepare(_substream, _file);
		return 0;
	} else {
		atomic_dec(&open_count);
		return -EBUSY;
	}
}

static int eac_release(struct inode *ip, struct file* fp)
{
	atomic_dec(&open_count);
	return 0;
}

static int eac_ioctl(struct inode* ip, struct file* fp, unsigned int cmd, unsigned long arg)
{
	/* temporary workaround, until we switch to the ALSA API */
	if (cmd == 315)
		return -1;
	else
		return 0;
}

static struct file_operations eac_fops = {
	.owner = THIS_MODULE,
	.read = eac_read,
	.write = eac_write,
	.ioctl = eac_ioctl,
	.open = eac_open,
	.release = eac_release,
};

static struct miscdevice misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "eac",
	.fops = &eac_fops,
};

static int __init eac_init(void)
{
	init_timer(&timer);
        timer.function = eac_timer;

	return misc_register(&misc);
}

device_initcall(eac_init);
