# NFC Reader for Home Assistant with M5Stack ATOM Echo and LEGO

## Demo  
https://youtu.be/kcqGKI0bAL4  

## Components  
- [M5Stack ATOM Echo](https://shop.m5stack.com/collections/atom-series/products/atom-echo-smart-speaker-dev-kit)  
- [M5Stack Mini RFID Reader/Writer Unit (MFRC522)](https://shop.m5stack.com/products/rfid-sensor-unit)  
- LEGO Question Mark Block: search for "MOC 8887" or "MOC 93671" on Aliexpress.  
  
M5Stack components are also available on Aliexpress & Banggood.  

## Arduino
- Board: M5Stack-ATOM ([Board URL](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/package_m5stack_index.json))
- Libraries:
  - [256dpi/arduino-mqtt](https://github.com/256dpi/arduino-mqtt)
  - [m5stack/M5Atom](https://github.com/m5stack/M5Atom) (Tested with version 0.0.8)
  - [MFRC522_I2C](https://github.com/m5stack/M5Atom/tree/master/examples/Unit/RFID_RC522)

## How it works
1. Read NFC card.  
2. Send card ID to local MQTT server, then sleep for 5 seconds (to prevent sending same card multiple times).  
3. Play sound on Atom Echo.  
4. Receive card ID in Node RED.  
5. Run automation depends on card ID.

## How to generate custom sound  
1. Prepare a 44100 Hz sample rate, Signed 16-bit PCM WAV file. I used Audacity.  
2. Run `xxd -i sound.wav sound.c` in terminal to generate .C file. I use MacOS so I'm not sure about Windows or Linux. Sorry.
