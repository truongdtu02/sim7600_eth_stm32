################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Src/AES_128.c \
../Drivers/Src/bignum.c \
../Drivers/Src/debugAPI.c \
../Drivers/Src/ethLAN.c \
../Drivers/Src/flash.c \
../Drivers/Src/md5.c \
../Drivers/Src/mp3Frame.c \
../Drivers/Src/rsa.c \
../Drivers/Src/sim7600_uart_dma.c \
../Drivers/Src/tcp_udp_stack.c 

OBJS += \
./Drivers/Src/AES_128.o \
./Drivers/Src/bignum.o \
./Drivers/Src/debugAPI.o \
./Drivers/Src/ethLAN.o \
./Drivers/Src/flash.o \
./Drivers/Src/md5.o \
./Drivers/Src/mp3Frame.o \
./Drivers/Src/rsa.o \
./Drivers/Src/sim7600_uart_dma.o \
./Drivers/Src/tcp_udp_stack.o 

C_DEPS += \
./Drivers/Src/AES_128.d \
./Drivers/Src/bignum.d \
./Drivers/Src/debugAPI.d \
./Drivers/Src/ethLAN.d \
./Drivers/Src/flash.d \
./Drivers/Src/md5.d \
./Drivers/Src/mp3Frame.d \
./Drivers/Src/rsa.d \
./Drivers/Src/sim7600_uart_dma.d \
./Drivers/Src/tcp_udp_stack.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Src/%.o: ../Drivers/Src/%.c Drivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -DUSE_FULL_LL_DRIVER -DUSE_HAL_DRIVER -DSTM32F407xx -c -I../Core/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I"E:/truyenthanhproject/STM32CubeIDE/workspace_1.6.1/sim7600LL/Drivers/Inc" -I../LWIP/App -I../LWIP/Target -I../Middlewares/Third_Party/LwIP/src/include -I../Middlewares/Third_Party/LwIP/system -I../Middlewares/Third_Party/LwIP/src/include/netif/ppp -I../Middlewares/Third_Party/LwIP/src/include/lwip -I../Middlewares/Third_Party/LwIP/src/include/lwip/apps -I../Middlewares/Third_Party/LwIP/src/include/lwip/priv -I../Middlewares/Third_Party/LwIP/src/include/lwip/prot -I../Middlewares/Third_Party/LwIP/src/include/netif -I../Middlewares/Third_Party/LwIP/src/include/compat/posix -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/arpa -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/net -I../Middlewares/Third_Party/LwIP/src/include/compat/posix/sys -I../Middlewares/Third_Party/LwIP/src/include/compat/stdc -I../Middlewares/Third_Party/LwIP/system/arch -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

