import ST7789
import time

from PIL import Image,ImageDraw,ImageFont








# 240x240 display with hardware SPI:
disp = ST7789.ST7789()

# Initialize library.
disp.Init()

# Clear display.
disp.clear()

# Create blank image for drawing.
image = Image.open('pic.jpg')
disp.ShowImage(image,0,0)
