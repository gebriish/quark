#ifndef QUARK_WINDOW_H
#define QUARK_WINDOW_H

#include "../base/base_core.h"
#include "../base/base_string.h"

#include <GLFW/glfw3.h>

typedef GLFWwindow* Quark_Window;

typedef u32 Special_Press_Flags;
enum {
	Special_Press_None   = 0,
	Special_Press_Cmd    = 1 << 1,
	Special_Press_Insert = 1 << 2,
	Special_Press_Escape = 1 << 3,

	Special_Press_Up     = 1 << 4,
	Special_Press_Down   = 1 << 5,
	Special_Press_Left   = 1 << 6,
	Special_Press_Right  = 1 << 7,

	Special_Press_Enter  = 1 << 8,

	Special_Press_Backspace  = 1 << 9,
	Special_Press_Delete     = 1 << 10,
	Special_Press_Tab        = 1 << 11,
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
