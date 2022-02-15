set(incs .)

if (CONFIG_WPA3)
list(APPEND srcs
	"wolfcrypt/src/wolfmath.c"
	"wolfcrypt/src/memory.c"
	"wolfcrypt/src/tfm.c"
	"wolfcrypt/src/ecc.c"
	"wolfcrypt/src/random.c"
	)
endif()
armino_component_register(INTERNAL_LIB SRCS "${srcs}"
						  INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common wpa_supplicant-2.9)
