armino_build_get_property(target BDK_SOC)
set(incs
	.
	include/bk_private
	# os/freertos
	)

set(srcs
	boot_handlers_base.S
	boot_vectors_base.S
	stack_base.c
	trap_base.c
	# os/freertos/boot_handlers.S
	# os/freertos/cpu.c
	arm.c
	arch_interrupt.c
	)

if(CONFIG_FREERTOS)
	list(APPEND incs os/freertos)
	list(APPEND srcs os/freertos/boot_handlers.S os/freertos/cpu.c)
elseif(CONFIG_LITEOS_M)
	list(APPEND incs os/liteos_m)
	list(APPEND srcs os/liteos_m/boot_handlers.S os/liteos_m/cpu.c)
endif()


if("${target}" STREQUAL "bk7256" OR "${target}" STREQUAL "bk7256_cp1")

else()
	armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common bk_init)
endif()
