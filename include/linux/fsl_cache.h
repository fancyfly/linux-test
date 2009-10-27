#include <linux/types.h>

#define FSLCACHE_IOCINV		_IOR('c', 0, struct fsl_cache_addr)
#define FSLCACHE_IOCCLEAN 	_IOR('c', 1, struct fsl_cache_addr)
#define FSLCACHE_IOCFLUSH 	_IOR('c', 2, struct fsl_cache_addr)

struct fsl_cache_addr {
	void *start;
	void *end;
};
