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
all: TPFINAL_ED3.axf

# Tool invocations
TPFINAL_ED3.axf: $(OBJS) $(USER_OBJS)
	@echo 'Building target: $@'
	@echo 'Invoking: MCU Linker'
	arm-none-eabi-gcc -nostdlib -L"/home/mathrisk/Documents/MCUXpresso_11.4.0_6224/TPFINAL_ED3/CMSISv2p00_LPC17xx/Debug" -Xlinker -Map="TPFINAL_ED3.map" -Xlinker --cref -Xlinker --gc-sections -Xlinker -print-memory-usage -mcpu=cortex-m3 -mthumb -T "TPFINAL_ED3_Debug.ld" -o "TPFINAL_ED3.axf" $(OBJS) $(USER_OBJS) $(LIBS)
	@echo 'Finished building target: $@'
	@echo ' '
	$(MAKE) --no-print-directory post-build

# Other Targets
clean:
	-$(RM) $(EXECUTABLES)$(OBJS)$(C_DEPS) TPFINAL_ED3.axf
	-@echo ' '

post-build:
	-@echo 'Performing post-build steps'
	-arm-none-eabi-size "TPFINAL_ED3.axf"; # arm-none-eabi-objcopy -v -O binary "TPFINAL_ED3.axf" "TPFINAL_ED3.bin" ; # checksum -p LPC1769 -d "TPFINAL_ED3.bin";
	-@echo ' '

.PHONY: all clean dependents post-build

-include ../makefile.targets
