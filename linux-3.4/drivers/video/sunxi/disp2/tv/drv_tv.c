#include "drv_tv_i.h"
#include "dev_tv.h"

#include <linux/regulator/consumer.h>
#include <linux/clk-private.h>

#define TV_SOURCE 	"pll_de"
#define TVE_CLK	 	"tve"
extern s32 disp_set_tv_func(disp_tv_func * func);

static bool g_used;
bool g_dac_used;
bool g_suspend;
static struct mutex mlock;


static void tve_clk_enable(u32 sel);

static void tve_clk_init(u32 sel);

static void tve_clk_disable(u32 sel);

static void tve_clk_config(u32 sel, u32 tv_mode);

disp_video_timings video_timing[] =
{
	/*vic         tv_mode          PCLK   AVI    x     y    HT  HBP HFP HST VT VBP VFP VST*/
	{0,	DISP_TV_MOD_PAL,27000000, 0, 720, 576, 864, 137, 3,   2,  625, 20, 25, 2, 0, 0,1, 0, 0},
	{0,	DISP_TV_MOD_NTSC,13500000, 0,  720,   480,   858,   57,   19,   62,  525,   15,  4,  3,  0,   0,   1,   0,   0},
};

extern s32 disp_set_tv_func(disp_tv_func * func); //dev_disp.c

#if defined(CONFIG_SWITCH) || defined(CONFIG_ANDROID_SWITCH)

static struct task_struct * g_tve_task;
struct work_struct g_hpd_work;

u32 g_tv_hpd = 0;

static struct switch_dev cvbs_switch_dev = {
    .name = "cvbs",
};

static struct switch_dev svideo_switch_dev = {
    .name = "svideo",
};

static struct switch_dev ypbpr_switch_dev = {
    .name = "ypbpr",
};

static struct switch_dev vga_switch_dev = {
    .name = "vga",
};

void tv_report_hpd_work(struct work_struct *work)
{
	switch(g_tv_hpd)
    	{
		case DISP_TV_NONE:
        	{
			switch_set_state(&cvbs_switch_dev, 0);
	            	switch_set_state(&svideo_switch_dev, 0);
	            	switch_set_state(&ypbpr_switch_dev, 0);
	            	switch_set_state(&vga_switch_dev, 0);
	            	break;
	      }
		case DISP_TV_CVBS:
	       {	printk("[TV]report switch set cvbs");
	            	switch_set_state(&cvbs_switch_dev, 1);
	            	break;
	       }
	       case DISP_TV_SVIDEO:
	       {
	            	switch_set_state(&svideo_switch_dev, 1);
	            	break;
	       }
	      	case DISP_TV_YPBPR: {
	            	switch_set_state(&ypbpr_switch_dev, 1);
	            	break;
	       }
	       case 8: {
			switch_set_state(&vga_switch_dev, 1);
	            	break;
	       }
	       default: {
	        	switch_set_state(&cvbs_switch_dev, 0);
	            	switch_set_state(&svideo_switch_dev, 0);
	            	switch_set_state(&ypbpr_switch_dev, 0);
	            	switch_set_state(&vga_switch_dev, 0);
	            	break;
	       }
	}
}


s32 tv_detect_thread(void *parg)
{
	s32 hpd;
    	while(1) {
		if(kthread_should_stop()) {
			break;
		}
	       if(!g_suspend) {
			hpd = tv_get_dac_hpd(1);   //modify
	            	if(hpd != g_tv_hpd) {
				set_current_state(TASK_INTERRUPTIBLE);
	                	schedule_timeout(80);
	                	hpd = tv_get_dac_hpd(1);  //modify
	                	if(hpd != g_tv_hpd) {
	                    	g_tv_hpd = hpd;
	                    	schedule_work((struct work_struct*)&tv_report_hpd_work);
	                	}
	        	}
		}
	set_current_state(TASK_INTERRUPTIBLE);
	schedule_timeout(200);
	}
	return 0;
}

s32 tv_detect_enable(void)
{
	INIT_WORK(&g_hpd_work, tv_report_hpd_work);
    	switch_dev_register(&cvbs_switch_dev);
   	switch_dev_register(&ypbpr_switch_dev);
    	switch_dev_register(&svideo_switch_dev);
    	switch_dev_register(&vga_switch_dev);

   	 g_tve_task = kthread_create(tv_detect_thread, (void*)0, "tve detect");
    	if(IS_ERR(g_tve_task)) {
       	s32 err = 0;
    	 //wrn("Unable to start kernel thread %s.\n","tve detect");
        err = PTR_ERR(g_tve_task);
        g_tve_task = NULL;
        return err;
    	}
	wake_up_process(g_tve_task);
	return 0;
}

s32 tv_detect_disable(void)
{
	if(g_tve_task) {				//modify
		kthread_stop(g_tve_task);
		g_tve_task = 0;
    	}
    	return 0;
}
#else
void tv_report_hpd_work(struct work_struct *work)
{
	pr_debug("there is null report hpd work,you need support the switch class!");
}

s32 tv_detect_thread(void *parg)
{
	pr_debug("there is null tv_detect_thread,you need support the switch class!");
	return -1;
}

s32 tv_detect_enable(void)
{
	pr_debug("there is null tv_detect_enable,you need support the switch class!");
	return -1;
}

s32 tv_detect_disable(void)
{
	pr_debug("there is null tv_detect_disable,you need support the switch class!");
    	return -1;
}
#endif

s32 tv_get_dac_hpd(u32 sel)
{
	u8 dac[3] = {0};
    	s32 i = 0;
	u32  ret = DISP_TV_NONE;

 	for(i=0; i<g_tv_info.dac_count; i++) {
	       dac[i] = 1;//tve_low_get_dac_status(i); modify
	       if(dac[i]>1) {
	            //DE_WRN("dac %d short to ground\n", i);
	            dac[i] = 0;
	       }

	       if(g_tv_info.screen[sel].dac_source[i] == DISP_TV_DAC_SRC_COMPOSITE && dac[i] == 1) {
			ret |= DISP_TV_CVBS;
	       }
	       else if(g_tv_info.screen[sel].dac_source[i] == DISP_TV_DAC_SRC_Y && dac[i] == 1) {
	            	ret |= DISP_TV_YPBPR;
	       }
	       else if(g_tv_info.screen[sel].dac_source[i] == DISP_TV_DAC_SRC_LUMA && dac[i] == 1) {
	            	ret |= DISP_TV_SVIDEO;
	       }
	}
	printk("<4>[TV]dac source = %c short to ground\n",ret);
	return  ret;
}

s32 tv_get_video_info(s32 mode)
{
	s32 i,count;
	count = sizeof(video_timing)/sizeof(disp_video_timings);
	for(i=0;i<count;i++) {
		if(mode == video_timing[i].tv_mode)
			return i;
	}
	return -1;
}

s32 tv_get_list_num(void)
{
	return sizeof(video_timing)/sizeof(disp_video_timings);

}

s32 tv_init(void)
{
	disp_tv_func disp_func;
	s32 i = 0;
	script_item_u   val;
	script_item_value_type_e  type;
	char sub_key[20];

	g_suspend = 0;
	type = script_get_item("tv_para", "tv_used", &val);
	if(SCIRPT_ITEM_VALUE_TYPE_INT == type)
		g_used = val.val;
	if(g_used) {
		for(i=0; i <SCREEN_COUNT; i++) {
			tve_low_set_reg_base(i, g_tv_info.screen[i].base_address);
	    		tve_clk_init(i);
			tve_clk_enable(i);
	    		tve_low_init(i);
		}
		g_tv_info.screen[0].tv_mode = DISP_TV_MOD_PAL;
		g_tv_info.screen[1].tv_mode = DISP_TV_MOD_PAL;
		mutex_init(&mlock);

		type = script_get_item("tv_para", "tv_dac_used", &val);
		if(SCIRPT_ITEM_VALUE_TYPE_INT == type)
			g_dac_used = val.val;
		if(g_dac_used) {
			g_tv_info.dac_count = 1;
			for(i=0; i<4; i++) {						//modify i < 4
				sprintf(sub_key, "tv_dac_src%d", i);
		           	type = script_get_item("tv_para", sub_key, &val);
	            		if(SCIRPT_ITEM_VALUE_TYPE_INT == type) {				//modify
					g_tv_info.screen[0].dac_source[i] = val.val;		//modify   val.val;
					g_tv_info.screen[1].dac_source[i] = val.val;
					g_tv_info.dac_count++;			//modify to g_tv_info.dac_count = 1 for test
		            	}
				else {
					break;
	          		}
			}
		}
		tv_detect_enable();
		disp_func.tv_enable = tv_enable;
		disp_func.tv_disable = tv_disable;
		disp_func.tv_resume = tv_resume;
		disp_func.tv_suspend = tv_suspend;
		disp_func.tv_get_mode = tv_get_mode;
		disp_func.tv_set_mode = tv_set_mode;
		disp_func.tv_get_video_timing_info = tv_get_video_timing_info;
		disp_func.tv_get_input_csc = tv_get_input_csc;
		disp_func.tv_mode_support = tv_mode_support;
		disp_set_tv_func(&disp_func);
	}
		return 0;
}

s32 tv_exit(void)
{
	s32 sel;
	mutex_lock(&mlock);
	tv_detect_disable();
	mutex_unlock(&mlock);
	for(sel=0; sel <SCREEN_COUNT; sel++) {
		tv_disable(sel);
		tve_low_exit(sel);
	}
	return 0;
}


s32 tv_get_mode(u32 sel)
{
	 //add get mode from gtv

	 return g_tv_info.screen[sel].tv_mode;
}

s32 tv_set_mode(u32 sel, disp_tv_mode tv_mode)
{
	if(tv_mode >= DISP_TV_MODE_NUM) {
      		return -1;
    	}
    	mutex_lock(&mlock);
	g_tv_info.screen[sel].tv_mode = tv_mode;
	tve_low_set_tv_mode(sel, tv_mode);
	mutex_unlock(&mlock);
	return  0;
}

s32 tv_get_input_csc(void)
{
	return 1;   	//support yuv only
}

s32 tv_get_video_timing_info(u32 sel, disp_video_timings **video_info)
{
	disp_video_timings *info;
	int ret = -1;
	int i, list_num;
	info = video_timing;
	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		mutex_lock(&mlock);
		if(info->tv_mode == g_tv_info.screen[sel].tv_mode){
			*video_info = info;
			ret = 0;
			mutex_unlock(&mlock);
			break;
		}
		mutex_unlock(&mlock);
		info ++;
	}
	return ret;
}


s32 tv_enable(u32 sel)
{
	if(!g_tv_info.enable) {
		tve_clk_config(sel, g_tv_info.screen[sel].tv_mode);
		tve_clk_enable(sel);
		tve_low_set_tv_mode(sel, g_tv_info.screen[sel].tv_mode);
		tve_low_dac_cfg(sel, g_tv_info.screen[sel].tv_mode);
		tve_low_open(sel);
		mutex_lock(&mlock);
		g_tv_info.enable = 1;
		mutex_unlock(&mlock);
	}
	tv_low_print_base_reg();
	return 0;
}

s32 tv_disable(u32 sel)
{
	u32 i;
	mutex_lock(&mlock);
	if(g_tv_info.enable) {
		tve_clk_disable(sel);
		for(i=0; i<g_tv_info.dac_count; i++) {
			tve_low_dac_disable(sel, i);
		}
		tve_low_close(sel);
		g_tv_info.enable = 0;
	}
	mutex_unlock(&mlock);
	tv_low_print_base_reg();
	return 0;
}

s32 tv_suspend(void)
{
	mutex_lock(&mlock);
	if(g_used && (0 == g_suspend)) {
		g_suspend = true;
	}
	mutex_unlock(&mlock);

	return 0;
}

s32 tv_resume(void)
{
	mutex_lock(&mlock);
	if(g_used && (1 == g_suspend)) {
		g_suspend= false;
	}
	mutex_unlock(&mlock);

	return  0;
}

s32 tv_mode_support(disp_tv_mode mode)
{
	u32 i, list_num;
	disp_video_timings *info;


	info = video_timing;
	list_num = tv_get_list_num();
	for(i=0; i<list_num; i++) {
		if(info->tv_mode == mode) {
			return 1;
		}
		info ++;
	}
	return 0;
}

static void tve_clk_init(u32 sel)
{
	char clk[20] = {0};

	if(SCREEN_COUNT <= sel)
		sel = sel-1;
	sprintf(clk, "tve%d", sel);
	disp_sys_clk_set_parent(clk, TV_SOURCE);
}


static void tve_clk_enable(u32 sel)
{
	char clk[20] = {0};

	if(SCREEN_COUNT <= sel)
		sel = sel-1;
	sprintf(clk, "tve%d", sel);
	disp_sys_clk_enable(clk);
}

static void tve_clk_disable(u32 sel)
{
	char clk[20] = {0};

	if(SCREEN_COUNT <= sel)
		sel = sel-1;
	sprintf(clk, "tve%d", sel);
	disp_sys_clk_disable(clk);
}

static void tve_clk_config(u32 sel, u32 tv_mode)
{
	int index = 0;
	char clk[20] = {0};
	disp_video_timings *pinfo;
	if(SCREEN_COUNT <= sel)
		sel = sel-1;
	pinfo = video_timing;
	sprintf(clk, "tve%d", sel);
	index = tv_get_video_info(tv_mode);			//modify tv_mode

	disp_sys_clk_set_rate(clk, pinfo[index].pixel_clk);
}

