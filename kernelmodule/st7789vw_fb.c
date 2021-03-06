/*
 * linux/drivers/video/st7789vwfb.c -- FB driver for st7789vw LCD controller
 * Layout is based on skeletonfb.c by James Simmons and Geert Uytterhoeven.
 *
 * Copyright (C) 2011, Matt Porter
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/fb.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include "st7789vw_fb.h"


static const struct fb_fix_screeninfo st7789vwfb_fix  = {
    .id =		"st7789vw",
    .type =		FB_TYPE_PACKED_PIXELS,
    .visual =	FB_VISUAL_PSEUDOCOLOR,
    .xpanstep =	0,
    .ypanstep =	0,
    .ywrapstep =	0,
    .line_length =	WIDTH*BPP/8,
    .accel =	FB_ACCEL_NONE,
};

static const struct fb_var_screeninfo st7789vwfb_var = {
    .xres =			WIDTH,
    .yres =			HEIGHT,
    .xres_virtual =		WIDTH,
    .yres_virtual =		HEIGHT,
    .bits_per_pixel =	BPP,
    .nonstd	=		1,
};

static int st7789vw_write(struct st7789vwfb_par *par, u8 data)
{
    u8 txbuf[2]; /* allocation from stack must go */

    txbuf[0] = data;

    return spi_write(par->spi, &txbuf[0], 1);
}

static void st7789vw_write_data(struct st7789vwfb_par *par, u8 data)
{
    int ret = 0;

    /* Set data mode */
    gpio_set_value(par->dc, 1);

    ret = st7789vw_write(par, data);
    if (ret < 0)
        pr_err("%s: write data %02x failed with status %d\n",
               par->info->fix.id, data, ret);
}

static int st7789vw_write_data_buf(struct st7789vwfb_par *par,
                                   u8 *txbuf, int size)
{
    /* Set data mode */
    gpio_set_value(par->dc, 1);

    /* Write entire buffer */
    return spi_write(par->spi, txbuf, size);
}

static void st7789vw_write_cmd(struct st7789vwfb_par *par, u8 data)
{
    int ret = 0;

    /* Set command mode */
    gpio_set_value(par->dc, 0);

    ret = st7789vw_write(par, data);
    if (ret < 0)
        pr_err("%s: write command %02x failed with status %d\n",
               par->info->fix.id, data, ret);
}
static void mipi_dbi_command_stackbuf(struct st7789vwfb_par *dbi, u8 cmd, u8 *data, size_t len) {
    int i = 0;
    st7789vw_write_cmd(dbi, cmd);
    for(i = 0; i<len; i++) {
        st7789vw_write_data(dbi, data[i]);
    }

}
#define mipi_dbi_command(dbi, cmd, seq...) \
({ \
	u8 d[] = { seq }; \
	mipi_dbi_command_stackbuf(dbi, cmd, d, ARRAY_SIZE(d)); \
})


static void st7789vw_run_cfg_script(struct st7789vwfb_par *dbi)
{
    mipi_dbi_command(dbi,0x36, 0x70);

    mipi_dbi_command(dbi,0x3A,0x05);

    mipi_dbi_command(dbi,0xB2,0x0C,0x0C,0x00,0x33,0x33);

    mipi_dbi_command(dbi,0xB7,0x35);

    mipi_dbi_command(dbi,0xBB,0x19);

    mipi_dbi_command(dbi,0xC0,0x2C);

    mipi_dbi_command(dbi,0xC2,0x01);

    mipi_dbi_command(dbi,0xC3,0x12);

    mipi_dbi_command(dbi,0xC4,0x20);

    mipi_dbi_command(dbi,0xC6,0x0F);

    mipi_dbi_command(dbi,0xD0,0xA4,0xA1);

    mipi_dbi_command(dbi,0xE0,0xD0,0x04,0x0D,0x11,0x13,0x2B,0x3F,0x54,0x4C,0x18,0x0D,0x0B,0x1F,0x23);

    mipi_dbi_command(dbi,0xE1,0xD0,0x04,0x0C,0x11,0x13,0x2C,0x3F,0x44,0x51,0x2F,0x1F,0x1F,0x20,0x23);

    mipi_dbi_command(dbi,0x21);

    mipi_dbi_command(dbi,0x11);

    mipi_dbi_command(dbi,0x29);

    msleep(20);
}

static void st7789vw_set_addr_win(struct st7789vwfb_par *par,
                                  int xs, int ys, int xe, int ye)
{
    st7789vw_write_cmd(par, st7789vw_CASET);
    st7789vw_write_data(par, 0x00);
    st7789vw_write_data(par, xs+2);
    st7789vw_write_data(par, 0x00);
    st7789vw_write_data(par, xe+2);
    st7789vw_write_cmd(par, st7789vw_RASET);
    st7789vw_write_data(par, 0x00);
    st7789vw_write_data(par, ys+1);
    st7789vw_write_data(par, 0x00);
    st7789vw_write_data(par, ye+1);
}

static void st7789vw_reset(struct st7789vwfb_par *par)
{
    /* Reset controller */
    gpio_set_value(par->rst, 0);
    udelay(10);
    gpio_set_value(par->rst, 1);
    mdelay(120);
}

static void st7789vwfb_update_display(struct st7789vwfb_par *par)
{
    int ret = 0;
    u8 *vmem = par->info->screen_base;
#ifdef __LITTLE_ENDIAN
    int i;
    u16 *vmem16 = (u16 *)vmem;
    u16 *ssbuf = par->ssbuf;

    for (i=0; i<WIDTH*HEIGHT*BPP/8/2; i++)
        ssbuf[i] = swab16(vmem16[i]);
#endif
    /*
    	TODO:
    	Allow a subset of pages to be passed in
    	(for deferred I/O).  Check pages against
    	pan display settings to see if they
    	should be updated.
    */
    /* For now, just write the full 40KiB on each update */

    /* Set row/column data window */
    st7789vw_set_addr_win(par, 0, 0, WIDTH-1, HEIGHT-1);

    /* Internal RAM write command */
    st7789vw_write_cmd(par, st7789vw_RAMWR);

    /* Blast framebuffer to st7789vw internal display RAM */
#ifdef __LITTLE_ENDIAN
    ret = st7789vw_write_data_buf(par, (u8 *)ssbuf, WIDTH*HEIGHT*BPP/8);
#else
    ret = st7789vw_write_data_buf(par, vmem, WIDTH*HEIGHT*BPP/8);
#endif
    if (ret < 0)
        pr_err("%s: spi_write failed to update display buffer\n",
               par->info->fix.id);
}

static void st7789vwfb_deferred_io(struct fb_info *info,
                                   struct list_head *pagelist)
{
    st7789vwfb_update_display(info->par);
}

static int st7789vwfb_init_display(struct st7789vwfb_par *par)
{
    /* TODO: Need some error checking on gpios */

    /* Request GPIOs and initialize to default values */
    gpio_request_one(par->rst, GPIOF_OUT_INIT_HIGH,
                     "st7789vw Reset Pin");
    gpio_request_one(par->dc, GPIOF_OUT_INIT_LOW,
                     "st7789vw Data/Command Pin");

    st7789vw_reset(par);

    st7789vw_run_cfg_script(par);

    return 0;
}

void st7789vwfb_fillrect(struct fb_info *info, const struct fb_fillrect *rect)
{
    struct st7789vwfb_par *par = info->par;

    sys_fillrect(info, rect);

    st7789vwfb_update_display(par);
}

void st7789vwfb_copyarea(struct fb_info *info, const struct fb_copyarea *area)
{
    struct st7789vwfb_par *par = info->par;

    sys_copyarea(info, area);

    st7789vwfb_update_display(par);
}

void st7789vwfb_imageblit(struct fb_info *info, const struct fb_image *image)
{
    struct st7789vwfb_par *par = info->par;

    sys_imageblit(info, image);

    st7789vwfb_update_display(par);
}

static ssize_t st7789vwfb_write(struct fb_info *info, const char __user *buf,
                                size_t count, loff_t *ppos)
{
    struct st7789vwfb_par *par = info->par;
    unsigned long p = *ppos;
    void *dst;
    int err = 0;
    unsigned long total_size;

    if (info->state != FBINFO_STATE_RUNNING)
        return -EPERM;

    total_size = info->fix.smem_len;

    if (p > total_size)
        return -EFBIG;

    if (count > total_size) {
        err = -EFBIG;
        count = total_size;
    }

    if (count + p > total_size) {
        if (!err)
            err = -ENOSPC;

        count = total_size - p;
    }

    dst = (void __force *) (info->screen_base + p);

    if (copy_from_user(dst, buf, count))
        err = -EFAULT;

    if  (!err)
        *ppos += count;

    st7789vwfb_update_display(par);

    return (err) ? err : count;
}

static struct fb_ops st7789vwfb_ops = {
    .owner		= THIS_MODULE,
    .fb_read	= fb_sys_read,
    .fb_write	= st7789vwfb_write,
    .fb_fillrect	= st7789vwfb_fillrect,
    .fb_copyarea	= st7789vwfb_copyarea,
    .fb_imageblit	= st7789vwfb_imageblit,
};

static struct fb_deferred_io st7789vwfb_defio = {
    .delay		= HZ/30,
    .deferred_io	= st7789vwfb_deferred_io,
};

static int __init st7789vwfb_probe (struct spi_device *spi)
{
    int chip = spi_get_device_id(spi)->driver_data;
    struct st7789vwfb_platform_data *pdata = spi->dev.platform_data;
    int vmem_size = WIDTH*HEIGHT*BPP/8;
    u16 *vmem;
    struct fb_info *info;
    struct st7789vwfb_par *par;
    int retval = -ENOMEM;

    if (chip != st7789vw_DISPLAY_AF_TFT18) {
        pr_err("%s: only the %s device is supported\n", DRVNAME,
               to_spi_driver(spi->dev.driver)->id_table->name);
        return -EINVAL;
    }

    if (!pdata) {
        pr_err("%s: platform data required for rst and dc info\n",
               DRVNAME);
        return -EINVAL;
    }

    vmem = vzalloc(vmem_size);
    if (!vmem)
        return retval;

    info = framebuffer_alloc(sizeof(struct st7789vwfb_par), &spi->dev);
    if (!info)
        goto fballoc_fail;

    info->screen_base = (u8 __force __iomem *)vmem;
    info->fbops = &st7789vwfb_ops;
    info->fix = st7789vwfb_fix;
    info->fix.smem_len = vmem_size;
    info->var = st7789vwfb_var;
    /* Choose any packed pixel format as long as it's RGB565 */
    info->var.red.offset = 11;
    info->var.red.length = 5;
    info->var.green.offset = 5;
    info->var.green.length = 6;
    info->var.blue.offset = 0;
    info->var.blue.length = 5;
    info->var.transp.offset = 0;
    info->var.transp.length = 0;
    info->flags = FBINFO_FLAG_DEFAULT | FBINFO_VIRTFB;
    info->fbdefio = &st7789vwfb_defio;
    fb_deferred_io_init(info);

    par = info->par;
    par->info = info;
    par->spi = spi;
    par->rst = pdata->rst_gpio;
    par->dc = pdata->dc_gpio;

#ifdef __LITTLE_ENDIAN
    /* Allocate swapped shadow buffer */
    vmem = vzalloc(vmem_size);
    if (!vmem)
        return retval;
    par->ssbuf = vmem;
#endif

    retval = register_framebuffer(info);
    if (retval < 0)
        goto fbreg_fail;

    spi_set_drvdata(spi, info);

    retval = st7789vwfb_init_display(par);
    if (retval < 0)
        goto init_fail;

    printk(KERN_INFO
           "fb%d: %s frame buffer device,\n\tusing %d KiB of video memory\n",
           info->node, info->fix.id, vmem_size);

    return 0;


    /* TODO: release gpios on fail */
init_fail:
    spi_set_drvdata(spi, NULL);

fbreg_fail:
    framebuffer_release(info);

fballoc_fail:
    vfree(vmem);

    return retval;
}

static int  st7789vwfb_remove(struct spi_device *spi)
{
    struct fb_info *info = spi_get_drvdata(spi);

    spi_set_drvdata(spi, NULL);

    if (info) {
        unregister_framebuffer(info);
        vfree(info->screen_base);
        framebuffer_release(info);
    }

    /* TODO: release gpios */

    return 0;
}

static const struct spi_device_id st7789vwfb_ids[] = {
    { "adafruit_tft18", st7789vw_DISPLAY_AF_TFT18 },
    { },
};

MODULE_DEVICE_TABLE(spi, st7789vwfb_ids);

static struct spi_driver st7789vwfb_driver = {
    .driver = {
        .name   = "st7789vwfb",
        .owner  = THIS_MODULE,
    },
    .id_table = st7789vwfb_ids,
    .probe  = st7789vwfb_probe,
    .remove = st7789vwfb_remove,
};

static int __init st7789vwfb_init(void)
{
    return spi_register_driver(&st7789vwfb_driver);
}

static void __exit st7789vwfb_exit(void)
{
    spi_unregister_driver(&st7789vwfb_driver);
}

/* ------------------------------------------------------------------------- */

module_init(st7789vwfb_init);
module_exit(st7789vwfb_exit);

MODULE_DESCRIPTION("FB driver for st7789vw display controller");
MODULE_AUTHOR("Matt Porter");
MODULE_LICENSE("GPL");
