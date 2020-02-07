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
import numbers
import time
import numpy as np

import struct

from pyA20.gpio import gpio


__version__ = '0.0.2'

BG_SPI_CS_BACK = 0
BG_SPI_CS_FRONT = 1

SPI_CLOCK_HZ = 16000000

ST7789_NOP = 0x00
ST7789_SWRESET = 0x01
ST7789_RDDID = 0x04
ST7789_RDDST = 0x09

ST7789_SLPIN = 0x10
ST7789_SLPOUT = 0x11
ST7789_PTLON = 0x12
ST7789_NORON = 0x13

# ILI9341_RDMODE = 0x0A
# ILI9341_RDMADCTL = 0x0B
# ILI9341_RDPIXFMT = 0x0C
# ILI9341_RDIMGFMT = 0x0A
# ILI9341_RDSELFDIAG = 0x0F

ST7789_INVOFF = 0x20
ST7789_INVON = 0x21
# ILI9341_GAMMASET = 0x26
ST7789_DISPOFF = 0x28
ST7789_DISPON = 0x29

ST7789_CASET = 0x2A
ST7789_RASET = 0x2B
ST7789_RAMWR = 0x2C
ST7789_RAMRD = 0x2E

ST7789_PTLAR = 0x30
ST7789_MADCTL = 0x36
ST7789_COLMOD = 0x3A

ST7789_FRMCTR1 = 0xB1
ST7789_FRMCTR2 = 0xB2
ST7789_FRMCTR3 = 0xB3
ST7789_INVCTR = 0xB4
# ILI9341_DFUNCTR = 0xB6
ST7789_DISSET5 = 0xB6

ST7789_GCTRL = 0xB7
ST7789_GTADJ = 0xB8
ST7789_VCOMS = 0xBB

ST7789_LCMCTRL = 0xC0
ST7789_IDSET = 0xC1
ST7789_VDVVRHEN = 0xC2
ST7789_VRHS = 0xC3
ST7789_VDVS = 0xC4
ST7789_VMCTR1 = 0xC5
ST7789_FRCTRL2 = 0xC6
ST7789_CABCCTRL = 0xC7

ST7789_RDID1 = 0xDA
ST7789_RDID2 = 0xDB
ST7789_RDID3 = 0xDC
ST7789_RDID4 = 0xDD

ST7789_GMCTRP1 = 0xE0
ST7789_GMCTRN1 = 0xE1

ST7789_PWCTR6 = 0xFC


class ST7789(object):
    """Representation of an ST7789 TFT LCD."""

    def __init__(self, spi, dc, backlight=None, rst=None, width=240,
                 height=240, rotation=90, invert=True, spi_speed_hz=4000000):
        """Create an instance of the display using SPI communication.

        Must provide the GPIO pin number for the D/C pin and the SPI driver.

        Can optionally provide the GPIO pin number for the reset pin as the rst parameter.

        :param port: SPI port number
        :param cs: SPI chip-select number (0 or 1 for BCM
        :param backlight: Pin for controlling backlight
        :param rst: Reset pin for ST7789
        :param width: Width of display connected to ST7789
        :param height: Height of display connected to ST7789
        :param rotation: Rotation of display connected to ST7789
        :param invert: Invert display
        :param spi_speed_hz: SPI speed (in Hz)

        """

        self._spi = spi

        self._dc = dc
        self._rst = rst
        self._width = width
        self._height = height
        self._rotation = rotation
        self._invert = invert

        self._offset_left = 0
        self._offset_top = 0


        self._gpio = gpio

        self._gpio.init()
        # Set DC as output.
        self._gpio.setcfg(dc, gpio.OUTPUT)

        # Setup reset as output (if provided).
        if rst is not None:
            self._gpio.setcfg(rst, gpio.OUTPUT)

        self._gpio.output(self._rst, 1)
        self._gpio.output(self._dc, 1)

        self.reset()
        self._init()

    def send(self, data, is_data=True, chunk_size=60):
        """Write a byte or array of bytes to the display. Is_data parameter
        controls if byte should be interpreted as display data (True) or command
        data (False).  Chunk_size is an optional size of bytes to write in a
        single SPI transaction, with a default of 4096.
        """
        # Set DC low for command, high for data.
        if(is_data==True):
            self._gpio.output(self._dc, 1)
        else:
            self._gpio.output(self._dc, 0)
        # time.sleep(0.01)

        # Convert scalar argument to list so either can be passed as parameter.
        if isinstance(data, numbers.Number):
            data = [data & 0xFF]
        # Write data a chunk at a time.
        for start in range(0, len(data), chunk_size):
            end = min(start + chunk_size, len(data))
            self._spi.write(data[start:end])

    def set_backlight(self, value):
        """Set the backlight on/off."""
        if self._backlight is not None:
            GPIO.output(self._backlight, value)

    @property
    def width(self):
        return self._width if self._rotation == 0 or self._rotation == 180 else self._height

    @property
    def height(self):
        return self._height if self._rotation == 0 or self._rotation == 180 else self._width

    def command(self, data):
        """Write a byte or array of bytes to the display as command data."""
        self.send(data, False)

    def data(self, data):
        """Write a byte or array of bytes to the display as display data."""
        self.send(data, True)

    def reset(self):
        """Reset the display, if reset pin is connected."""
        if self._rst is not None:
            self._gpio.output(self._rst, 1)
            time.sleep(0.100)
            self._gpio.output(self._rst, 0)
            time.sleep(0.100)
            self._gpio.output(self._rst, 1)
            time.sleep(0.100)

    def _init(self):
        self.command(0x36)
        self.data([0x70])

        self.command(0x3A)
        self.data([0x05])

        self.command(0xB2)
        self.data([0x0C, 0x0C, 0x00, 0x33, 0x33])

        self.command(0xB7)
        self.data([0x35])

        self.command(0xBB)
        self.data([0x19])

        self.command(0xC0)
        self.data([0x2C])

        self.command(0xC2)
        self.data([0x01])

        self.command(0xC3)
        self.data([0x12])

        self.command(0xC4)
        self.data([0x20])

        self.command(0xC6)
        self.data([0x0F])

        self.command(0xD0)
        self.data([0xA4, 0xA1])

        self.command(0xE0)
        self.data([0xD0])
        self.data([0x04])
        self.data([0x0D])
        self.data([0x11])
        self.data([0x13])
        self.data([0x2B])
        self.data([0x3F])
        self.data([0x54])
        self.data([0x4C])
        self.data([0x18])
        self.data([0x0D])
        self.data([0x0B])
        self.data([0x1F])
        self.data([0x23])

        self.command(0xE1)
        self.data([0xD0])
        self.data([0x04])
        self.data([0x0C])
        self.data([0x11])
        self.data([0x13])
        self.data([0x2C])
        self.data([0x3F])
        self.data([0x44])
        self.data([0x51])
        self.data([0x2F])
        self.data([0x1F])
        self.data([0x1F])
        self.data([0x20])
        self.data([0x23])

        self.command(0x21)

        self.command(0x11)

        self.command(0x29)


    def begin(self):
        """Set up the display

        Deprecated. Included in __init__.

        """
        pass


    def set_window(self, x0=0, y0=0, x1=None, y1=None):
        """Set the pixel address window for proceeding drawing commands. x0 and
        x1 should define the minimum and maximum x pixel bounds.  y0 and y1
        should define the minimum and maximum y pixel bound.  If no parameters
        are specified the default will be to update the entire display from 0,0
        to width-1,height-1.
        """
        if x1 is None:
            x1 = self._width - 1

        if y1 is None:
            y1 = self._height - 1

        y0 += self._offset_top
        y1 += self._offset_top

        x0 += self._offset_left
        x1 += self._offset_left

        self.command(ST7789_CASET)       # Column addr set
        self.data(x0 >> 8)
        self.data(x0 & 0xFF)             # XSTART
        self.data(x1 >> 8)
        self.data(x1 & 0xFF)             # XEND
        self.command(ST7789_RASET)       # Row addr set
        self.data(y0 >> 8)
        self.data(y0 & 0xFF)             # YSTART
        self.data(y1 >> 8)
        self.data(y1 & 0xFF)             # YEND
        self.command(ST7789_RAMWR)       # write to RAM

    def display(self, image):
      w, h = 240, 240
      self.set_window(0, 0, w, h)

      #packed_image = BitArray().join(BitArray(uint=x & 0x00111111, length=6) for x in image.tobytes()).tobytes()

      #self.data(image.tobytes())

      img = np.asarray(image.convert('RGB'))
      pix = np.zeros((w, h, 2), dtype=np.uint8)
      pix[...,[0]] = np.add(np.bitwise_and(img[...,[0]], 0xF8), np.right_shift(img[...,[1]], 5))
      pix[...,[1]] = np.add(np.bitwise_and(np.left_shift(img[...,[1]], 3), 0xE0), np.right_shift(img[...,[2]], 3))
      pix = pix.flatten().tolist()
      print(pix)

      self.data(pix)

    def image_to_data(self, image, rotation=0):
        """Generator function to convert a PIL image to 16-bit 565 RGB bytes."""
        # NumPy is much faster at doing this. NumPy code provided by:
        # Keith (https://www.blogger.com/profile/02555547344016007163)
        pb = np.rot90(np.array(image.convert('RGB')), rotation // 90).astype('uint8')

        result = np.zeros((self._width, self._height, 2), dtype=np.uint8)
        result[..., [0]] = np.add(np.bitwise_and(pb[..., [0]], 0xF8), np.right_shift(pb[..., [1]], 5))
        result[..., [1]] = np.add(np.bitwise_and(np.left_shift(pb[..., [1]], 3), 0xE0), np.right_shift(pb[..., [2]], 3))
        return result.flatten().tolist()
