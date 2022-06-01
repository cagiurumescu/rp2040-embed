## Getting started

Setup the pico-sdk as described [here](https://github.com/raspberrypi/pico-sdk). Make sure to install TinyUSB:

> `git submodule update --init`

## The board
<img src="docs/pinout.jpg" alt="RP2040" width="600"/>

Connect some UART dongle to UART0. Also connect a USB-C cable to the board. Hold BOOT while pressing RESET to enable the USB filesystem. Copy either hello_serial.uf2 (to see output on UART0 dongle) or hello_usb.uf2 (to see output on UART over USB-C).
