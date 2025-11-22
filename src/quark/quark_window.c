#include "../gfx/gfx_core.h"
#include "quark_window.h"
#include "quark_core.h"

global Input_Data g_input_data = {0};

internal void 
resize_callback(GLFWwindow* window, int w, int h) 
{
	gfx_resize_target(w, h);
}

internal void 
char_callback(GLFWwindow* window, unsigned int codepoint) 
{
	g_input_data.codepoint = codepoint;

	switch(codepoint) {
		case ':':
			g_input_data.special_press |= Press_Cmd;
		break;
		case 'i':
			g_input_data.special_press |= Press_Insert;
		break;
	}

}

internal void
key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{

	Press_Flags normal_mask =  (g_input_data.curr_state == Quark_State_Normal) ? ~0 : 0;

	if (action != GLFW_RELEASE)	{
		switch(key)
		{
			case GLFW_KEY_ESCAPE:
				g_input_data.special_press |= Press_Escape;
			break;
			case GLFW_KEY_LEFT:
				g_input_data.special_press |= Press_Left;
			break;
			case GLFW_KEY_RIGHT:
				g_input_data.special_press |= Press_Right;
			break;
			case GLFW_KEY_UP:
				g_input_data.special_press |= Press_Up;
			break;
			case GLFW_KEY_DOWN:
				g_input_data.special_press |= Press_Down;
			break;
			case GLFW_KEY_ENTER:
				g_input_data.special_press |= Press_Enter;
			break;
			case GLFW_KEY_BACKSPACE:
				g_input_data.special_press |= Press_Backspace;
			break;
			case GLFW_KEY_DELETE:
				g_input_data.special_press |= Press_Delete;
			break;
			case GLFW_KEY_TAB:
				g_input_data.special_press |= Press_Tab;
			break;
			case GLFW_KEY_H:
				g_input_data.special_press |= Press_Left & normal_mask;
			break;
			case GLFW_KEY_J: 
				g_input_data.special_press |= Press_Down & normal_mask;
			break;
			case GLFW_KEY_K: 
				g_input_data.special_press |= Press_Up & normal_mask;
			break;
			case GLFW_KEY_L:
				g_input_data.special_press |= Press_Right & normal_mask;
			break;
		}
	}
}

internal Quark_Window 
quark_window_open() 
{
	if (!glfwInit()) {
		LogError("Failed to initialize GLFW\n");
		return NULL;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(1000, 625, "quark", NULL, NULL);
	if (!window) {
		LogError("Failed to create GLFW window\n");
		glfwTerminate();
		return NULL;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(1);

	glfwSetFramebufferSizeCallback(window, resize_callback);
	glfwSetCharCallback(window, char_callback);
	glfwSetKeyCallback(window, key_callback);

	return window;
}

internal void 
quark_window_close(Quark_Window window) 
{
	glfwSetWindowShouldClose(window, GLFW_TRUE);
}


internal void
quark_window_deinit(Quark_Window window)
{
	glfwDestroyWindow(window);
	glfwTerminate();
}

internal bool 
quark_window_is_open(Quark_Window window) 
{
	return !glfwWindowShouldClose(window);
}

internal void 
quark_window_swap_buff(Quark_Window window) 
{
	glfwSwapBuffers(window);
}

internal vec2_i32
quark_window_size(Quark_Window window)
{
	int x, y;
	glfwGetWindowSize(window, &x, &y);
	return (vec2_i32) {x , y};
}

internal Input_Data 
quark_gather_input(Quark_Window window, Quark_State state) 
{
	g_input_data = (Input_Data){0};
	g_input_data.curr_state = state;

	glfwPollEvents();

	GLFWwindow *glfw_handle = (GLFWwindow *)window;

	{
		f64 cx, cy;
		glfwGetCursorPos(glfw_handle, &cx, &cy);
		g_input_data.mouse_pointer = (vec2_f32){ (f32) cx, (f32) cy };
	}

	return g_input_data;
}
