################################################################################
# Automatically-generated file. Do not edit!
################################################################################

-include ../makefile.init

RM := rm -rf

# All of the sources participating in the build are defined here
-include sources.mk
-include src/subdir.mk
-include subdir.mk
-include objects.mk

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(strip $(C_DEPS)),)
-include $(C_DEPS)
endif
endif

-include ../makefile.defs

# Add inputs and outputs from these tool invocations to the build variables 

# All Target
all: Teclado.axf

# Tool invocations
Teclado.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/home/mathrisk/Desktop/REPOS/ED3_TP_Integrador/AFINADOR_ED3/CMSISv2p00_LPC17xx/Debug" -Xlinker -Map="Teclado.map" -Xlinker --cref -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m3 -mthumb -T "Teclado_Debug.ld" -o "Teclado.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) Teclado.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "Teclado.axf"; # arm-none-eabi-objcopy -v -O binary "Teclado.axf" "Teclado.bin" ; # checksum -p LPC1769 -d "Teclado.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
