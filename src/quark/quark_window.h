#ifndef QUARK_WINDOW_H
#define QUARK_WINDOW_H

#include "../base/base_core.h"
#include "../base/base_string.h"

#include <GLFW/glfw3.h>

typedef GLFWwindow* Quark_Window;

typedef u32 Special_Press_Flags;
enum {
	Special_Press_Return     = (Special_Press_Flags)(1 << 0),
	Special_Press_Backspace  = (Special_Press_Flags)(1 << 1),
	Special_Press_Tab        = (Special_Press_Flags)(1 << 2),
	Special_Press_L          = (Special_Press_Flags)(1 << 3),
	Special_Press_R          = (Special_Press_Flags)(1 << 4),
	Special_Press_D          = (Special_Press_Flags)(1 << 5),
	Special_Press_U          = (Special_Press_Flags)(1 << 6),
	Special_Copy             = (Special_Press_Flags)(1 << 7),
	Special_Press_Delete     = (Special_Press_Flags)(1 << 8),
};

typedef struct Input_Data Input_Data;
struct Input_Data {
	u32      codepoint;
	String8  clipboard_str;
	vec2_f32 mouse_pointer;
	Special_Press_Flags special_press;
};

internal Quark_Window quark_window_open();
internal void quark_window_close(Quark_Window window);

internal bool quark_window_is_open(Quark_Window window);
internal void quark_window_swap_buff(Quark_Window window);


internal Input_Data quark_gather_input(Quark_Window window);

#define quark_get_ogl_proc_addr glfwGetProcAddress


#endif
