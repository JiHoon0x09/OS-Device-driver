/*
 *  linux/drivers/sound/s3c2440-ac97.c -- AC97 interface for the S3C2440
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *  
 *  2004-02-23 kwanghyun La <laputa: nala.la@samsung.com>
 *  -- update register address for s3c2440A
 *  
 *  2004-07-14 Jaswinder Singh Brar <Jassi: jassi@samsung.com>
 *  -- support for MIC-In and Line-In.
 *  -- support to switch between recording sources, _while_ recording.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/sound.h>
#include <linux/soundcard.h>
#include <linux/ac97_codec.h>
#include <linux/pm.h>
#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <asm/uaccess.h>
#include <asm/io.h>
#include <asm/hardware.h>
#include <asm/semaphore.h>
#include <asm/dma.h>
#include <asm/arch/dma.h>
#include <asm/arch/regs-ac97.h>
#include <asm/arch/regs-gpio.h>
//#include <asm/hardware/clock.h>
#include <asm/arch/regs-clock.h>

#include "s3c2440-audio.h"

#define GDEBUG
#ifdef  GDEBUG
#    define dprintk(fmt, args...) printk(KERN_INFO "%s: " fmt, __FUNCTION__ , ## args)
#else
#    define dprintk( x... )
#endif

#define AC97_REG_NAME		"ac97_reg"
#define AC97_REG_MAJOR		251

static struct completion CAR_completion;
static int      waitingForMask = AC_GLBSTAT_RI;
static          DECLARE_MUTEX(CAR_mutex);

static          DECLARE_MUTEX(elfin_ac97_mutex);
static int      elfin_ac97_refcount;

static audio_stream_t ac97_audio_out;


static unsigned short elfin_ac97_read(struct ac97_codec *codec, unsigned char reg);
static void elfin_ac97_write(struct ac97_codec *codec, u8 reg, u16 val);
static void elfin_ac97_wait(struct ac97_codec *codec);


static struct ac97_codec elfin_ac97_codec = {
    .codec_read = elfin_ac97_read,
    .codec_write = elfin_ac97_write,
    .codec_wait = elfin_ac97_wait,
};

/**********************************************************************
* 	For procfs debugging Start
**********************************************************************/

#ifdef CONFIG_PROC_FS
#define CS4299_PROC_NAME	"alc203"

static struct proc_dir_entry *cs4299_dir = NULL;
static struct proc_dir_entry *registers_dir = NULL;


typedef struct cs4299_reg_entry
{
    char           *name;
    int             offset;
} cs4299_reg_entry_t;

static cs4299_reg_entry_t cs4299_registers[] = {
    {"00-AC97_RESET", 0x00},
    {"02-AC97_MASTER_VOL_STEREO", 0x02},
    {"04-AC97_HEADPHONE_VOL", 0x04},
    {"06-AC97_MASTER_VOL_MONO", 0x06},
    {"0A-AC97_PCBEEP_VOL", 0x0A},
    {"0C-AC97_PHONE_VOL", 0x0C},
    {"0E-AC97_MIC_VOL", 0x0E},
    {"10-AC97_LINEIN_VOL", 0x10},
    {"12-AC97_CD_VOL", 0x12},
    {"14-AC97_VIDEO_VOL", 0x14},
    {"16-AC97_AUX_VOL", 0x16},
    {"18-AC97_PCMOUT_VOL", 0x18},
    {"1A-AC97_RECORD_SELECT", 0x1A},
    {"1C-AC97_RECORD_GAIN", 0x1C},
    {"1E-AC97_MIC_RECORD_GAIN", 0x1E},    
    {"20-AC97_GENERAL_PURPOSE", 0x20},
    {"22-AC97_3D_CONTROL", 0x22},
    {"26-AC97_POWER_CONTROL", 0x26},
    {"28-AC97_EXTENDED_AUDIO_ID", 0x28},
    {"2A-AC97_EXTENDED_STATUS", 0x2A},
    {"2C-AC97_DAC_RATE", 0x2C},
    {"32-AC97_ADC_RATE", 0x32},
    {"34-AC97_MIC_RATE", 0x34},
    {"5E-AC97_AC_MODE_CONTROL", 0x5E},
    {"60-AC97_SPDIF_low", 0x60},
    {"62-AC97_SPDIF_high", 0x62},
    {"6A-AC97_DATA_FLOW", 0x6A},    
    {"7C-VENDOR-ID1", 0x7C},
    {"7E-VENDOR-ID2", 0x7E},
};



int
cs4299_register_proc_entry(char *name, read_proc_t * _read)
{
    return create_proc_read_entry(name, 0, cs4299_dir, _read,
                                  NULL) ? 0 : -EINVAL;
}

void
cs4299_unregister_proc_entry(char *name)
{
    remove_proc_entry(name, cs4299_dir);
}


// see fs/proc/generic.c proc_file_read()
static ssize_t
cs4299_proc_read_reg(struct file *file, char *buf, size_t nbytes, loff_t * ppos)
{
    struct ac97_codec *codec = &elfin_ac97_codec;
    struct inode   *my_inode = file->f_dentry->d_inode;
    struct proc_dir_entry *dp;
    cs4299_reg_entry_t *current_reg = NULL;
    char            outputbuf[15];
    int             count;

    if (*ppos > 0)
        return 0;

    dp = PDE(my_inode);
    current_reg = (cs4299_reg_entry_t *) (dp->data);

//dprintk("dp = %p, data = %p\n", dp, dp->data);

    if (current_reg == NULL)
        return -EINVAL;

//return 0;

    count = sprintf(&outputbuf[0], "0x%02x: 0x%04x\n", current_reg->offset, elfin_ac97_read(codec, current_reg->offset));

    *ppos += count;

    if (count > nbytes)
        return -EINVAL;
    if (copy_to_user(buf, &outputbuf[0], count))
        return -EFAULT;

    return count;
}



static ssize_t
cs4299_proc_write_reg(struct file *file, const char *buffer,
                      size_t count, loff_t * ppos)
{
    struct ac97_codec *codec = &elfin_ac97_codec;
    struct inode   *my_inode = file->f_dentry->d_inode;
    struct proc_dir_entry *dp;
    cs4299_reg_entry_t *current_reg = NULL;
    unsigned long   new_reg_value;
    char           *endp;

    dp = PDE(my_inode);
    current_reg = (cs4299_reg_entry_t *) (dp->data);

    if (current_reg == NULL)
        return -EINVAL;

    //new_reg_value = simple_strtoul(buffer, &endp, 0); // base == 0, 10진수
    new_reg_value = simple_strtoul(buffer, &endp, 16);  // hex convert
    elfin_ac97_write(codec, current_reg->offset, new_reg_value & 0xFFFF);
    return (count + endp - buffer);
}

static struct file_operations cs4299_proc_reg_fops = {
    .read = cs4299_proc_read_reg,
    .write = cs4299_proc_write_reg,
};

void inline
cs4299_create_proc_interface(void)
{
    struct proc_dir_entry *entry;
    int             i;

    cs4299_dir = proc_mkdir(CS4299_PROC_NAME, proc_root_driver);
    registers_dir = proc_mkdir("registers", cs4299_dir);

    for (i = 0; i < ARRAY_SIZE(cs4299_registers); i++)
    {
        entry = create_proc_entry(cs4299_registers[i].name,
                                  S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH,
                                  registers_dir);
        entry->proc_fops = &cs4299_proc_reg_fops;
        entry->data = &cs4299_registers[i];
        //dprintk("entry = %p, data = %p\n", entry, entry->data);
    }
}

void inline
cs4299_destroy_proc_interface(void)
{
    int             i;

    for (i = 0; i < ARRAY_SIZE(cs4299_registers); i++)
        remove_proc_entry(cs4299_registers[i].name, registers_dir);
    remove_proc_entry("registers", cs4299_dir);

    remove_proc_entry(CS4299_PROC_NAME, proc_root_driver);
}
#else
#        define cs4299_create_proc_interface() (void)(0)
#        define cs4299_destroy_proc_interface() (void)(0)
#endif


/***************************************************************************************
* For procfs Debugging end
***************************************************************************************/


static unsigned short
elfin_ac97_read(struct ac97_codec *codec, unsigned char reg)
{
    unsigned int    ___stat;
    unsigned char   ___addr;
    unsigned short  ___data;

//dprintk("read 0\n");

    down(&CAR_mutex);

//dprintk("read 1\n");

    init_completion(&CAR_completion);
    waitingForMask = AC_GLBSTAT_RI;
    AC_CODEC_CMD = AC_CMD_R | AC_CMD_ADDR(reg);
    udelay(50);
    AC_GLBCTRL |= AC_GLBCTRL_RIE;
    
//dprintk("AC_GLBCTRL = 0x%08x\n", AC_GLBCTRL);
    
    wait_for_completion(&CAR_completion);
    
    udelay(50);


    ___stat = AC_CODEC_STAT;
    ___addr = (___stat >> 16) & 0x7f;
    ___data = (___stat & 0xffff);

    if (___addr != reg)
    {
        printk("req addr = %02x, rep addr = %02x, data = 0x%04x\n", reg, ___addr, ___data);
    }

//dprintk("read 2\n");
    up(&CAR_mutex);
    

    return ___data;
}

static void
elfin_ac97_write(struct ac97_codec *codec, u8 reg, u16 val)
{
    down(&CAR_mutex);

dprintk("codec_write...................reg: 0x%02x, 0x%04x\n", reg, val);

    init_completion(&CAR_completion);
    waitingForMask = AC_GLBSTAT_RI;
    AC_CODEC_CMD = AC_CMD_ADDR(reg) | AC_CMD_DATA(val);
    udelay(50);
    AC_GLBCTRL |= AC_GLBCTRL_RIE;
    wait_for_completion(&CAR_completion);
    AC_CODEC_CMD |= AC_CMD_R;   /* By default it shud be read enabled. No? */
    up(&CAR_mutex);
}

static void
elfin_ac97_wait(struct ac97_codec *codec)
{
	u32 ret;

    while( 1 )
    {
    	ret = AC_GLBSTAT_CSTAT();
    	dprintk("cstat = 0x%x\n", ret);
    	if( ret != AC_CSTAT_ACTIVE )
    	//if( ret != AC_CSTAT_READY )
        	schedule();
        else
        	break;
    }
}

static irqreturn_t 
elfin_ac97_isr(int irq, void *dev_id, struct pt_regs *regs)
{
    int             gsr = AC_GLBSTAT & waitingForMask;

dprintk("1\n");
    if (gsr)
    {
dprintk("2\n");    	
        AC_GLBCTRL &= ~AC_GLBCTRL_RIE;
        complete(&CAR_completion);
    }
    
	return IRQ_HANDLED;    
}


static void
codec_reset_settings(int mode)
{
	dprintk("...........reset\n");
    switch (mode)
    {

    case WRITE_START:

        //elfin_ac97_write(NULL, AC97_MASTER_VOL_STEREO, 0x0A0a); // 상관있음.
        elfin_ac97_write(NULL, AC97_MASTER_VOL_STEREO, 0x8000); // 상관있음. mute	modified by taekki
        elfin_ac97_write(NULL, AC97_HEADPHONE_VOL, 0x8000); // 상관없음
        elfin_ac97_write(NULL, AC97_MASTER_VOL_MONO, 0x8000); // 상관없음
        //elfin_ac97_write(NULL, AC97_PCMOUT_VOL, 0x0404);    /* +6dB gain */
        //elfin_ac97_write(NULL, AC97_PCMOUT_VOL, 0x0a0a);    /* +6dB gain */ // --> gmixer control
        elfin_ac97_write(NULL, AC97_PCMOUT_VOL, 0x8000);    // off	modified by taekki
        
        // 없는기능....alc203에는
        //elfin_ac97_write(NULL, 0x76, 0xABBA);   /* 0x78 enable */
        //elfin_ac97_write(NULL, 0x78, 0x0000);   /* ADC High Pass Filter -> ON */
        
        break;

    case READ_START:
    	//printk("reset..........READ_START\n");
		// micin        
        elfin_ac97_write(NULL, AC97_RECORD_SELECT, 0x0000); /* MIC -> Left + Right */
        //elfin_ac97_write(NULL, AC97_MIC_VOL, 0x0040);   /* +0dB gain + Boost */
        elfin_ac97_write(NULL, AC97_MIC_VOL, 0x0000);   /* +0dB gain + no boost */
        elfin_ac97_write(NULL, AC97_LINEIN_VOL, 0x0000);    /* +0dB gain */
        elfin_ac97_write(NULL, AC97_RECORD_GAIN, 0x0000);

        break;

    case WRITE_RELEASE:
        elfin_ac97_write(NULL, AC97_MASTER_VOL_STEREO, 0x8000); /* Mute */
        elfin_ac97_write(NULL, AC97_HEADPHONE_VOL, 0x8000);
        elfin_ac97_write(NULL, AC97_MASTER_VOL_MONO, 0x8000);
        elfin_ac97_write(NULL, AC97_PCMOUT_VOL, 0x08000);

		// 없는기능....alc203에는
        //elfin_ac97_write(NULL, 0x76, 0xABBA);   /* 0x78 enable */
        //elfin_ac97_write(NULL, 0x78, 0x0001);   /* ADC High Pass Filter -> OFF */
        break;

    case READ_RELEASE:
        elfin_ac97_write(NULL, AC97_MIC_VOL, 0x8000);   /* Mute */
        elfin_ac97_write(NULL, AC97_LINEIN_VOL, 0x8000);    /* Mute */
        /*
         * Gain Left and Right = +12dB 
         */
        elfin_ac97_write(NULL, AC97_RECORD_GAIN, 0x8000);
        break;
    default:
    	break;
    }
    elfin_ac97_write(NULL, AC97_EXTENDED_STATUS, 0x0001);   /* Variable Rate
                                                             * Enabled (ADC and 
                                                             * DAC) */
}


int
elfin_ac97_get(struct ac97_codec **codec)
{
    int             ret = 0;
	struct clk      *clk_ac97 = clk_get(NULL, "ac97");    

dprintk("1\n");
    *codec = NULL;
    down(&elfin_ac97_mutex);
    if (!elfin_ac97_refcount)
    {
        //CLKCON |= CLKCON_AC97;
dprintk("2\n");        
        clk_enable( clk_ac97 );
        
dprintk("3\n");                

        AC_GLBCTRL = AC_GLBCTRL_COLD;
        udelay(1000);
        AC_GLBCTRL = 0;
        udelay(1000);
       
        AC_GLBCTRL |= AC_GLBCTRL_AE;
        udelay(1000);

        AC_GLBCTRL |= AC_GLBCTRL_TE;
        udelay(1000);
        
#if 0        
        AC_GLBCTRL &= ~AC_GLBCTRL_RIE;
#endif        

        /*
         * Enable both always 
         */
        //AC_GLBCTRL |= AC_GLBCTRL_POMODE_DMA | AC_GLBCTRL_PIMODE_DMA | AC_GLBCTRL_MIMODE_DMA;

dprintk("4\n");
		AC_GLBCTRL &= ~(3<<12); //PCM Out Transfer Mode Off          
        ret = request_irq(IRQ_S3C2440_AC97, elfin_ac97_isr, SA_SHIRQ, "AC97", "ac97");
        if (ret)
        {
        	printk("reqeust_irq error\n");
            goto out;
        }

dprintk("5\n");                
        ret = ac97_probe_codec(&elfin_ac97_codec);
        if (ret != 1)
        {
            free_irq(IRQ_S3C2440_AC97, NULL);
            AC_GLBCTRL = 0;
            //CLKCON &= ~CLKCON_AC97;
            clk_disable( clk_ac97 );
            goto out;
        }
        ret = 0;
    }

    elfin_ac97_refcount++;

  out:
    up(&elfin_ac97_mutex);
    *codec = &elfin_ac97_codec;

    return ret;
}

void
elfin_ac97_put(void)
{
	struct clk      *clk_ac97 = clk_get(NULL, "ac97");    
	
    down(&elfin_ac97_mutex);
    elfin_ac97_refcount--;
    if (!elfin_ac97_refcount)
    {
        AC_GLBCTRL = 0;
        //CLKCON &= ~CLKCON_AC97;
        clk_disable( clk_ac97 );
        free_irq(IRQ_S3C2440_AC97, NULL);
    }
    up(&elfin_ac97_mutex);
}

EXPORT_SYMBOL(elfin_ac97_get);
EXPORT_SYMBOL(elfin_ac97_put);

static audio_state_t ac97_audio_state;

static int
mixer_ioctl(struct inode *inode, struct file *file,
            unsigned int cmd, unsigned long arg)
{
    int             ret;

    ret = elfin_ac97_codec.mixer_ioctl(&elfin_ac97_codec, cmd, arg);
    if (ret)
        return ret;

    return 0;
}



static struct file_operations mixer_fops = {
  ioctl:mixer_ioctl,
  llseek:no_llseek,
  owner:THIS_MODULE
};

static int      codec_adc_rate;
static int      codec_dac_rate;

static int
ac97_ioctl(struct inode *inode, struct file *file,
           unsigned int cmd, unsigned long arg)
{
    int             ret;
    long            val = 0;

    switch (cmd)
    {
    case SNDCTL_DSP_STEREO:
        if (get_user(val, (int *) arg))
            return -EINVAL;
        ac97_audio_out.channels = (val) ? 2 : 1;
        return 0;

    case SNDCTL_DSP_CHANNELS:
        if (get_user(val, (int *) arg))
            return -EINVAL;
        if (val != 1 && val != 2)
            return -EINVAL;
        ac97_audio_out.channels = val;
        return put_user(val, (int *) arg);

    case SOUND_PCM_READ_CHANNELS:
        return put_user(ac97_audio_out.channels, (long *) arg);

    case SNDCTL_DSP_SPEED:
        ret = get_user(val, (long *) arg);
        if (ret)
            return ret;
        if (file->f_mode & FMODE_READ)
            codec_adc_rate = ac97_set_adc_rate(&elfin_ac97_codec, val);
        if (file->f_mode & FMODE_WRITE)
            codec_dac_rate = ac97_set_dac_rate(&elfin_ac97_codec, val);
        /*
         * fall through 
         */

    case SOUND_PCM_READ_RATE:
        if (file->f_mode & FMODE_READ)
            val = codec_adc_rate;
        if (file->f_mode & FMODE_WRITE)
            val = codec_dac_rate;
        return put_user(val, (long *) arg);

    case SNDCTL_DSP_SETFMT:
    case SNDCTL_DSP_GETFMTS:
        /*
         * FIXME: can we do other fmts? 
         */
        return put_user(AFMT_S16_LE, (long *) arg);

    default:
        /*
         * Maybe this is meant for the mixer (As per OSS Docs) 
         */
        return mixer_ioctl(inode, file, cmd, arg);
    }
    return 0;
}

static audio_stream_t ac97_audio_in = {
#ifdef MIC_IN_MODE
  name:"AC97MICIN",
  other_name:"AC97PCMIN",
  dma_ch:S3C2410_DMA_CH3,      /* MICIN */
  other_dma_ch:S3C2410_DMA_CH2,        /* PCMIN */
#else
  name:"AC97PCMIN",
  other_name:"AC97MICIN",
  dma_ch:S3C2410_DMA_CH2,
  other_dma_ch:S3C2410_DMA_CH3,
#endif
};

static audio_stream_t ac97_audio_out = {
  name:"AC97PCMOUT",           /* name is now the channel name itself */
  dma_ch:S3C2410_DMA_CH1,
};

static audio_state_t ac97_audio_state = {
  output_stream:&ac97_audio_out,
  input_stream:&ac97_audio_in,
  read_busy:FLAG_DOWN,         /* Initialize the flag */
  output_id:"S3C24x0 AC97 out",
  need_tx_for_rx:1,
  client_ioctl:ac97_ioctl,
  reset_conf:codec_reset_settings,
    /*
     * resets our codec 
     */
  //sem:__MUTEX_INITIALIZER(ac97_audio_state.sem),
  sem: __SEMAPHORE_INIT(ac97_audio_state.sem,1),  
};

static int
ac97_audio_open(struct inode *inode, struct file *file)
{
	//u16 val;
		
    return elfin_audio_attach(inode, file, &ac97_audio_state);
}


static struct file_operations ac97_audio_fops = {
    .open = ac97_audio_open,
    .release = s3c2440_audio_release,
    .write = s3c2440_audio_write,
    .read = s3c2440_audio_read,
    .poll = s3c2440_audio_poll,
    .ioctl = s3c2440_audio_ioctl,
    .llseek = s3c2440_audio_llseek,
    .owner = THIS_MODULE,
};



struct h_ac97
{
    unsigned int    reg;
    unsigned int    val;
};


#define S3C2440_AC97_REG_WRITE          _IOW('f', 0xf4, struct h_ac97)
#define S3C2440_READ_AC_GLBCTRL         _IO('f', 0xf5)

static int __init
elfin_ac97_port_init(void)
{
#if 1 // 아예 이 루틴을 없애야 제대로 읽힌다. 지랄....ALT 셑팅이 이 값으로하면 잘못됨..지랄

    /* GPE 0: AC_SYNC */
    s3c2410_gpio_cfgpin( S3C2410_GPE0, S3C2440_GPE0_ACSYNC);
    s3c2410_gpio_pullup( S3C2410_GPE0, 0); 
    /* GPE 1: AC_BIT_CLK */
    s3c2410_gpio_cfgpin( S3C2410_GPE1, S3C2440_GPE1_ACBITCLK);
    s3c2410_gpio_pullup( S3C2410_GPE1, 0); 
    /* GPE 2: AC_nRESET */
    s3c2410_gpio_cfgpin( S3C2410_GPE2, S3C2440_GPE2_ACRESET);
    s3c2410_gpio_pullup( S3C2410_GPE2, 0); 
    /* GPE 3: AC_SDATA_IN */
    s3c2410_gpio_cfgpin( S3C2410_GPE3, S3C2440_GPE3_ACIN);
    s3c2410_gpio_pullup( S3C2410_GPE3, 0); 
    /* GPE 4: AC_SDATA_OUT */
    s3c2410_gpio_cfgpin( S3C2410_GPE4, S3C2440_GPE4_ACOUT);
    s3c2410_gpio_pullup( S3C2410_GPE4, 0); 

#endif
}

static int __init
elfin_ac97_init(void)
{
    int             ret;
    struct ac97_codec *codec;

    elfin_ac97_port_init();
    
dprintk("1\n");

    ret = elfin_ac97_get(&codec);
    if (ret)
        return ret;
dprintk("2\n");

    ac97_audio_state.dev_dsp = register_sound_dsp(&ac97_audio_fops, -1);
    elfin_ac97_codec.dev_mixer = register_sound_mixer(&mixer_fops, -1);

dprintk("3\n");
    /*
     * by Pain 
     */
    //flush_scheduled_tasks();
    init_waitqueue_head(&ac97_audio_state.rbq); /* For read_busy */
    codec_reset_settings(WRITE_START);
    codec_reset_settings(READ_START);
    
	cs4299_create_proc_interface();    
    
dprintk("4\n");    
    return 0;
}

static void __exit
elfin_ac97_exit(void)
{
    unregister_sound_dsp(ac97_audio_state.dev_dsp);
    unregister_sound_mixer(elfin_ac97_codec.dev_mixer);
    elfin_ac97_put();
}

// add by taekki start
void master_volume_on(void)
{
	elfin_ac97_write(NULL, AC97_MASTER_VOL_STEREO, 0x0A0a); // 상관있음.
	elfin_ac97_write(NULL, AC97_PCMOUT_VOL, 0x0a0a);    /* +6dB gain */ // --> gmixer control
}

void master_volume_off(void)
{
	elfin_ac97_write(NULL, AC97_MASTER_VOL_STEREO, 0x8000);
	elfin_ac97_write(NULL, AC97_PCMOUT_VOL, 0x8000);
}

EXPORT_SYMBOL(master_volume_on);
EXPORT_SYMBOL(master_volume_off);
// add by taekki end

module_init(elfin_ac97_init);
module_exit(elfin_ac97_exit);
