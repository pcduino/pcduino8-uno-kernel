#ifndef __DEV_TV_H__
#define __DEV_TV_H__

#include "drv_tv_i.h"

#define 	SCREEN_COUNT 1
int tv_open(struct inode *inode, struct file *file);
int tv_release(struct inode *inode, struct file *file);
ssize_t tv_read(struct file *file, char __user *buf, size_t count, loff_t *ppos);
ssize_t tv_write(struct file *file, const char __user *buf, size_t count, loff_t *ppos);
int tv_mmap(struct file *file, struct vm_area_struct * vma);
long tv_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

extern s32 tv_init(void);
extern s32 tv_exit(void);


typedef struct
{
	disp_tv_dac_source   	dac_source[4];
	disp_tv_mode         	tv_mode;
	u32                   	base_address;
}tv_screen_t;


typedef struct
{
	struct device           	*dev;
    	u32                  		enable;
	u32 					dac_count;
	tv_screen_t			screen[2];
       struct work_struct      	hpd_work;
}tv_info_t;

extern tv_info_t g_tv_info;

#endif
