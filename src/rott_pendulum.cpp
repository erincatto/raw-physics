#include "rott_pendulum.h"
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
static Constraint* constraints;

static Perspective_Camera create_camera() {
	Perspective_Camera camera;
	vec3 camera_position = { 0.0, 5.0, 15.0 };
	r64 camera_near_plane = -0.01;
	r64 camera_far_plane = -1000.0;
	r64 camera_fov = 45.0;
	camera_init(&camera, camera_position, camera_near_plane, camera_far_plane, camera_fov);
	return camera;
}

static void reset_joint_distance(Entity* e1, Entity* e2, vec3 r1_lc, vec3 r2_lc) {
	mat3 e1_rot_matrix = quaternion_get_matrix3(&e1->world_rotation);
	mat3 e2_rot_matrix = quaternion_get_matrix3(&e2->world_rotation);
	vec3 r1_wc = gm_mat3_multiply_vec3(&e1_rot_matrix, r1_lc);
	vec3 r2_wc = gm_mat3_multiply_vec3(&e2_rot_matrix, r2_lc);
	vec3 p1 = gm_vec3_add(e1->world_position, r1_wc);
	vec3 p2 = gm_vec3_add(e2->world_position, r2_wc);
	vec3 delta_r = gm_vec3_subtract(p1, p2);
	vec3 delta_x = delta_r;
	entity_set_position(e2, gm_vec3_add(e2->world_position, delta_x));
}

static eid base_id, static_piece_id, free_piece_id;

static Constraint* create_pendulum() {
	Vertex* cube_vertices;
	u32* cube_indices;

	obj_parse("./res/cube.obj", &cube_vertices, &cube_indices);
	Mesh cube_mesh = graphics_mesh_create(cube_vertices, cube_indices);

	vec3 support_position = {0.0, 0.0, -2.0};
	vec3 support_collider_scale = {0.1, 0.1, 0.1};
	Collider* support_colliders = examples_util_create_single_convex_hull_collider_array(cube_vertices, cube_indices, support_collider_scale);
	eid support_id = entity_create_fixed(cube_mesh, support_position, quaternion_new({0.0, 0.0, 0.0}, 0.0), support_collider_scale,
		{0.0, 1.0, 0.0, 1.0}, support_colliders, 0.5, 0.5, 0.0);

	vec3 base_collider_scale = {1.0, 0.1, 0.1};
	Collider* base_colliders = examples_util_create_single_convex_hull_collider_array(cube_vertices, cube_indices, base_collider_scale);
	base_id = entity_create(cube_mesh, {0.0, 0.0, 0.0}, quaternion_new({0.0, 0.0, 0.0}, 0.0), base_collider_scale,
		{0x77 / 255.0, 0xc3 / 255.0, 0xec / 255.0}, 1.0, base_colliders, 0.6, 0.6, 0.0);

	vec3 free_piece_collider_scale = {0.1, 1.0, 0.1};
	Collider* free_piece_colliders = examples_util_create_single_convex_hull_collider_array(cube_vertices, cube_indices, free_piece_collider_scale);
	free_piece_id = entity_create(cube_mesh, {0.0, 0.0, 0.0}, quaternion_new({0.0, 0.0, 0.0}, 0.0), free_piece_collider_scale,
		{1.0, 0.0, 0.0, 1.0}, 1.0, free_piece_colliders, 0.6, 0.6, 0.0);

	vec3 static_piece_collider_scale = {0.1, 1.0, 0.1};
	Collider* static_piece_colliders = examples_util_create_single_convex_hull_collider_array(cube_vertices, cube_indices, static_piece_collider_scale);
	static_piece_id = entity_create(cube_mesh, {0.0, 0.0, 0.0}, quaternion_new({0.0, 0.0, 0.0}, 0.0), static_piece_collider_scale,
		{0x77 / 255.0, 0xc3 / 255.0, 0xec / 255.0}, 1.0, static_piece_colliders, 0.6, 0.6, 0.0);

	array_free(cube_vertices);
	array_free(cube_indices);

	Constraint* constraints = array_new(Constraint);
	Constraint constraint;

	Entity* support_entity = entity_get_by_id(support_id);
	Entity* base_entity = entity_get_by_id(base_id);
	Entity* free_piece_entity = entity_get_by_id(free_piece_id);
	Entity* static_piece_entity = entity_get_by_id(static_piece_id);

	vec3 r1_lc, r2_lc;

	// Support - Base Joint
	r1_lc = {0.0, 0.0, 2.0};
	r2_lc = {0.0, 0.0, 0.0};
	reset_joint_distance(support_entity, base_entity, r1_lc, r2_lc);
	pbd_hinge_joint_constraint_unlimited_init(&constraint, support_id, base_id, r1_lc, r2_lc, 0.0, PBD_POSITIVE_Z_AXIS, PBD_POSITIVE_Z_AXIS);
	array_push(constraints, constraint);

	// Base - Free Piece Joint
	r1_lc = { 0.9, 0.0, 0.0 };
	r2_lc = { 0.0, 1.1, -0.25 };
	reset_joint_distance(base_entity, free_piece_entity, r1_lc, r2_lc);
	pbd_hinge_joint_constraint_unlimited_init(&constraint, base_id, free_piece_id, r1_lc, r2_lc, 0.0, PBD_POSITIVE_Z_AXIS, PBD_POSITIVE_Z_AXIS);
	array_push(constraints, constraint);

	// Base - Static Piece Joint
	r1_lc = { -0.9, 0.0, 0.0 };
	r2_lc = { 0.0, 1.1, 0.0 };
	reset_joint_distance(base_entity, static_piece_entity, r1_lc, r2_lc);
	pbd_hinge_joint_constraint_limited_init(&constraint, base_id, static_piece_id, r1_lc, r2_lc, 0.0, PBD_POSITIVE_Z_AXIS, PBD_POSITIVE_Z_AXIS, PBD_POSITIVE_Y_AXIS, PBD_POSITIVE_Y_AXIS,
		0.0, 0.0);
	array_push(constraints, constraint);

	return constraints;
}

int ex_rott_pendulum_init() {
	entity_module_init();

	// Create camera
	camera = create_camera();
	// Create light
	lights = examples_util_create_lights();

	constraints = create_pendulum();

	return 0;
}

void ex_rott_pendulum_destroy() {
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
	array_free(constraints);
	entity_module_destroy();
}

void ex_rott_pendulum_update(r64 delta_time) {
	Entity** entities = entity_get_all();
	for (u32 i = 0; i < array_length(entities); ++i) {
		Entity* e = entities[i];
		colliders_update(e->colliders, e->world_position, &e->world_rotation);
		//printf("e%d: <%.50f, %.50f, %.50f>\n", i, e->world_position.x, e->world_position.y, e->world_position.z);
		//printf("e%d: rot: <%.50f, %.50f, %.50f, %.50f>\n", i, e->world_rotation.x, e->world_rotation.y, e->world_rotation.z, e->world_rotation.w);
	}

	const r64 GRAVITY = 10.0;
	for (u32 i = 0; i < array_length(entities); ++i) {
		entity_add_force(entities[i], {0.0, 0.0, 0.0}, {0.0, -GRAVITY * 1.0 / entities[i]->inverse_mass, 0.0}, false);
	}

	pbd_simulate_with_constraints(delta_time, entities, constraints, 50, 50, false);

	for (u32 i = 0; i < array_length(entities); ++i) {
		entity_clear_forces(entities[i]);
	}
	array_free(entities);
}

void ex_rott_pendulum_render() {
	Entity** entities = entity_get_all();

	for (u32 i = 0; i < array_length(entities); ++i) {
		graphics_entity_render_phong_shader(&camera, entities[i], lights);
	}

	graphics_renderer_primitives_flush(&camera);
	array_free(entities);
}

void ex_rott_pendulum_input_process(boolean* key_state, r64 delta_time) {
	r64 movement_speed = 30.0;
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

	if (key_state[GLFW_KEY_B]) {
		Entity* base_entity = entity_get_by_id(base_id);
		entity_add_force(base_entity, {1.0, 0.0, 0.0}, {0.0, -200.0, 0.0}, true);
		entity_activate(base_entity);
	}

	if (key_state[GLFW_KEY_V]) {
		Entity* base_entity = entity_get_by_id(base_id);
		Entity* static_piece_entity = entity_get_by_id(static_piece_id);
		Entity* free_piece_entity = entity_get_by_id(free_piece_id);
		base_entity->linear_velocity = {0.0, 0.0, 0.0};
		base_entity->angular_velocity = {0.0, 0.0, 0.0};
		static_piece_entity->linear_velocity = {0.0, 0.0, 0.0};
		static_piece_entity->angular_velocity = {0.0, 0.0, 0.0};
		free_piece_entity->linear_velocity = {0.0, 0.0, 0.0};
		free_piece_entity->angular_velocity = {0.0, 0.0, 0.0};
		key_state[GLFW_KEY_V] = false;
	}
}

void ex_rott_pendulum_mouse_change_process(boolean reset, r64 x_pos, r64 y_pos) {
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

void ex_rott_pendulum_mouse_click_process(s32 button, s32 action, r64 x_pos, r64 y_pos) {

}

void ex_rott_pendulum_scroll_change_process(r64 x_offset, r64 y_offset) {

}

void ex_rott_pendulum_window_resize_process(s32 width, s32 height) {
	camera_force_matrix_recalculation(&camera);
}

void ex_rott_pendulum_menu_update() {
	ImGui::Text("Rott Pendulum");
	ImGui::Separator();

	ImGui::TextWrapped("Press B to apply a force!");
	ImGui::TextWrapped("Press V to zero velocities!");
}

Example_Scene rott_pendulum_example_scene = {
	.name = "Rott Pendulum",
	.init = ex_rott_pendulum_init,
	.destroy = ex_rott_pendulum_destroy,
	.input_process = ex_rott_pendulum_input_process,
	.menu_properties_update = ex_rott_pendulum_menu_update,
	.mouse_change_process = ex_rott_pendulum_mouse_change_process,
	.mouse_click_process = ex_rott_pendulum_mouse_click_process,
	.render = ex_rott_pendulum_render,
	.scroll_change_process = ex_rott_pendulum_scroll_change_process,
	.update = ex_rott_pendulum_update,
	.window_resize_process = ex_rott_pendulum_window_resize_process
};