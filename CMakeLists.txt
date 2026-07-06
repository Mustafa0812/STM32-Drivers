cmake_minimum_required(VERSION 3.22)

# Must be before project()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32_drivers C ASM)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CPU flags for STM32F411 Cortex-M4
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# Compiler flags
set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -O0 -g3 -DSTM32F411xE")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS} \
    -T\"${CMAKE_SOURCE_DIR}/STM32F411RETX_FLASH.ld\" \
    -Wl,--gc-sections \
    -Wl,-Map=output.map \
    --specs=nano.specs \
    --specs=nosys.specs")

# Select which example to build:
#   cmake -B build -DEXAMPLE=led
#   cmake -B build -DEXAMPLE=mpu9250_accel_i2c   (default)
#   cmake -B build -DEXAMPLE=mpu9250_accel_spi
#   cmake -B build -DEXAMPLE=mpu9250_accel_spi_dma
#   cmake -B build -DEXAMPLE=bmp390_temp_i2c
#   cmake -B build -DEXAMPLE=pwm_led
#   cmake -B build -DEXAMPLE=freertos_blink
#   cmake -B build -DEXAMPLE=freertos_sensor_blink
set(EXAMPLE "mpu9250_accel_i2c" CACHE STRING "Example folder under examples/ to build")

# Peripheral bus drivers — always compiled
file(GLOB_RECURSE BUS_SOURCES
    "drivers/gpio/src/*.c"
    "drivers/i2c/src/*.c"
    "drivers/spi/src/*.c"
    "drivers/systick/src/*.c"
    "drivers/uart/src/*.c"
    "drivers/pwm/src/*.c"
    "drivers/dma/*/src/*.c"
    "system/*.c"
    "startup/*.s"
)

# Device driver — matched by exact example name to avoid false pattern matches
if(EXAMPLE STREQUAL "mpu9250_accel_i2c")
    set(DEVICE_SOURCES "drivers/mpu9250/src/mpu9250_i2c.c")
elseif(EXAMPLE STREQUAL "mpu9250_accel_spi")
    set(DEVICE_SOURCES "drivers/mpu9250/src/mpu9250_spi.c")
elseif(EXAMPLE STREQUAL "mpu9250_accel_spi_dma")
    set(DEVICE_SOURCES "drivers/mpu9250/src/mpu9250_spi-dma.c")
elseif(EXAMPLE STREQUAL "bmp390_temp_i2c")
    set(DEVICE_SOURCES "drivers/bmp390/src/bmp390.c")
elseif(EXAMPLE STREQUAL "freertos_sensor_blink")
    set(DEVICE_SOURCES "drivers/mpu9250/src/mpu9250_spi-dma.c")
endif()

# FreeRTOS kernel — only compiled for FreeRTOS-based examples, so bare-metal
# examples never need to provide FreeRTOS-only callbacks like
# vApplicationStackOverflowHook.
if(EXAMPLE STREQUAL "freertos_blink" OR EXAMPLE STREQUAL "freertos_sensor_blink")
    file(GLOB_RECURSE RTOS_SOURCES "third_party/FreeRTOS/*.c")
    set(RTOS_INC_DIRS
        third_party/FreeRTOS/include
        third_party/FreeRTOS/portable/GCC/ARM_CM4F
        config/
    )
endif()

set(SOURCES ${BUS_SOURCES} ${DEVICE_SOURCES} ${RTOS_SOURCES} "examples/${EXAMPLE}/main.c")

# Include paths — CMSIS headers always visible; driver inc/ folders auto-discovered
file(GLOB DRIVER_INC_DIRS "drivers/*/inc")
file(GLOB DMA_INC_DIRS "drivers/dma/*/inc")

include_directories(
    chip_headers/CMSIS/Device/ST/STM32F4xx/Include
    chip_headers/CMSIS/Include
    ${RTOS_INC_DIRS}
    ${DRIVER_INC_DIRS}
    ${DMA_INC_DIRS}
)

add_executable(${PROJECT_NAME}.elf ${SOURCES})

# Generate .hex and .bin after every build
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex
        $<TARGET_FILE:${PROJECT_NAME}.elf>
        ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary
        $<TARGET_FILE:${PROJECT_NAME}.elf>
        ${PROJECT_NAME}.bin
    COMMENT "Generating HEX and BIN files")
