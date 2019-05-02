# OLEDPong

A game of 'Pong' on the 128x64 pixel OLED display.

## The Display

The 128x64 pixel OLED display uses an SSD1306 driver chip.
This is a commonly-available module, but it uses a 3.3V interface
voltage (the standard Arduino is 5V).
I use a level-shifter based on either the CD4050 or the 74HC4050
to convert the SPI pins on the Arduino to 3.3V.
