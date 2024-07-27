add_executable(gpico
    img/rpi1.cpp
    img/rpi2.cpp
    img/rpi4.cpp
    img/rpi8.cpp
    img/peter1.cpp
    img/peter2.cpp
    img/peter4.cpp
    img/peter8.cpp

    src/main.cpp
    src/ssd1306_i2c.c
    src/buttons.c
    src/dsp.c
    src/ui.c
    src/pitch.c
    src/tinycl.cpp
    src/usbmain.c
    src/usb_descriptors.c
    src/waves.c
)

add_picovga(gpico)

# for vga_config.h include
target_include_directories(gpico PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
)

# create map/bin/hex file etc.
pico_add_extra_outputs(gpico)

# Allow stdio to USB
pico_enable_stdio_usb(gpico 0)
