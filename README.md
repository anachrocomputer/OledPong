![Static Badge](https://img.shields.io/badge/MCU-ATmega328-green "MCU:ATmega328")
![Static Badge](https://img.shields.io/badge/BOARD-Arduino-green "BOARD:Arduino")
![Static Badge](https://img.shields.io/badge/DISPLAY-SSD1306-green "DISPLAY:SSD1306")
![Static Badge](https://img.shields.io/badge/MiniLD-58-green "MiniLD:58")

# OLEDPong #

A game of 'Pong' on the 128x64 pixel OLED display.

## Ludum Dare ##

I wrote this game for Ludum Dare's MiniLD \#58 PONG:

https://web.archive.org/web/20150328062339/http:/ludumdare.com:80/compo/2015/03/13/mini-ld-58-march-20-29/

Many implementations of PONG were sent to the MiniLD contest, and mine is here:

https://web.archive.org/web/20150404142251/http:/ludumdare.com/compo/minild-58/?action=preview&uid=13730

## The Display ##

The 128x64 pixel OLED display uses an SSD1306 driver chip.
This is a commonly-available module,
but it uses a 3.3V interface voltage (the standard Arduino is 5V).
I use a level-shifter based on either the CD4050 or the 74HC4050
to convert the SPI pins on the Arduino to 3.3V.

## Player Control ##

The player starts the game via a button on the Arduino digital pin 2.
Control of the bat is by an analog paddle or joystick connected to
the Arduino analog pin A0.

