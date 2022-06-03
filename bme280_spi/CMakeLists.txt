cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(bme280_spi)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(bme280_spi
        bme280_spi.c
        )

# pull in common dependencies and additional spi hardware support
target_link_libraries(bme280_spi pico_stdlib hardware_spi)

# disable uart over usb output, enable uart0 output
pico_enable_stdio_usb(bme280_spi 0)
pico_enable_stdio_uart(bme280_spi 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(bme280_spi)
