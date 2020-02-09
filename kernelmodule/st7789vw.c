// SPDX-License-Identifier: GPL-2.0+
/*
 * DRM driver for Sitronix st7789R panels
 *
 * Based on Code from David Lechner <david@lechnology.com>
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/dma-buf.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/property.h>
#include <linux/spi/spi.h>
#include <video/mipi_display.h>

#include <drm/drm_atomic_helper.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_gem_cma_helper.h>
#include <drm/drm_gem_framebuffer_helper.h>
#include <drm/drm_mipi_dbi.h>

#define st7789R_FRMCTR1		0xb1
#define st7789R_FRMCTR2		0xb2
#define st7789R_FRMCTR3		0xb3
#define st7789R_INVCTR		0xb4
#define st7789R_PWCTR1		0xc0
#define st7789R_PWCTR2		0xc1
#define st7789R_PWCTR3		0xc2
#define st7789R_PWCTR4		0xc3
#define st7789R_PWCTR5		0xc4
#define st7789R_VMCTR1		0xc5
#define st7789R_GAMCTRP1	0xe0
#define st7789R_GAMCTRN1	0xe1

#define st7789R_MY	BIT(7)
#define st7789R_MX	BIT(6)
#define st7789R_MV	BIT(5)

static void jd_t18003_t01_pipe_enable(struct drm_simple_display_pipe *pipe,
				      struct drm_crtc_state *crtc_state,
				      struct drm_plane_state *plane_state)
{
	struct mipi_dbi_dev *dbidev = drm_to_mipi_dbi_dev(pipe->crtc.dev);
	struct mipi_dbi *dbi = &dbidev->dbi;
	int ret, idx;
	u8 addr_mode;

	if (!drm_dev_enter(pipe->crtc.dev, &idx))
		return;

	DRM_DEBUG_KMS("\n");


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

	mipi_dbi_enable_flush(dbidev, crtc_state, plane_state);
out_exit:
	drm_dev_exit(idx);
}

static const struct drm_simple_display_pipe_funcs jd_t18003_t01_pipe_funcs = {
	.enable		= jd_t18003_t01_pipe_enable,
	.disable	= mipi_dbi_pipe_disable,
	.update		= mipi_dbi_pipe_update,
	.prepare_fb	= drm_gem_fb_simple_display_pipe_prepare_fb,
};

static const struct drm_display_mode jd_t18003_t01_mode = {
	DRM_SIMPLE_MODE(240, 240, 20, 20),
};

DEFINE_DRM_GEM_CMA_FOPS(st7789r_fops);

static struct drm_driver st7789r_driver = {
	.driver_features	= DRIVER_GEM | DRIVER_MODESET | DRIVER_ATOMIC,
	.fops			= &st7789r_fops,
	.release		= mipi_dbi_release,
	DRM_GEM_CMA_VMAP_DRIVER_OPS,
	.debugfs_init		= mipi_dbi_debugfs_init,
	.name			= "st7789r",
	.desc			= "Sitronix st7789R",
	.date			= "20171128",
	.major			= 1,
	.minor			= 0,
};

static const struct of_device_id st7789r_of_match[] = {
	{ .compatible = "sitronix,st7789R" },
	{ },
};
MODULE_DEVICE_TABLE(of, st7789r_of_match);

static const struct spi_device_id st7789r_id[] = {
	{ "st7789R", 0 },
	{ },
};
MODULE_DEVICE_TABLE(spi, st7789r_id);

static int st7789r_probe(struct spi_device *spi)
{
	struct device *dev = &spi->dev;
	struct mipi_dbi_dev *dbidev;
	struct drm_device *drm;
	struct mipi_dbi *dbi;
	struct gpio_desc *dc;
	u32 rotation = 0;
	int ret;

	dbidev = kzalloc(sizeof(*dbidev), GFP_KERNEL);
	if (!dbidev)
		return -ENOMEM;

	dbi = &dbidev->dbi;
	drm = &dbidev->drm;
	ret = devm_drm_dev_init(dev, drm, &st7789r_driver);
	if (ret) {
		kfree(dbidev);
		return ret;
	}

	drm_mode_config_init(drm);

	dbi->reset = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(dbi->reset)) {
		DRM_DEV_ERROR(dev, "Failed to get gpio 'reset'\n");
		return PTR_ERR(dbi->reset);
	}

	dc = devm_gpiod_get(dev, "dc", GPIOD_OUT_LOW);
	if (IS_ERR(dc)) {
		DRM_DEV_ERROR(dev, "Failed to get gpio 'dc'\n");
		return PTR_ERR(dc);
	}

	dbidev->backlight = devm_of_find_backlight(dev);
	if (IS_ERR(dbidev->backlight))
		return PTR_ERR(dbidev->backlight);

	device_property_read_u32(dev, "rotation", &rotation);

	ret = mipi_dbi_spi_init(spi, dbi, dc);
	spi->mode = SPI_MODE_3;
	if (ret)
		return ret;

	/* Cannot read from Adafruit 1.8" display via SPI */
	dbi->read_commands = NULL;

	ret = mipi_dbi_dev_init(dbidev, &jd_t18003_t01_pipe_funcs, &jd_t18003_t01_mode, rotation);
	if (ret)
		return ret;

	drm_mode_config_reset(drm);

	ret = drm_dev_register(drm, 0);
	if (ret)
		return ret;

	spi_set_drvdata(spi, drm);

	drm_fbdev_generic_setup(drm, 0);

	return 0;
}

static int st7789r_remove(struct spi_device *spi)
{
	struct drm_device *drm = spi_get_drvdata(spi);

	drm_dev_unplug(drm);
	drm_atomic_helper_shutdown(drm);

	return 0;
}

static void st7789r_shutdown(struct spi_device *spi)
{
	drm_atomic_helper_shutdown(spi_get_drvdata(spi));
}

static struct spi_driver st7789r_spi_driver = {
	.driver = {
		.name = "st7789vw",
		.owner = THIS_MODULE,
		.of_match_table = st7789r_of_match,
	},
	.id_table = st7789r_id,
	.probe = st7789r_probe,
	.remove = st7789r_remove,
	.shutdown = st7789r_shutdown,
};
module_spi_driver(st7789r_spi_driver);

MODULE_DESCRIPTION("Sitronix ST7789VW DRM driver");
MODULE_AUTHOR("Elias Kotlyar <elias.kotlyar@gmail.com");
MODULE_LICENSE("GPL");
