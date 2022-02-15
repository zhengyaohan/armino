#TODO
# 1. Move BT libs to bk_bt
# 2. Move WiFi libs to bk_wifi
# 3. Move BLE libs to bk_ble

armino_build_get_property(armino_soc BDK_SOC)

set(incs include/bk_private)
set(srcs sys_config.c sys_config_check.c)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_config driver bk_common)

#TODO don't use GLOB

file(GLOB common_libs ${CMAKE_CURRENT_SOURCE_DIR}/common/*.a)
foreach(lib ${common_libs})
	get_filename_component(lib_name ${lib} NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/common/${lib_name}")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()

file(GLOB libs ${CMAKE_CURRENT_SOURCE_DIR}/${armino_soc}/*.a)

foreach(lib ${libs})
 	get_filename_component(lib_name ${lib} NAME)
 	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/${armino_soc}/${lib_name}")
 	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endforeach()

