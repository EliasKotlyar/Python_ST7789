# Copyright (c) 2014 Adafruit Industries
# Author: Tony DiCola
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
import sys

from PIL import Image
import ST7789 as ST7789
from pyA20.spi import spi

from pyA20.gpio import port

image_file = "cat.jpg"

# DC = UEXT3 / PI20
DC  = port.PI12
# RST = UEX4 / PI21 - Gelb
RST = port.PI13
# LED : nothing.
LED = port.PI21

SPI_PORT = 0
SPI_DEVICE = 0
SPI_MODE = 0b11
SPI_SPEED_HZ = 16000000

# Default HZ:
# 1000000
# Maximum HZ:
# 40000000

spi.open("/dev/spidev2.0", mode=3, delay=0, bits_per_word=8, speed=SPI_SPEED_HZ)

# Create ST7789 LCD display class.
disp = ST7789.ST7789(spi=spi,
        rst=RST, dc=DC)

WIDTH = disp.width
HEIGHT = disp.height

# Initialize display.
disp.begin()

# Load an image.
print('Loading image: {}...'.format(image_file))
image = Image.open(image_file)

# Resize the image
image = image.resize((WIDTH, HEIGHT))

# Draw the image on the display hardware.
print('Drawing image')

disp.display(image)
