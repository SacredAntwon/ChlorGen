# ChlorGen

Device used to control chlorine generation in a salt water pool. The device will connect to the users WiFi and host an HTML page locally. The user will be able to select the interval between each polarity swap. There will also be an option to select the time frame that the device should generate chlorine for the pool

## Parts List

_Microcontroller:_ ESP32

_Screen:_ SSD1306 OLED 128x64

_Motor Controller:_ L298N Motor Drive Controller

_RGB LED_

_Buzzer_

_Barrel Power Jack_

_Custom Case:_ `ChlorGenCase.stl` and `ChlorGenLid.stl` stored in `STL` folder

## Pin Layout

| Screen Pin | ESP32 Pin |
| :--------: | :-------: |
|    VCC     |    3V3    |
|    GND     |    GND    |
|    SDA     |    21     |
|    SCL     |    22     |

| Motor Controller Pin | ESP32 Pin |
| :------------------: | :-------: |
|         POS          |    3V3    |
|         RPMW         |    26     |
|         LPMW         |    27     |
|         GND          |    GND    |

|  LED Pin   | ESP32 Pin |
| :--------: | :-------: |
|  LED Red   |    32     |
| LED Green  |    33     |
|  LED Blue  |    25     |
| Ground Pin |    GND    |

| Buzzer Pin | ESP32 Pin |
| :--------: | :-------: |
|  Positive  |    12     |
|  Negative  |    GND    |

| Barrel Jack Pin | ESP32 Pin |
| :-------------: | :-------: |
|    Positive     |    VIN    |
|    Negative     |    GND    |
