#
# Set the target used for the standard project build.
#
macro(__target_init)
    # Input is BDK_SOC environement variable
    set(env_armino_target $ENV{BDK_SOC})

    if(NOT env_armino_target)
        # BDK_SOC not set in environment, see if it is set in cache
        if(BDK_SOC)
            set(env_armino_target ${BDK_SOC})
        else()
            set(env_armino_target bk7231n)
            LOGI("BDK_SOC not set, using default target: ${env_armino_target}")
        endif()
    else()
        # BDK_SOC set both in environment and in cache, must be the same
        if(NOT ${BDK_SOC} STREQUAL ${env_armino_target})
            LOGE("BDK_SOC in CMake cache does not match "
                            "BDK_SOC environment variable. To change the target, clear "
                            "the build directory and sdkconfig file, and build the project again")
        endif()
    endif()

    # BDK_SOC will be used by Kconfig, make sure it is set
    set(ENV{BDK_SOC} ${env_armino_target})

    # Finally, set BDK_SOC in cache
    set(BDK_SOC ${env_armino_target} CACHE STRING "BDK Build Target")

    # Check if BDK_ENV_FPGA environment is set
    set(env_armino_env_fpga $ENV{BDK_ENV_FPGA})
    if(${env_armino_env_fpga})
        armino_build_set_property(__BDK_ENV_FPGA "y")
        LOGI("BDK_ENV_FPGA is set, building for FPGA environment")
    endif()
endmacro()

#
# Check that the set build target and the config target matches.
#
function(__target_check)
    # Should be called after sdkconfig CMake file has been included.
    armino_build_get_property(armino_target BDK_SOC)

    if (${CONFIG_SOC_BK7231N})
	set(config_armino_target "bk7231n")
    endif()

    if (${CONFIG_SOC_BK7231U})
	set(config_armino_target "bk7231u")
    endif()

    if (${CONFIG_SOC_BK7251})
	set(config_armino_target "bk7251")
    endif()

    if (${CONFIG_SOC_BK7271})
	set(config_armino_target "bk7271")
    endif()

    if (${CONFIG_SOC_BK7236})
	set(config_armino_target "bk7236")
    endif()

    if (${CONFIG_SOC_BK7256})
	set(config_armino_target "bk7256")
    endif()

    if (${CONFIG_SOC_BK7256_CP1})
	set(config_armino_target "bk7256_cp1")
    endif()

    if(NOT ${armino_target} STREQUAL ${config_armino_target})
        LOGE("CONFIG_SOC_BKxxx(${config_armino_target}) in sdkconfig does not match "
            "BDK_SOC(${armino_target}) environment variable. To change the target, delete "
            "sdkconfig file and build the project again.")
    endif()
endfunction()

#
# Used by the project CMake file to set the toolchain before project() call.
#
macro(__target_set_toolchain)
    armino_build_get_property(armino_path ARMINO_PATH)
    # First try to load the toolchain file from the tools/cmake/directory of BDK
    set(toolchain_file_global ${armino_path}/tools/build_tools/cmake/toolchain-${BDK_SOC}.cmake)
    if(EXISTS ${toolchain_file_global})
	LOGI("global toolchain file: ${toolchain_file_global}")
        set(CMAKE_TOOLCHAIN_FILE ${toolchain_file_global})
    else()
        __component_get_target(component_target ${BDK_SOC})
        if(NOT component_target)
            LOGE("Unable to resolve '${BDK_SOC}' for setting toolchain file.")
        endif()
        get_property(component_dir TARGET ${component_target} PROPERTY COMPONENT_DIR)
        # Try to load the toolchain file from the directory of BDK_SOC component
        set(toolchain_file_component ${component_dir}/toolchain-${BDK_SOC}.cmake)
        if(EXISTS ${toolchain_file_component})
            set(CMAKE_TOOLCHAIN_FILE ${toolchain_file_component})
        else()
            LOGE("Toolchain file toolchain-${BDK_SOC}.cmake not found,"
                    "checked ${toolchain_file_global} and ${toolchain_file_component}")
        endif()
    endif()
endmacro()
