cmake_minimum_required(VERSION 3.22)
include(pico_sdk_import.cmake)

project(PicoEEG C CXX ASM)

add_executable(eegRcvr
        main.c
        dataRecv.c
        dataProc.c
        )

add_library(kiss_fft
    kissfft/kiss_fft.c
    kissfft/kiss_fftr.c)

pico_sdk_init()
# create map/bin/hex file etc. 
pico_add_extra_outputs(eegRcvr)

pico_enable_stdio_usb(eegRcvr 1)
pico_enable_stdio_uart(eegRcvr 1)

# Pull in our pico_stdlib which pulls in commonly used features
target_link_libraries(eegRcvr pico_stdlib)
target_link_libraries(eegRcvr pico_multicore)
target_link_libraries(eegRcvr pico_runtime)
target_link_libraries(eegRcvr hardware_adc)
target_link_libraries(eegRcvr kiss_fft)
