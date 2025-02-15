#include "spot_storm.h"
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include "light_array.h"
#include <stdio.h>
#include <math.h>
#include "imgui.h"
#include "graphics.h"
#include "obj.h"
#include "gjk.h"
#include "epa.h"
#include "clipping.h"
#include "pbd.h"
#include "entity.h"
#include "util.h"
#include "examples_util.h"

static Perspective_Camera camera;
static Light* lights;
static r64 thrown_objects_initial_linear_velocity_norm = 15.0;

static Perspective_Camera create_camera() {
	Perspective_Camera camera;
	vec3 camera_position = { -15.0, 15.0, 25.0 };
	r64 camera_near_plane = -0.01;
	r64 camera_far_plane = -1000.0;
	r64 camera_fov = 45.0;
	camera_init(&camera, camera_position, camera_near_plane, camera_far_plane, camera_fov);
	camera.rotation = {0.210476, 0.000000, 0.000000, 0.977617};
	camera.yrotation = {0.000000, 0.303208, 0.000000, 0.952945};
	camera.position = {-16.194581, 15.336559, 25.110162};
	camera_force_matrix_recalculation(&camera);
	return camera;
}

static Collider* create_spot_colliders(vec3 scale, Vertex** hulls_vertices, u32** hulls_indices) {
	Collider* spot_colliders = array_new(Collider);
	Collider collider;

	for (u32 i = 0; i < array_length(hulls_vertices); ++i) {
		collider = examples_util_create_convex_hull_collider(hulls_vertices[i], hulls_indices[i], scale);
		array_push(spot_colliders, collider);
	}

	return spot_colliders;
}

static Quaternion generate_random_quaternion() {
	r64 x = rand() / (r64)RAND_MAX;
	r64 y = rand() / (r64)RAND_MAX;
	r64 z = rand() / (r64)RAND_MAX;
	r64 angle = rand() / (r64)RAND_MAX;
	angle = -180.0 + angle * 360.0;
	return quaternion_new({x, y, z}, angle);
}

int ex_spot_storm_init() {
	entity_module_init();

	// Create camera
	camera = create_camera();
	// Create light
	lights = examples_util_create_lights();

	Vertex* cube_vertices;
	u32* cube_indices;
	obj_parse("./res/cube.obj", &cube_vertices, &cube_indices);
	Mesh cube_mesh = graphics_mesh_create(cube_vertices, cube_indices);

	vec3 floor_scale = {50.0, 1.0, 50.0};
	Collider* floor_colliders = examples_util_create_single_convex_hull_collider_array(cube_vertices, cube_indices, floor_scale);
	entity_create_fixed(cube_mesh, {0.0, -2.0, 0.0}, quaternion_new({0.0, 1.0, 0.0}, 0.0),
		floor_scale, {1.0, 1.0, 1.0, 1.0}, floor_colliders, 0.5, 0.5, 0.0);
	
	Vertex* spot_vertices;
	u32* spot_indices;
	obj_parse("./res/spot/spot.obj", &spot_vertices, &spot_indices);
	Mesh spot_mesh = graphics_mesh_create(spot_vertices, spot_indices);
	vec3 spot_scale = {2.0, 2.0, 2.0};

	Vertex** hulls_vertices = array_new(Vertex*);
	u32** hulls_indices = array_new(u32*);
	Vertex* hull_vertices;
	u32* hull_indices;
	obj_parse("./res/spot/spot-hull-1.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-2.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-3.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-4.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-5.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-6.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-7.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-8.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-9.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-10.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);
	obj_parse("./res/spot/spot-hull-11.obj", &hull_vertices, &hull_indices);
	array_push(hulls_vertices, hull_vertices);
	array_push(hulls_indices, hull_indices);

	const u32 N = 2;
	r64 y = 2.0;
	r64 gap = 3.5;
	for (u32 i = 0; i < N; ++i) {
		y += gap;

		r64 x = -2.0 * (N / 2.0);
		for (u32 j = 0; j < N; ++j) {
			x += gap;

			r64 z = -2.0 * (N / 2.0);
			for (u32 k = 0; k < N; ++k) {
				z += gap;

				Collider* spot_colliders = create_spot_colliders(spot_scale, hulls_vertices, hulls_indices);
				entity_create(spot_mesh, {x, y, z}, generate_random_quaternion(),
					spot_scale, util_pallete(i + j + k), 1.0, spot_colliders, 0.8, 0.8, 0.0);
			}
		}
	}

	array_free(cube_vertices);
	array_free(cube_indices);
	array_free(spot_vertices);
	array_free(spot_indices);
	for (u32 i = 0; i < array_length(hulls_vertices); ++i) {
		array_free(hulls_vertices[i]);
		array_free(hulls_indices[i]);
	}
	array_free(hulls_vertices);
	array_free(hulls_indices);

	return 0;
}

void ex_spot_storm_destroy() {
	array_free(lights);

	Entity** entities = entity_get_all();
	for (u32 i = 0; i < array_length(entities); ++i) {
		Entity* e = entities[i];
		colliders_destroy(e->colliders);
		array_free(e->colliders);
		mesh_destroy(&e->mesh);
		entity_destroy(e);
	}
	array_free(entities);
	entity_module_destroy();
}

void ex_spot_storm_update(r64 delta_time) {
	//printf("{%f, %f, %f, %f}\n", camera.rotation.x, camera.rotation.y, camera.rotation.z, camera.rotation.w);
	//printf("{%f, %f, %f, %f}\n", camera.yrotation.x, camera.yrotation.y, camera.yrotation.z, camera.yrotation.w);
	//printf("{%f, %f, %f}\n", camera.position.x, camera.position.y, camera.position.z);

	Entity** entities = entity_get_all();
	for (u32 i = 0; i < array_length(entities); ++i) {
		Entity* e = entities[i];
		colliders_update(e->colliders, e->world_position, &e->world_rotation);
	}

	const r64 GRAVITY = 10.0;
	for (u32 i = 0; i < array_length(entities); ++i) {
		entity_add_force(entities[i], {0.0, 0.0, 0.0}, {0.0, -GRAVITY * 1.0 / entities[i]->inverse_mass, 0.0}, false);
	}

	pbd_simulate(delta_time, entities, 1, 1, true);

	for (u32 i = 0; i < array_length(entities); ++i) {
		entity_clear_forces(entities[i]);
	}
	array_free(entities);
}

void ex_spot_storm_render() {
	Entity** entities = entity_get_all();
	for (u32 i = 0; i < array_length(entities); ++i) {
		graphics_entity_render_phong_shader(&camera, entities[i], lights);
	}

	graphics_renderer_primitives_flush(&camera);
	array_free(entities);
}

void ex_spot_storm_input_process(boolean* key_state, r64 delta_time) {
	r64 movement_speed = 3.0;
	r64 rotation_speed = 300.0;

	if (key_state[GLFW_KEY_LEFT_SHIFT]) {
		movement_speed = 0.5;
	}
	if (key_state[GLFW_KEY_RIGHT_SHIFT]) {
		movement_speed = 0.01;
	}

	if (key_state[GLFW_KEY_W]) {
		camera_move_forward(&camera, movement_speed * delta_time);
	}
	if (key_state[GLFW_KEY_S]) {
		camera_move_forward(&camera, -movement_speed * delta_time);
	}
	if (key_state[GLFW_KEY_A]) {
		camera_move_right(&camera, -movement_speed * delta_time);
	}
	if (key_state[GLFW_KEY_D]) {
		camera_move_right(&camera, movement_speed * delta_time);
	}
	if (key_state[GLFW_KEY_L]) {
		static boolean wireframe = false;

		if (wireframe) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		} else {
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}

		wireframe = !wireframe;
		key_state[GLFW_KEY_L] = false;
	}

	if (key_state[GLFW_KEY_SPACE]) {
		examples_util_throw_object(&camera, thrown_objects_initial_linear_velocity_norm);
		key_state[GLFW_KEY_SPACE] = false;
	}
}

void ex_spot_storm_mouse_change_process(boolean reset, r64 x_pos, r64 y_pos) {
	static const r64 camera_mouse_speed = 0.1;
	static r64 x_pos_old, y_pos_old;

	r64 x_difference = x_pos - x_pos_old;
	r64 y_difference = y_pos - y_pos_old;

	x_pos_old = x_pos;
	y_pos_old = y_pos;

	if (reset) return;

	camera_rotate_x(&camera, camera_mouse_speed * (r64)x_difference);
	camera_rotate_y(&camera, camera_mouse_speed * (r64)y_difference);
}

void ex_spot_storm_mouse_click_process(s32 button, s32 action, r64 x_pos, r64 y_pos) {

}

void ex_spot_storm_scroll_change_process(r64 x_offset, r64 y_offset) {

}

void ex_spot_storm_window_resize_process(s32 width, s32 height) {
	camera_force_matrix_recalculation(&camera);
}

void ex_spot_storm_menu_update() {
	ImGui::Text("Spot Storm");
	ImGui::Separator();

	ImGui::TextWrapped("Press SPACE to throw objects!");
	ImGui::TextWrapped("Thrown objects initial linear velocity norm:");
	r32 vel = (r32)thrown_objects_initial_linear_velocity_norm;
	ImGui::SliderFloat("Vel", &vel, 1.0f, 30.0f, "%.2f");
	thrown_objects_initial_linear_velocity_norm = vel;
}

Example_Scene spot_storm_example_scene = {
	.name = "Spot Storm",
	.init = ex_spot_storm_init,
	.destroy = ex_spot_storm_destroy,
	.input_process = ex_spot_storm_input_process,
	.menu_properties_update = ex_spot_storm_menu_update,
	.mouse_change_process = ex_spot_storm_mouse_change_process,
	.mouse_click_process = ex_spot_storm_mouse_click_process,
	.render = ex_spot_storm_render,
	.scroll_change_process = ex_spot_storm_scroll_change_process,
	.update = ex_spot_storm_update,
	.window_resize_process = ex_spot_storm_window_resize_process
};