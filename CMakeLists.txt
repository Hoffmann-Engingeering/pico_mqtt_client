# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico2_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(pico_client C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(pico_client 
        src/mqtt_client.c
        src/wifi.c
        src/main.c )

pico_set_program_name(pico_client "pico_client")
pico_set_program_version(pico_client "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(pico_client 0)
pico_enable_stdio_usb(pico_client 1)

# Add the standard library to the build
target_link_libraries(pico_client
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(pico_client PRIVATE
        inc
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(pico_client 
        pico_cyw43_arch_lwip_poll
        pico_lwip_mqtt
        hardware_adc
        )

pico_add_extra_outputs(pico_client)

# Add WIFI credentials as compile definitions
add_compile_definitions(
        SSID="your ssid here"
        PASSWORD="your password here"
        SERVER_IP="mqtt server ip here"
        CLIENT_ID="pico_client"
        MQTT_PORT=1883 #1883 for unsecure mqtt, 8883 for secure mqtt
)

