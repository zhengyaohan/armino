armino_component_register(SRCS "multi_config_main.c"
                       INCLUDE_DIRS ".")

if(CONFIG_MULTI_CONFIG_DEBUG)
    target_sources(${COMPONENT_LIB} PRIVATE "multi_config_debug.c")
elseif(CONFIG_MULTI_CONFIG_RELEASE)
    target_sources(${COMPONENT_LIB} PRIVATE "multi_config_release.c")
endif()
