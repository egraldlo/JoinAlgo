################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algo/base.cpp \
../src/algo/flatmem.cpp \
../src/algo/hashbase.cpp \
../src/algo/hashtable.cpp \
../src/algo/nl.cpp \
../src/algo/storage.cpp 

OBJS += \
./src/algo/base.o \
./src/algo/flatmem.o \
./src/algo/hashbase.o \
./src/algo/hashtable.o \
./src/algo/nl.o \
./src/algo/storage.o 

CPP_DEPS += \
./src/algo/base.d \
./src/algo/flatmem.d \
./src/algo/hashbase.d \
./src/algo/hashtable.d \
./src/algo/nl.d \
./src/algo/storage.d 


# Each subdirectory must supply rules for building sources it contributes
src/algo/%.o: ../src/algo/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GNU C++ 编译器'
	g++ -O3 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


