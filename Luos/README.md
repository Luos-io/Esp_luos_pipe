# ESP_Luos_pipe
This is an arduino code used to create a pipe between a [Luos Robotics](https://www.luos-robotics.com) Gate module and a wifi and Bluetooth connection.
This code manage :
 - Serial debug messages from ESP
 - Serial interface to a [Luos Robotics](https://www.luos-robotics.com) Gate module
 - SSID, PASS, and hostname save on EEPROM
 - Access Point creation (using hostname) if no known wifi found
 - Alias MDNS using Hostname (working with Bonjour or Avahi)
 - MDNS services to be easily detected in the wifi network
 - Captive DNS server in Access point mode
 - Websocket used to stream [Luos Robotics](https://www.luos-robotics.com) network datas (PORT 9342)
 - Serial (SPP) Bluetooth mode used to stream [Luos Robotics](https://www.luos-robotics.com) network datas
 - OTA server allowing update of this firmware trough Wifi
 - Webserver allowing to :
   - Reset default parameters
   - Setup hostname
   - Select SSID
   - Setup Wifi password

You are free to use and modify this code.

## How to use and setup your ESP

The first boot your ESP module will use default settings :
 - *hostname* : "LuosNetwork"
 - *SSID* : Void
 - *PASS* : Void

### Connecting using Bluetooth

To connect your device to the ESP Bluetooth server just start your module and check your Bluetooth devices list. You should find a device named as the hostname of your module.
There is no password so you can simply connect to it. When your connection is established your device should create a Serial port available on your Serial port list (depend on your system).
You can simply use this serial port to stream [Luos Robotics](https://www.luos-robotics.com) Json data using [Pyluos](https://github.com/Luos-Robotics/pyluos) for example.

### Connecting using Wifi

To connect your device to the ESP Wifi just start your module and check your Wifi list. You should find a Wifi named as the hostname of your module.
There is no password so you can simply connect to it. When your connection is established you can use WebSocket (hostname.local or 192.168.1.1, port : 9342) to stream [Luos Robotics](https://www.luos-robotics.com) Json data using [Pyluos](https://github.com/Luos-Robotics/pyluos) for example.
When your connection is established your device should open a webpage allowing you to configure your device. If this website doesn't aprear you can access it using your web-browser at http://hostname.local or http://192.168.1.1 or any http address.
When you finished to setup your wifi following instruction on the web interface, you can use WebSocket (hostname.local or 192.168.1.1, port : 9342) to stream [Luos Robotics](https://www.luos-robotics.com) Json data using [Pyluos](https://github.com/Luos-Robotics/pyluos) for example.

## How to compile and update my ESP32

1) First you have to clone this repository and have Arduino or Plateformio installed
2) Install the ESP32 board management into Arduino or Plateformio following this tutorial : https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
3) Install the WebSockets library by Markus Sattler using Sketch/include a library/manage library
4) In Arduino Board selection menu, select "ESP32 DEV Module"
5) In Arduino Tools/Partition Scheme select "Minimal SPIFFS"
6) Start your ESP32 module and connect your computer to the same wifi network (This firmware need to be written at least one time using serial before this)
7) In Arduino Tools/Ports select your ESP module in the "Network Port section"

You are now ready to build and update your ESP
