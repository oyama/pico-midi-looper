cmake_minimum_required(VERSION 3.13...3.27)

include(pico_sdk_import.cmake)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
project(pico-midi-looper C CXX ASM)
pico_sdk_init()

add_executable(${CMAKE_PROJECT_NAME}
  src/main.c
  src/button.c
  src/tap_tempo.c
  src/display.c
  src/ble_midi.c
  src/usb_midi.c
)
pico_btstack_make_gatt_header(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/midi_service.gatt)
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(${CMAKE_PROJECT_NAME}
  pico_stdlib
  ble_midi
  usb_midi
)

add_library(ble_midi INTERFACE)
target_link_libraries(ble_midi INTERFACE
  pico_btstack_ble
  pico_btstack_cyw43
  pico_cyw43_arch_none
)

add_library(usb_midi INTERFACE)
target_link_libraries(ble_midi INTERFACE
  tinyusb_device
  tinyusb_board
)

pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
