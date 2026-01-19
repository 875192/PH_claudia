################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../8led.c \
../Bmp.c \
../button.c \
../celda.c \
../cola.c \
../lcd.c \
../led.c \
../main.c \
../sudoku_2025.c \
../sudoku_lcd.c \
../tableros.c \
../timer.c \
../timer1.c \
../timer2.c \
../tp.c 

ASM_SRCS += \
../init_b.asm 

OBJS += \
./8led.o \
./Bmp.o \
./button.o \
./celda.o \
./cola.o \
./init_b.o \
./lcd.o \
./led.o \
./main.o \
./sudoku_2025.o \
./sudoku_lcd.o \
./tableros.o \
./timer.o \
./timer1.o \
./timer2.o \
./tp.o 

C_DEPS += \
./8led.d \
./Bmp.d \
./button.d \
./celda.d \
./cola.d \
./lcd.d \
./led.d \
./main.d \
./sudoku_2025.d \
./sudoku_lcd.d \
./tableros.d \
./timer.d \
./timer1.d \
./timer2.d \
./tp.d 

ASM_DEPS += \
./init_b.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC C Compiler'
	arm-none-eabi-gcc -I"C:\hlocal\workspace_Hardware\practica3\common" -O0 -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -mapcs-frame -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.asm
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Sourcery Windows GCC Assembler'
	arm-none-eabi-gcc -x assembler-with-cpp -I"C:\hlocal\workspace_Hardware\practica3\common" -Wall -Wa,-adhlns="$@.lst" -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -mcpu=arm7tdmi -g3 -gdwarf-2 -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


