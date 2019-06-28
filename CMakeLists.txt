cmake_minimum_required (VERSION 2.6)
project (sandbox)

find_path(USB_INC NAMES libusb.h PATH_SUFFIXES libusb-1.0)
find_library(USB_LIB usb-1.0)

add_executable(sandbox sandbox.cpp)
target_include_directories(sandbox PRIVATE "${USB_INC}")
target_link_libraries(sandbox PRIVATE "${USB_LIB}")
