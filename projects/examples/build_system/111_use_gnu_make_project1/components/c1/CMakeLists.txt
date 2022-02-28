armino_component_register(SRCS c1.c INCLUDE_DIRS include)

include(ExternalProject)
ExternalProject_Add(foo_project
	SOURCE_DIR ${COMPONENT_DIR}/foo
	BUILD_IN_SOURCE 1
	BUILD_COMMAND make CC=${CMAKE_C_COMPILER} libfoo.a
	BUILD_ALWAYS 1
	)

add_custom_target(foo_build DEPENDS foo_project)
add_library(foo STATIC IMPORTED GLOBAL)
add_dependencies(foo foo_build)
set_target_properties(foo PROPERTIES IMPORTED_LOCATION ${COMPONENT_DIR}/foo/libfoo.a)
set_target_properties(foo PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${COMPONENT_DIR}/foo/include)
target_link_libraries(${COMPONENT_LIB} PUBLIC foo)
