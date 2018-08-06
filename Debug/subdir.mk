################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CU_SRCS += \
../scheduler.cu 

CPP_SRCS += \
../eth_socket_example.cpp \
../packets.cpp \
../req_receiver.cpp \
../schedThread.cpp \
../send_grant.cpp 

O_SRCS += \
../packets.o 

OBJS += \
./eth_socket_example.o \
./packets.o \
./req_receiver.o \
./schedThread.o \
./scheduler.o \
./send_grant.o 

CU_DEPS += \
./scheduler.d 

CPP_DEPS += \
./eth_socket_example.d \
./packets.d \
./req_receiver.d \
./schedThread.d \
./send_grant.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -G -g -O0 -gencode arch=compute_53,code=sm_53  -odir "." -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -G -g -O0 --compile  -x c++ -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

%.o: ../%.cu
	@echo 'Building file: $<'
	@echo 'Invoking: NVCC Compiler'
	/usr/local/cuda-8.0/bin/nvcc -G -g -O0 -gencode arch=compute_53,code=sm_53  -odir "." -M -o "$(@:%.o=%.d)" "$<"
	/usr/local/cuda-8.0/bin/nvcc -G -g -O0 --compile --relocatable-device-code=false -gencode arch=compute_53,code=compute_53 -gencode arch=compute_53,code=sm_53  -x cu -o  "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


