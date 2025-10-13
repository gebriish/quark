#include "../gfx/gfx_core.h"
#include "quark_window.h"

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

	return window;
}

internal void 
quark_window_close(Quark_Window window) 
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

internal Input_Data 
quark_gather_input(Quark_Window window) 
{
	g_input_data = (Input_Data){0};

	glfwPollEvents();

	GLFWwindow *glfw_handle = (GLFWwindow *)window;

	{
		f64 cx, cy;
		glfwGetCursorPos(glfw_handle, &cx, &cy);
		g_input_data.mouse_pointer = (vec2_f32){ (f32) cx, (f32) cy };
	}


	return g_input_data;
}
