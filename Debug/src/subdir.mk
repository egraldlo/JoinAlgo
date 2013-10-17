################################################################################
# 自动生成的文件。不要编辑！
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/Barrier.cpp \
../src/JoinAlgo.cpp \
../src/ProcessorMap.cpp \
../src/affinitizer.cpp \
../src/comparator.cpp \
../src/hash.cpp \
../src/joinerfactory.cpp \
../src/loader.cpp \
../src/main-serial.cpp \
../src/page.cpp \
../src/parser.cpp \
../src/partitioner.cpp \
../src/partitionerfactory.cpp \
../src/schema.cpp \
../src/table.cpp 

OBJS += \
./src/Barrier.o \
./src/JoinAlgo.o \
./src/ProcessorMap.o \
./src/affinitizer.o \
./src/comparator.o \
./src/hash.o \
./src/joinerfactory.o \
./src/loader.o \
./src/main-serial.o \
./src/page.o \
./src/parser.o \
./src/partitioner.o \
./src/partitionerfactory.o \
./src/schema.o \
./src/table.o 

CPP_DEPS += \
./src/Barrier.d \
./src/JoinAlgo.d \
./src/ProcessorMap.d \
./src/affinitizer.d \
./src/comparator.d \
./src/hash.d \
./src/joinerfactory.d \
./src/loader.d \
./src/main-serial.d \
./src/page.d \
./src/parser.d \
./src/partitioner.d \
./src/partitionerfactory.d \
./src/schema.d \
./src/table.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp
	@echo '正在构建文件： $<'
	@echo '正在调用： GNU C++ 编译器'
	g++ -O3 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo '已结束构建： $<'
	@echo ' '


