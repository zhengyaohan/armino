#TODO
# 1. Move BT libs to bk_bt
# 2. Move WiFi libs to bk_wifi
# 3. Move BLE libs to bk_ble

armino_build_get_property(armino_soc ARMINO_SOC)

set(incs)
set(srcs)
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES driver bk_common)

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

