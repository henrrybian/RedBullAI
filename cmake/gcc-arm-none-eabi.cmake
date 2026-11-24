# 阶段 2 交叉工具链文件（AI 生成）
# 以下绝对路径全部来自 @docs/env_check.md 第 3 / 8.3 / 11 节的一键校验实测输出，
# 不凭记忆填写；CMake 4.5.0-rc3 与 Ninja 1.13.2 由会话级 PATH 或绝对路径调用。
set(CMAKE_SYSTEM_NAME      Generic)
set(CMAKE_SYSTEM_PROCESSOR  arm)
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(ARM_TOOLCHAIN_BIN "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/14.2 rel1/bin")
set(CMAKE_C_COMPILER   "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-g++.exe")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gcc.exe")
set(CMAKE_AR       "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-ar.exe"      CACHE FILEPATH "" FORCE)
set(CMAKE_OBJCOPY  "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-objcopy.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_SIZE     "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-size.exe"    CACHE FILEPATH "" FORCE)
set(CMAKE_GDB      "${ARM_TOOLCHAIN_BIN}/arm-none-eabi-gdb.exe"     CACHE FILEPATH "" FORCE)

set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb -mfloat-abi=soft")
set(CMAKE_C_FLAGS_INIT   "${CPU_FLAGS}")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
