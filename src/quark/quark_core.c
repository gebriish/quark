#include "quark_core.h"

#include <stdlib.h>

internal void
quark_new(Quark_Context *context)
{
	Assert(context && "quark context ptr is null");

	context->allocator = heap_allocator();

	Alloc_Buffer arena_buffer = mem_alloc(&context->allocator, MB(4), DEFAULT_ALIGNMENT);

	if (arena_buffer.err != Alloc_Err_None) {
		LogError("Failed to allocate temp arena's memory buffer"); Trap();
	}

	Arena *arena = arena_new(arena_buffer);
	context->temp_allocator = arena_allocator(arena);

	buffer_manager_init(&context->allocator, &context->buffer_manager);

	context->window = quark_window_open();
	context->active_buffer = q_buffer_new(&context->allocator, &context->buffer_manager, KB(4));
}

internal void
quark_delete(Quark_Context *context)
{
	Assert(context && "quark context ptr is null");

	quark_window_deinit(context->window);

	mem_free(&context->allocator, context->temp_allocator.data, 0 /* size is irrelevant */);
}

internal String8 
gather_cmd_input(Quark_Context *context, String8 input_string, Press_Flags s_flags)
{
	Quark_Buffer *cmd_buffer = context->cmd_gap_buffer;
	if (!cmd_buffer || !cmd_buffer->data)
	{
		context->cmd_gap_buffer = q_buffer_new(&context->allocator, &context->buffer_manager, 256);
		cmd_buffer = context->cmd_gap_buffer;

		if(!cmd_buffer) {
			LogError("Failed to create Command Buffer");
			return str8(NULL, 0);
		}
	}

	if (input_string.len > 0) {
		q_buffer_insert(cmd_buffer, input_string);
	}

	if (s_flags & Press_Enter)
	{
		u8 *data = cmd_buffer->data;
		String8 command = q_buffer_to_str(cmd_buffer, &context->temp_allocator);
		cmd_buffer->gap_index = 0;
		cmd_buffer->gap_size = cmd_buffer->capacity;
		return command;
	}
	else if (s_flags & Press_Backspace) q_buffer_delete_rune(cmd_buffer, 1, Cursor_Dir_Left);
	else if (s_flags & Press_Delete)    q_buffer_delete_rune(cmd_buffer, 1, Cursor_Dir_Right);
	else if (s_flags & Press_Right)     q_buffer_move_gap_by(cmd_buffer, 1, Cursor_Dir_Right);
	else if (s_flags & Press_Left)      q_buffer_move_gap_by(cmd_buffer, 1, Cursor_Dir_Left);

	return str8(NULL, 0);
}

internal void
quark_frame_update(Quark_Context *context, Input_Data data)
{
	Press_Flags special_press = data.special_press;
	Quark_State current_state = context->state;
	rune input_char = data.codepoint;

	u8 mem[4];
	String8 input_string = {0};
	if (input_char) {
		input_string = str8_encode_rune(input_char, mem);
	}

	if (special_press & Press_Escape) {
		if (current_state != Quark_State_Normal) {
			context->state = Quark_State_Normal;
		}
		return;
	}

	switch (current_state) {
		case Quark_State_Normal: {
			if (special_press & Press_Insert) {
				context->state = Quark_State_Insert;
			}
			else if (special_press & Press_Cmd) {
				context->state = Quark_State_Cmd;
			}
		} break;

		case Quark_State_Insert: {
		} break;

		case Quark_State_Cmd: {
			String8 cmd_string = gather_cmd_input(context, input_string, special_press);
			if (cmd_string.len) {
				if (str8_equal(cmd_string, str8_lit("q")))
					quark_window_close(context->window);
				else if(str8_equal(cmd_string, str8_lit("w")))
					LogInfo("write!");
				else if(str8_equal(cmd_string, str8_lit("clear")))
					LogInfo("clear!");
				context->state = Quark_State_Normal;
			}
		} break;
	}
}
