# solver2d samples app

# glad for OpenGL API
set(GLAD_DIR ${CMAKE_SOURCE_DIR}/glad)

add_library(
	glad STATIC
	${GLAD_DIR}/src/glad.c
	${GLAD_DIR}/include/glad/glad.h
	${GLAD_DIR}/include/KHR/khrplatform.h
)
target_include_directories(glad PUBLIC ${GLAD_DIR}/include)

# glfw for windowing and input
SET(GLFW_BUILD_DOCS OFF CACHE BOOL "GLFW Docs")
SET(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "GLFW Examples")
SET(GLFW_BUILD_TESTS OFF CACHE BOOL "GLFW Tests")
SET(GLFW_INSTALL OFF CACHE BOOL "GLFW Install")

FetchContent_Declare(
	glfw
	GIT_REPOSITORY https://github.com/glfw/glfw.git
	GIT_TAG master
	GIT_SHALLOW TRUE
	GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(glfw)

set(SAMPLES_SOURCE
	common.h
	gm.h
	hash_map.h
	light_array.h
	stb_image_write.h
	stb_image.h
	tiny_obj_loader.h

	core.cpp
	core.h
	entity.cpp
	entity.h
	main.cpp
	quaternion.cpp
	quaternion.h
	util.cpp
	util.h

	triple_pendula.h
	arm.cpp
	arm.h
	brick_wall.cpp
	brick_wall.h
	coin.cpp
	coin.h
	cube_and_ramp.cpp
	cube_and_ramp.h
	cube_storm.cpp
	cube_storm.h
	debug.cpp
	debug.h
	examples_util.cpp
	examples_util.h
	hinge_joints.cpp
	hinge_joints.h
	mirror_cube.cpp
	mirror_cube.h
	rott_pendulum.cpp
	rott_pendulum.h
	seesaw.cpp
	seesaw.h
	spot_storm.cpp
	spot_storm.h
	spring.cpp
	spring.h
	stack.cpp
	stack.h
	triple_pendula.cpp

	support.h
	broad.cpp
	broad.h
	clipping.cpp
	clipping.h
	collider.cpp
	collider.h
	epa.cpp
	epa.h
	gjk.cpp
	gjk.h
	pbd.cpp
	pbd.h
	pbd_base_constraints.cpp
	pbd_base_constraints.h
	physics_util.cpp
	physics_util.h
	support.cpp

	obj.h
	camera.cpp
	camera.h
	graphics.cpp
	graphics.h
	menu.cpp
	menu.h
	mesh.cpp
	mesh.h
	obj.cpp

	imstb_truetype.h
	imconfig.h
	imgui.cpp
	imgui.h
	imgui_demo.cpp
	imgui_draw.cpp
	imgui_impl_glfw.cpp
	imgui_impl_glfw.h
	imgui_impl_opengl3.cpp
	imgui_impl_opengl3.h
	imgui_internal.h
	imgui_widgets.cpp
	imstb_rectpack.h
	imstb_textedit.h	
)

add_executable(samples ${SAMPLES_SOURCE})
target_link_libraries(samples PUBLIC glfw glad)

# message(STATUS "runtime = ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
# message(STATUS "binary = ${CMAKE_CURRENT_BINARY_DIR}")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SAMPLES_SOURCE})
