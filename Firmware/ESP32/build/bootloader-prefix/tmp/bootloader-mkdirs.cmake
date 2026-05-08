# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/eric/.espressif/v6.0.1/esp-idf/components/bootloader/subproject"
  "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader"
  "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix"
  "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix/tmp"
  "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix/src/bootloader-stamp"
  "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix/src"
  "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/eric/Documents/GitHub/FreeEEG/Firmware/ESP32/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
