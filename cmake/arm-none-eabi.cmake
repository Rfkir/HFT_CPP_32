set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_PROCESSOR arm)

# Derleyici yollarını kendi Vitis/GNU yoluna göre güncelle!
set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)

# Zybo (Cortex-A9) için CPU Flag'leri
set(CPU_FLAGS "-mcpu=cortex-a9 -mfpu=vfpv3 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -ffunction-sections -fdata-sections" CACHE STRING "")
set(CMAKE_CXX_FLAGS "${CPU_FLAGS} -Wall -ffunction-sections -fdata-sections" CACHE STRING "")