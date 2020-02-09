import time
import numpy as np
from pyA20.spi import spi
from pyA20.gpio import port
from pyA20.gpio import gpio


class ST7789(object):
    """class for ST7789  240*240 1.3inch OLED displays."""

    def __init__(self):

        self.width = 240
        self.height = 240
        # Initialize DC RST pin
        self._dc = port.PI12
        self._rst = port.PI13
        self._gpio = gpio

        self._gpio.init()
        self._gpio.setcfg(self._dc, gpio.OUTPUT)
        self._gpio.setcfg(self._rst, gpio.OUTPUT)
        self._gpio.output(self._rst, 1)
        self._gpio.output(self._dc, 1)

        speed = 100000  # 0.1 mhz
        speed = 4000000  # 4mhz

        spi.open("/dev/spidev2.0", mode=3, delay=0, bits_per_word=8, speed=speed)
        # Initialize SPI
        self._spi = spi

    """    Write register address and data     """

    def command(self, cmd):
        self._gpio.output(self._dc, 0)
        self.writeSpi([cmd])

    def data(self, val):
        self._gpio.output(self._dc, 1)
        self.writeSpi([val])

    def SPI_TRANSFER(self, arg1, *argv):
        self.command(arg1)  # Column addr set
        for arg in argv:
            self.data(arg)

    def Init(self):
        """Initialize dispaly"""
        self.reset()

        self.SPI_TRANSFER(0x36, 0x70)  # self.data(0x00)

        self.SPI_TRANSFER(0x3A,
                          0x05)

        self.SPI_TRANSFER(0xB2,
                          0x0C,
                          0x0C,
                          0x00,
                          0x33,
                          0x33)

        self.SPI_TRANSFER(0xB7,
                          0x35)

        self.SPI_TRANSFER(0xBB,
                          0x19)

        self.SPI_TRANSFER(0xC0,
                          0x2C)

        self.SPI_TRANSFER(0xC2,
                          0x01)

        self.SPI_TRANSFER(0xC3,
                          0x12)

        self.SPI_TRANSFER(0xC4,
                          0x20)

        self.SPI_TRANSFER(0xC6,
                          0x0F)

        self.SPI_TRANSFER(0xD0,
                          0xA4,
                          0xA1)

        self.SPI_TRANSFER(0xE0,
                          0xD0,
                          0x04,
                          0x0D,
                          0x11,
                          0x13,
                          0x2B,
                          0x3F,
                          0x54,
                          0x4C,
                          0x18,
                          0x0D,
                          0x0B,
                          0x1F,
                          0x23)

        self.SPI_TRANSFER(0xE1,
                          0xD0,
                          0x04,
                          0x0C,
                          0x11,
                          0x13,
                          0x2C,
                          0x3F,
                          0x44,
                          0x51,
                          0x2F,
                          0x1F,
                          0x1F,
                          0x20,
                          0x23)

        self.SPI_TRANSFER(0x21)

        self.SPI_TRANSFER(0x11)

        self.SPI_TRANSFER(0x29)

    def reset(self):
        """Reset the display"""
        self._gpio.output(self._rst, 1)
        time.sleep(0.01)
        self._gpio.output(self._rst, 0)
        time.sleep(0.01)
        self._gpio.output(self._rst, 1)
        time.sleep(0.01)

    def SetWindows(self, Xstart, Ystart, Xend, Yend):
        # set the X coordinates
        self.command(0x2A)
        self.data(0x00)  # Set the horizontal starting point to the high octet
        self.data(Xstart & 0xff)  # Set the horizontal starting point to the low octet
        self.data(0x00)  # Set the horizontal end to the high octet
        self.data((Xend - 1) & 0xff)  # Set the horizontal end to the low octet

        # set the Y coordinates
        self.command(0x2B)
        self.data(0x00)
        self.data((Ystart & 0xff))
        self.data(0x00)
        self.data((Yend - 1) & 0xff)

        self.command(0x2C)

    def writeSpi(self, data):
        chunk_size = 60
        for start in range(0, len(data), chunk_size):
            end = min(start + chunk_size, len(data))
            self._spi.write(data[start:end])

    def ShowImage(self, Image, Xstart, Ystart):
        """Set buffer to value of Python Imaging Library image."""
        """Write display buffer to physical display"""
        imwidth, imheight = Image.size
        if imwidth != self.width or imheight != self.height:
            raise ValueError('Image must be same dimensions as display \
                ({0}x{1}).'.format(self.width, self.height))
        img = np.asarray(Image)
        pix = np.zeros((self.width, self.height, 2), dtype=np.uint8)
        pix[..., [0]] = np.add(np.bitwise_and(img[..., [0]], 0xF8), np.right_shift(img[..., [1]], 5))
        pix[..., [1]] = np.add(np.bitwise_and(np.left_shift(img[..., [1]], 3), 0xE0), np.right_shift(img[..., [2]], 3))
        pix = pix.flatten().tolist()
        self.SetWindows(0, 0, self.width, self.height)
        self._gpio.output(self._dc, 1)
        for i in range(0, len(pix), 4096):
            self.writeSpi(pix[i:i + 4096])

    def clear(self):
        """Clear contents of image buffer"""
        _buffer = [0xff] * (self.width * self.height * 2)
        self.SetWindows(0, 0, self.width, self.height)
        self._gpio.output(self._dc, 1)
        for i in range(0, len(_buffer), 4096):
            self.writeSpi(_buffer[i:i + 4096])
