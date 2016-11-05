################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
third_party/fatfs/src/ff.obj: C:/ti/TivaWare_C_Series-2.1.2.111/third_party/fatfs/src/ff.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_15.12.1.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_15.12.1.LTS/include" --include_path="C:/ti/TivaWare_C_Series-2.1.2.111" --include_path="C:/ti/TivaWare_C_Series-2.1.2.111/examples/boards/dk-tm4c123g" --include_path="C:/ti/TivaWare_C_Series-2.1.2.111/third_party" -g --gcc --define=ccs="ccs" --define=TARGET_IS_BLIZZARD_RB1 --define=PART_TM4C123GH6PM --diag_wrap=off --diag_warning=225 --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="third_party/fatfs/src/ff.d" --obj_directory="third_party/fatfs/src" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


