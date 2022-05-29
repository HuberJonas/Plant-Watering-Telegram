# Plant Watering Telegram

Ever wanted to water your plant automatically? Then this is the project for you, as it provides watering functionality at a low cost whilst allowing you to control it with your phone.

## Components

This project uses a Wemos D1 mini hooked up to a watering pump via a mosfet and a capacitive moisture sensor. Connect everything, put it in a box, follow the installation and you are ready to go.


## Installation

Clone the repository, rename `settings.h.example` to `settings.h` and adjust your settings. Upload it with the help of PlatformIO. For the Telegram connection you need a Telegram Bot which can be created with the help of BotFather. The available commands are listed below. When starting the conversation with the bot you will have to enter the secret and send `/reset` to clear the EEPROM.

```
status - aktueller Status
secret - Secret anzeigen
deleteme - Lösche Dich
water - Gieße die Pflanze
setthreshold - Schwellenwert setzen
reset - Einstellungen zurücksetzen
refill - Tank auffüllen
calibrate - Sensor kalibrieren
```


## Contributing

Feel free to contribute. Maybe you'd like to move to commands to a class to improve readability :)
