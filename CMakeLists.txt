cmake_minimum_required(VERSION 3.22)

# Must be before project()
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(stm32_drivers C ASM)

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

# Source files
file(GLOB_RECURSE SOURCES
    "Drivers/gpio/src/*.c"
    "Drivers/spi/src/*.c"
    "Drivers/uart/src/*.c"
    "Drivers/systick/src/*.c"
    "Drivers/mpu9250/src/*.c"
    "system/*.c"
    "example/src/*.c"
    "Startup/*.s"
)

# Include paths — CMSIS headers + per-driver inc folders
include_directories(
    chip_headers/CMSIS/Device/ST/STM32F4xx/Include
    chip_headers/CMSIS/Include
    Drivers/gpio/inc
    Drivers/spi/inc
    Drivers/uart/inc
    Drivers/systick/inc
    Drivers/mpu9250/inc
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
    COMMENT "Generating HEX and BIN files"
)