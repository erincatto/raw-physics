#include "hinge_joints.h"
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
static r64 thrown_objects_initial_linear_velocity_norm = 15.0;

static Perspective_Camera create_camera() {
	Perspective_Camera camera;
	vec3 camera_position = { 0.0, 5.0, 15.0 };
	r64 camera_near_plane = -0.01;
	r64 camera_far_plane = -1000.0;
	r64 camera_fov = 45.0;
	camera_init(&camera, camera_position, camera_near_plane, camera_far_plane, camera_fov);
	return camera;
}

static Constraint create_lever(vec3 lever_position, Quaternion lever_rotation, r64 angle_limit) {
	Vertex* support_vertices;
	u32* support_indices;
	Vertex* lever_vertices;
	u32* lever_indices;

	obj_parse("./res/lever_support.obj", &support_vertices, &support_indices);
	Mesh support_mesh = graphics_mesh_create(support_vertices, support_indices);

	obj_parse("./res/lever.obj", &lever_vertices, &lever_indices);
	Mesh lever_mesh = graphics_mesh_create(lever_vertices, lever_indices);

	vec3 support_collider_scale = {1.0, 1.0, 1.0};
	Collider* support_colliders = examples_util_create_single_convex_hull_collider_array(support_vertices, support_indices, support_collider_scale);
	eid support_id = entity_create_fixed(support_mesh, lever_position, lever_rotation, support_collider_scale,
		{0.0, 1.0, 0.0, 1.0}, support_colliders, 0.5, 0.5, 0.0);

	vec3 lever_collider_scale = {1.0, 1.0, 1.0};
	Collider* lever_colliders = examples_util_create_single_convex_hull_collider_array(lever_vertices, lever_indices, lever_collider_scale);
	eid lever_id = entity_create(lever_mesh, lever_position, lever_rotation, lever_collider_scale,
		{1.0, 1.0, 0.0, 1.0}, 1.0, lever_colliders, 0.6, 0.6, 0.0);

	array_free(support_vertices);
	array_free(support_indices);
	array_free(lever_vertices);
	array_free(lever_indices);

	mat3 lever_rotation_matrix = quaternion_get_matrix3(&lever_rotation);

	Entity* support_entity = entity_get_by_id(support_id);
	Entity* lever_entity = entity_get_by_id(lever_id);

	// Make sure that support and lever starts with the correct distance, otherwise simulation will explode in the 1st frame
	vec3 r1_lc = {0.0, 0.0, 0.0};
	vec3 r2_lc = {0.0, 3.0, 0.0};
	vec3 r1_wc = gm_mat3_multiply_vec3(&lever_rotation_matrix, r1_lc);
	vec3 r2_wc = gm_mat3_multiply_vec3(&lever_rotation_matrix, r2_lc);
	vec3 p1 = gm_vec3_add(support_entity->world_position, r1_wc);
	vec3 p2 = gm_vec3_add(lever_entity->world_position, r2_wc);
	vec3 delta_r = gm_vec3_subtract(p1, p2);
	vec3 delta_x = delta_r;
	entity_set_position(lever_entity, gm_vec3_add(lever_entity->world_position, delta_x));

	Constraint constraint;
	pbd_hinge_joint_constraint_limited_init(&constraint, support_id, lever_id, r1_lc, r2_lc, 0.0, PBD_POSITIVE_X_AXIS, PBD_POSITIVE_X_AXIS, PBD_POSITIVE_Y_AXIS, PBD_POSITIVE_Y_AXIS,
		-PI_F * angle_limit, PI_F * angle_limit);

	return constraint;
}

int ex_hinge_joints_init() {
	entity_module_init();

	// Create camera
	camera = create_camera();
	// Create light
	lights = examples_util_create_lights();

	constraints = array_new(Constraint);
	Constraint c;
	
	c = create_lever({0.0, 0.0, 0.0}, quaternion_new({1.0, 0.0, 0.0}, 0.0), 0.9);
	array_push(constraints, c);

	c = create_lever({5.0, 0.0, 0.0}, quaternion_new({0.0, 0.0, 1.0}, 45.0), 0.5);
	array_push(constraints, c);

	c = create_lever({-5.0, 0.0, 0.0}, quaternion_new({0.0, 0.0, -1.0}, 90.0), 0.5);
	array_push(constraints, c);

	return 0;
}

void ex_hinge_joints_destroy() {
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

void ex_hinge_joints_update(r64 delta_time) {

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

	pbd_simulate_with_constraints(delta_time, entities, constraints, 20, 1, true);

	for (u32 i = 0; i < array_length(entities); ++i) {
		entity_clear_forces(entities[i]);
	}
	array_free(entities);
}

void ex_hinge_joints_render() {
	Entity** entities = entity_get_all();

	// Render the aligned axes of every joint
	for (u32 i = 0; i < array_length(constraints); ++i) {
		Constraint* c = &constraints[i];
		if (c->type == HINGE_JOINT_CONSTRAINT) {
			Entity* support_entity = entity_get_by_id(c->e1_id);
			Entity* lever_entity = entity_get_by_id(c->e2_id);
			vec3 e1_a_wc = quaternion_get_right(&support_entity->world_rotation);
			vec3 e2_a_wc = quaternion_get_right(&lever_entity->world_rotation);
			graphics_renderer_debug_vector(support_entity->world_position, gm_vec3_add(support_entity->world_position, e1_a_wc),
				{1.0, 0.0, 0.0, 1.0});
			graphics_renderer_debug_vector(lever_entity->world_position, gm_vec3_add(lever_entity->world_position, e2_a_wc),
				{0.0, 0.0, 0.0, 1.0});
		}
	}

	for (u32 i = 0; i < array_length(entities); ++i) {
		graphics_entity_render_phong_shader(&camera, entities[i], lights);
	}

	graphics_renderer_primitives_flush(&camera);
	array_free(entities);
}

void ex_hinge_joints_input_process(boolean* key_state, r64 delta_time) {
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

	if (key_state[GLFW_KEY_SPACE]) {
		examples_util_throw_object(&camera, thrown_objects_initial_linear_velocity_norm);
		key_state[GLFW_KEY_SPACE] = false;
	}
}

void ex_hinge_joints_mouse_change_process(boolean reset, r64 x_pos, r64 y_pos) {
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

void ex_hinge_joints_mouse_click_process(s32 button, s32 action, r64 x_pos, r64 y_pos) {

}

void ex_hinge_joints_scroll_change_process(r64 x_offset, r64 y_offset) {

}

void ex_hinge_joints_window_resize_process(s32 width, s32 height) {
	camera_force_matrix_recalculation(&camera);
}

void ex_hinge_joints_menu_update() {
	ImGui::Text("Hinge Joints");
	ImGui::Separator();

	ImGui::TextWrapped("Press SPACE to throw objects!");
	ImGui::TextWrapped("Thrown objects initial linear velocity norm:");
	r32 vel = (r32)thrown_objects_initial_linear_velocity_norm;
	ImGui::SliderFloat("Vel", &vel, 1.0f, 30.0f, "%.2f");
	thrown_objects_initial_linear_velocity_norm = vel;
}

Example_Scene hinge_joints_example_scene = {
	.name = "Hinge Joints",
	.init = ex_hinge_joints_init,
	.destroy = ex_hinge_joints_destroy,
	.input_process = ex_hinge_joints_input_process,
	.menu_properties_update = ex_hinge_joints_menu_update,
	.mouse_change_process = ex_hinge_joints_mouse_change_process,
	.mouse_click_process = ex_hinge_joints_mouse_click_process,
	.render = ex_hinge_joints_render,
	.scroll_change_process = ex_hinge_joints_scroll_change_process,
	.update = ex_hinge_joints_update,
	.window_resize_process = ex_hinge_joints_window_resize_process
};