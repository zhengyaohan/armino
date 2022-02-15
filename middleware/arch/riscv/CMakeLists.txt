armino_build_get_property(target BDK_SOC)
set(incs 
   . 
   include
   include/bk_private)

set(srcs
	boot_handlers_base.S
	stack_base.c
	trap_base.c
	arch_interrupt.c
	reset.c
	ae350.c
	initfini.c
	riscv.c
	)

if(CONFIG_FREERTOS)
	list(APPEND incs os/freertos)
	list(APPEND srcs
		os/freertos/cpu.c
		boot_vectors_base.S
	)
endif()

if(CONFIG_LITEOS_M_V3)
	list(APPEND srcs
		boot_vectors_liteos_m.S
	)
endif()

if("${target}" STREQUAL "bk7256" OR "${target}" STREQUAL "bk7256_cp1")
	armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	)
endif()
