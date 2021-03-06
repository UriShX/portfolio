# Portfolio
### Complements my nicer looking portfolio on [my website](https://urishx.com/en/portfolio)
Following are links to projects I've been involved with in the past few years.
Some projects have proper documentation @ [Hackaday.io/urish](https://hackaday.io/urish), some are simply code snippets with either a Fritzing image or a more complete schematic drawn in KiCad.

## WiFi configuration utility web app using Bluetooth LE for ESP32 Written with Nuxt.JS
A recreation of a former app I built to configure WiFi AP credentials for a ESP32 over Bluetooth-LE, with web-bluetooth.
The app was recreated in [Nuxt.JS](https://nuxtjs.org/), a Vue.JS framework, so incorporating its components will be easier, for future projects in Nuxt (or Vue). \
[web app](https://urishx.github.io/Nuxt_esp32_web-ble_wifi_config/) \
[Github repo](https://github.com/UriShX/Nuxt_esp32_web-ble_wifi_config)

## ESP32-Arduino WiFi configuration over Bluetooth-LE sketch
An expansion of [Bernd Giesecke's](https://desire.giesecke.tk/index.php/2018/04/06/esp32-wifi-setup-over-ble/) sketch, which expands usability by adding discovered access point list on demand (via characteristic read), and WiFi connection status via push (characteristic notify). Code checked to confirm it works under Arduino IDE and PlatformIO. \
[My github repo](https://github.com/UriShX/esp32_wifi_ble_advanced).

## WiFi configuration utility web app using Bluetooth LE for ESP32
A single page app written with Knockout & Sammy JS, to configure WiFi credentials over BLE. The app is designed to work with [Bernd Giesecke's](https://desire.giesecke.tk/index.php/2018/04/06/esp32-wifi-setup-over-ble/) Arduino / PlatformIO sketch. ~~I intend to add some features to the app for improving usability and security in the near future (02/2020)~~ A more secure app, written with Nuxt.JS, can be found [here](https://github.com/UriShX/Nuxt_esp32_web-ble_wifi_config) (see description above).
['ESP32 WiFi config' web app](https://urishx.github.io/esp32_web-ble_wifi_config/),
[Github repository](https://github.com/UriShX/esp32_web-ble_wifi_config)

## Web Bluetooth control of servo and LED
A simple web app and a couple of gists, demonstrating a simple way to control ESP32 over BLE, using custom descriptors and a web based controller.
[App](https://github.com/UriShX/ESP32_fader),
['Breathing' LED gist](https://gist.github.com/UriShX/2b1f1c7b461b466a4b4ae336d52653dd),
[Hobby servo gist](https://gist.github.com/UriShX/81266ab108876c4ef4252cc9fd3e1432)

## Arduino MIDI controllers system
I dubbed this ['Arduino Blocks for MIDI Controllers'](https://hackaday.io/project/109296-arduino-blocks-for-midi-controllers), or
'AB4MC'. It was meant to be a system of shields connecting to the Arduino Uno, and allowing for Scratch- style programming. 
Hardware repository is [here](https://github.com/UriShX/AB4MIDICtrlrs), and two gists to display the system capabilities are: [8*8 button matrix](https://gist.github.com/UriShX/ac12b4dfd76a2afa1785bcdb08027061)
 and [4*16 analog input](https://gist.github.com/UriShX/a0cf2a0e9770fb016faa0da292c08822)

## 2 DC motors moving in tandem w/ AP for control and monitoring
ESP32 & Roboclaw control for exhibition in museum [code](/Roboclaw_control_over_ESP32_with_AP_for_control/roboclaw_esp32_w_AP_and_config.ino)

## Raspberry Pi play video when button pressed
Written for a commercial exhibition, video plays when a user inserts a 'coin' to a slot in the enclosure. [code](/Raspberry_Pi_play_video_with_GPIO/rPi_play_video_w_GPIO.py)

## NiTi light
Electrical realization of a table light design by Aya Shani. Code lives [here](/NiTi_light/NiTi_heating_3_wires_with_LED_pot_switch_fan_cycling_button.ino).
To see the design in action click [NiTi light blooming](https://urishx.com/wp-content/uploads/2019/08/NiTi_light_demo.gif)
Better viewed on my _prettier_ [portfolio](https://urishx.com/en/portfolio).
![NiTi light layout](/NiTi_light/NiTi_light_180926a_bb.png)