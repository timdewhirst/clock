# LED Clock
## Components

- nodemcu ESP2866
  - [pin out](https://i0.wp.com/randomnerdtutorials.com/wp-content/uploads/2019/05/ESP8266-NodeMCU-kit-12-E-pinout-gpio-pin.png)
- 4-gang LED 8x8 matrix (SPI, MAX7219 based?)
  - 8x8 suggests 8 bytes per section
  - [info](https://forum.arduino.cc/t/what-is-clk-din-cs-pin-on-led-dot-matrix-like-max7219-8x32-and-how-the-pins-work/936211/9)
- GPS if NTP doesn't work
- temperature sensor
- brightness potentiometer
- mode switch

## Arduino

- install arduino `sudo apt install arduino`
- add board package for ESP8266:
  - [adding additional board packages](https://support.arduino.cc/hc/en-us/articles/360016466340-Add-or-remove-third-party-boards-in-Boards-Manager?queryID=b902050fe0078a12ac11d8aa24e52957)
  - [ESP8266 how-to](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout/using-arduino-ide)
  - [board package](http://arduino.esp8266.com/stable/package_esp8266com_index.json)
  - select `NodeMCU (1.0)`
- connection:
  - appears on `/dev/ttyUSB1`
  - connection speed: 115200
