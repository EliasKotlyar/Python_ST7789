# Python_ST7789 on Olinuxino A20 Micro
Python library for using ST7789-based IPS LCD
(240x240 pixels, SPI interface, 7 pins without CS pin)


1. Install latest Armbian on your on your System
2. Install Kernel Patch which has been patched for SPI-Mode3:
spidev.patch
(There is a precompiled version in this repository)
3. Install newer DTB(sun7i-a20-olinuxino-micro-emmc.dtb) from DTB-Folder, which allows usage of SPI2 (UEXT1 Header)
4. Add following into your armbian-env:
```bash
overlays=spi-spidev spi2
param_spidev_spi_bus=2
param_spidev_max_freq=40000000
```

5. Wire the Display to UEXT1: https://www.olimex.com/wiki/images/a/a9/Example.jpg
5. Install following Software:
```angular2
apt-get install python python-numpy python-pip python-setuptools python-dev python-pil
pip install pyA20
```

6. Run main.py

