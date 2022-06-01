cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(hello_world)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(hello_serial
   hello_serial.c
)

if (TARGET tinyusb_device)
    add_executable(hello_usb
            hello_usb.c
            )

    # pull in common dependencies
    target_link_libraries(hello_usb pico_stdlib)
    target_link_libraries(hello_serial pico_stdlib)

    # enable uart over usb output, disable uart0 output
    pico_enable_stdio_usb(hello_usb 1)
    pico_enable_stdio_uart(hello_usb 0)

    # disable uart over usb output, enable uart0 output
    pico_enable_stdio_usb(hello_serial 0)
    pico_enable_stdio_uart(hello_serial 1)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(hello_usb)
    pico_add_extra_outputs(hello_serial)
elseif(PICO_ON_DEVICE)
    message(WARNING "not building hello_usb because TinyUSB submodule is not initialized in the SDK")
    # pull in common dependencies
    target_link_libraries(hello_serial pico_stdlib)

    # disable uart over usb output, enable uart0 output
    pico_enable_stdio_usb(hello_serial 0)
    pico_enable_stdio_uart(hello_serial 1)

    # create map/bin/hex/uf2 file etc.
    pico_add_extra_outputs(hello_serial)
endif()
