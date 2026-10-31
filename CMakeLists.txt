cmake_minimum_required(VERSION 3.13)

# Board: mac dinh pico2 (co the ghi de bang -DPICO_BOARD=...)
set(PICO_BOARD pico2 CACHE STRING "Board type")

# Nhan PICO_SDK_PATH tu -D... HOAC tu bien moi truong
if (NOT PICO_SDK_PATH AND DEFINED ENV{PICO_SDK_PATH})
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
endif()
if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "Chua dat PICO_SDK_PATH. Dung: cmake -DPICO_SDK_PATH=/duong/dan/pico-sdk ...")
endif()
include(${PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(usb_pcm54 C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_executable(usb_pcm54 main.c usb_descriptors.c usb_audio.c)
pico_generate_pio_header(usb_pcm54 ${CMAKE_CURRENT_LIST_DIR}/pcm54_par.pio)
target_include_directories(usb_pcm54 PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(usb_pcm54 pico_stdlib hardware_pio hardware_dma pico_unique_id
                      tinyusb_device tinyusb_board)
pico_add_extra_outputs(usb_pcm54)
