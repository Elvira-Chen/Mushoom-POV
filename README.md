# Itsumi Mushoom POV

An ESP32 based POV simulated a mushoom with growing up stage in the graduate design of itsumi. Developed with ESP-IDF. 

## Quick Start

To build and flash this project, you need ESP-IDF 4.4.1 installed. You can follow the [documents](https://docs.espressif.com/projects/esp-idf/en/v4.4.1/esp32/get-started/index.html) to prepare the environment. 

Build and flash with ESP-IDF tools. 

```powershell
idf.py build # Build the project
idf.py -p PORT [-b BAUD] flash # Flash the binary
```

## Hardware design

The hardware design could be implement in many partten. You can refer the design of [DevKit-C](https://www.espressif.com.cn/zh-hans/products/devkits/esp32-devkitc) development board. The LED light strip is made up of APA102. The detailed hardware design may be open source later.

## Licenses

The inspiration for this project comes from [@Itsumi](https://github.com/itsumi-yushang), while the development and implementation of software and hardware are completed by [@CharlieJ107](https://github.com/charlieJ107).

This project is open source under (Mozilla Public License, version 2.0)(https://www.mozilla.org/en-US/MPL/2.0/). You can refer to the LICENSE file for more information.