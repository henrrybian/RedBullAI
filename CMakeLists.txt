# 阶段 2 顶层构建配置（AI 生成）
# 来源：工作流阶段 2 提示词第 4 项；工具绝对路径取自 @docs/env_check.md 实测输出。
# 验收命令：
#   cmake -S . -B build/Debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
#   cmake --build build/Debug
#   arm-none-eabi-size build/Debug/*.elf
cmake_minimum_required(VERSION 3.22)

# CMake 4.5.0-rc3 对最低版本策略较严（@docs/env_check.md 292 行）：
# 使用 >=3.5 的声明，并在 project() 之前设置交叉工具链文件。
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake"
      CACHE FILEPATH "ARM cross toolchain file")
endif()

project(RedBull_Buzzer_PWM VERSION 0.1.0 LANGUAGES C ASM)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_C_FLAGS_DEBUG   "-O0 -g3"      CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS_DEBUG "-O0 -g3"      CACHE STRING "" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS_RELEASE "-Os"        CACHE STRING "" FORCE)

set(TARGET_NAME RedBull_Buzzer_PWM)
set(TARGET_ELF  ${TARGET_NAME}.elf)

# 阶段 3：编译期日志等级（0=OFF 1=ERROR 2=WARN 3=INFO 4=DEBUG）
# Debug 默认 4（INFO/DEBUG 开启）；Release 默认 2（INFO/DEBUG 关闭）。
# 可用 cmake -DDEBUG_LOG_LEVEL=N 覆盖，实现「Release 可关闭 / 全关」。
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(DEBUG_LOG_LEVEL_DEFAULT 4)
else()
  set(DEBUG_LOG_LEVEL_DEFAULT 2)
endif()
set(DEBUG_LOG_LEVEL ${DEBUG_LOG_LEVEL_DEFAULT} CACHE STRING "编译期日志等级：0=OFF 1=ERROR 2=WARN 3=INFO 4=DEBUG")
set_property(CACHE DEBUG_LOG_LEVEL PROPERTY STRINGS 0 1 2 3 4)

set(APP_SOURCES
    Core/Src/main.c
    Core/Src/debug_uart.c
    Core/Src/buzzer.c
    Core/Src/stm32f1xx_it.c
    Core/Src/system_stm32f1xx.c
    startup_stm32f103xe.s
)

# 最小必需官方 Cube HAL 源集合（@docs/stage2_hal_source.md 第 4 节）
set(HAL_SOURCES
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_exti.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_flash_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_pwr.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c
    Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c
)

set_source_files_properties(startup_stm32f103xe.s PROPERTIES LANGUAGE ASM)

add_executable(${TARGET_ELF} ${APP_SOURCES} ${HAL_SOURCES})

target_include_directories(${TARGET_ELF} PRIVATE
    Core/Inc
    Drivers/STM32F1xx_HAL_Driver/Inc
    Drivers/CMSIS/Include
    Drivers/CMSIS/Device/ST/STM32F1xx/Include
)

target_compile_definitions(${TARGET_ELF} PRIVATE
    STM32F103xE
    USE_HAL_DRIVER
    DEBUG_LOG_LEVEL=${DEBUG_LOG_LEVEL}
)

target_compile_options(${TARGET_ELF} PRIVATE
    -Wall
    -Wextra
    -ffunction-sections
    -fdata-sections
)

set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/STM32F103ZETx_FLASH.ld")
target_link_options(${TARGET_ELF} PRIVATE
    "-T${LINKER_SCRIPT}"
    "-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.map,--cref"
    -Wl,--gc-sections
)
set_target_properties(${TARGET_ELF} PROPERTIES LINK_DEPENDS "${LINKER_SCRIPT}")

add_custom_command(TARGET ${TARGET_ELF} POST_BUILD
    COMMAND "${CMAKE_OBJCOPY}" -O ihex   "$<TARGET_FILE:${TARGET_ELF}>" "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.hex"
    COMMAND "${CMAKE_OBJCOPY}" -O binary -S "$<TARGET_FILE:${TARGET_ELF}>" "${CMAKE_CURRENT_BINARY_DIR}/${TARGET_NAME}.bin"
    COMMAND "${CMAKE_SIZE}" --format=berkeley "$<TARGET_FILE:${TARGET_ELF}>"
    COMMENT "Generate .hex/.bin and report size"
)
