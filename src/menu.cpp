#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "core.h"
#include "common.h"
#include <stdio.h>

#include "light_array.h"

#include "glad/glad.h"
#include <GLFW/glfw3.h>

#define GLSL_VERSION "#version 330"
#define MENU_TITLE "Examples"

void menu_char_click_process(GLFWwindow* window, u32 c) {
	ImGui_ImplGlfw_CharCallback(window, c);
}

void menu_key_click_process(GLFWwindow* window, s32 key, s32 scan_code, s32 action, s32 mods) {
	ImGui_ImplGlfw_KeyCallback(window, key, scan_code, action, mods);
}

void menu_mouse_click_process(GLFWwindow* window, s32 button, s32 action, s32 mods) {
	ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
}

void menu_scroll_change_process(GLFWwindow* window, s64 x_offset, s64 y_offset) {
	ImGui_ImplGlfw_ScrollCallback(window, x_offset, y_offset);
}

void menu_init(GLFWwindow* window) {
	// Setup Dear ImGui binding
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImGui_ImplGlfw_InitForOpenGL(window, false);
	ImGui_ImplOpenGL3_Init(GLSL_VERSION);

	// Setup style
	ImGui::StyleColorsDark();
}

static void draw_main_window() {
ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
core_menu_render();
ImGui::End();
}

void menu_render() {
	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(550, 680), ImGuiCond_FirstUseEver);

#if 1
	draw_main_window();
#else
	ImGui::ShowDemoWindow();
#endif

	// Rendering
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void menu_destroy() {
	// Cleanup
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}
