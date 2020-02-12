/*
 * linux/include/video/st7789vwfb.h -- FB driver for st7789vw LCD controller
 *
 * Copyright (C) 2011, Matt Porter
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#define DRVNAME		"st7789vw_fb"
#define WIDTH		128
#define HEIGHT		160
#define BPP		16

/* Supported display modules */
#define st7789vw_DISPLAY_AF_TFT18		0	/* Adafruit SPI TFT 1.8" */

/* Init script function */
struct st7789vw_function {
	u16 cmd;
	u16 data;
};

/* Init script commands */
enum st7789vw_cmd {
	st7789vw_START,
	st7789vw_END,
	st7789vw_CMD,
	st7789vw_DATA,
	st7789vw_DELAY
};

struct st7789vwfb_par {
	struct spi_device *spi;
	struct fb_info *info;
	u16 *ssbuf;
	int rst;
	int dc;
};

struct st7789vwfb_platform_data {
	int rst_gpio;
	int dc_gpio;
};

/* st7789vw Commands */
#define st7789vw_NOP	0x0
#define st7789vw_SWRESET	0x01
#define st7789vw_RDDID	0x04
#define st7789vw_RDDST	0x09
#define st7789vw_SLPIN	0x10
#define st7789vw_SLPOUT	0x11
#define st7789vw_PTLON	0x12
#define st7789vw_NORON	0x13
#define st7789vw_INVOFF	0x20
#define st7789vw_INVON	0x21
#define st7789vw_DISPOFF	0x28
#define st7789vw_DISPON	0x29
#define st7789vw_CASET	0x2A
#define st7789vw_RASET	0x2B
#define st7789vw_RAMWR	0x2C
#define st7789vw_RAMRD	0x2E
#define st7789vw_COLMOD	0x3A
#define st7789vw_MADCTL	0x36
#define st7789vw_FRMCTR1	0xB1
#define st7789vw_FRMCTR2	0xB2
#define st7789vw_FRMCTR3	0xB3
#define st7789vw_INVCTR	0xB4
#define st7789vw_DISSET5	0xB6
#define st7789vw_PWCTR1	0xC0
#define st7789vw_PWCTR2	0xC1
#define st7789vw_PWCTR3	0xC2
#define st7789vw_PWCTR4	0xC3
#define st7789vw_PWCTR5	0xC4
#define st7789vw_VMCTR1	0xC5
#define st7789vw_RDID1	0xDA
#define st7789vw_RDID2	0xDB
#define st7789vw_RDID3	0xDC
#define st7789vw_RDID4	0xDD
#define st7789vw_GMCTRP1	0xE0
#define st7789vw_GMCTRN1	0xE1
#define st7789vw_PWCTR6	0xFC




