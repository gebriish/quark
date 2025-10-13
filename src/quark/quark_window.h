#ifndef QUARK_WINDOW_H
#define QUARK_WINDOW_H

#include "../base/base_core.h"
#include "../base/base_string.h"

#include <GLFW/glfw3.h>

typedef GLFWwindow* Quark_Window;

typedef struct Input_Data Input_Data;
struct Input_Data {
	u32      codepoint;
	String8  clipboard_str;
	vec2_f32 mouse_pointer;
};

internal Quark_Window quark_window_open();
internal void quark_window_close(Quark_Window window);

internal bool quark_window_is_open(Quark_Window window);
internal void quark_window_swap_buff(Quark_Window window);


internal Input_Data quark_gather_input(Quark_Window window);

#define quark_get_ogl_proc_addr glfwGetProcAddress


#endif
