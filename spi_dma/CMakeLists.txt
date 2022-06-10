cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(spi_dma)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(spi_dma
   spi_dma.c
)

target_link_libraries(spi_dma pico_stdlib hardware_spi hardware_dma)

# disable uart over usb output, enable uart0 output
pico_enable_stdio_usb(spi_dma 0)
pico_enable_stdio_uart(spi_dma 1)

# create map/bin/hex file etc.
pico_add_extra_outputs(spi_dma)
