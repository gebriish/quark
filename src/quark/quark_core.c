#include "quark_core.h"


internal void
quark_new(Quark_Context *context)
{
	Assert(context && "quark context ptr is null");

	context->persist_arena   = arena_alloc(MB(1));
	context->transient_arena = arena_alloc(MB(1));

	buffer_manager_init(context->persist_arena, &context->buffer_manager);
}

internal void
quark_delete(Quark_Context *context)
{
	Assert(context && "quark context ptr is null");

	arena_release(context->persist_arena);
	arena_release(context->transient_arena);
}

internal String8 
gather_cmd_input(Quark_Context *context, String8 input_string, Special_Press_Flags s_flags)
{
	Gap_Buffer *cmd_buffer = context->cmd_gap_buffer;
	if (!cmd_buffer || !cmd_buffer->data)
	{
		context->cmd_gap_buffer = gap_buffer_new(&context->buffer_manager, KB(4));
		cmd_buffer = context->cmd_gap_buffer;

		if(!cmd_buffer) {
			LogError("Failed to create Command Buffer");
			return str8(NULL, 0);
		}
	}

	if (input_string.len > 0) {
		gap_buffer_insert(cmd_buffer, input_string);
	}

	if (s_flags & Special_Press_Enter)
	{
		u8 *data = cmd_buffer->data;
		String8 command = gap_buffer_to_str(cmd_buffer, context->transient_arena);
		cmd_buffer->gap_index = 0;
		cmd_buffer->gap_size = cmd_buffer->capacity;
		return command;
	}
	else if (s_flags & Special_Press_Backspace) gap_buffer_delete_rune(cmd_buffer, 1, Cursor_Dir_Left);
	else if (s_flags & Special_Press_Delete)    gap_buffer_delete_rune(cmd_buffer, 1, Cursor_Dir_Right);
	else if (s_flags & Special_Press_Right)     gap_buffer_move_gap_by(cmd_buffer, 1, Cursor_Dir_Right);
	else if (s_flags & Special_Press_Left)      gap_buffer_move_gap_by(cmd_buffer, 1, Cursor_Dir_Left);

	return str8(NULL, 0);
}

internal void
quark_frame_update(Quark_Context *context, Input_Data data)
{
	Special_Press_Flags special_press = data.special_press;
	Quark_State current_state = context->state;
	rune input_char = data.codepoint;

	u8 mem[4];
	String8 input_string = {0};
	if (input_char) {
		input_string = str8_encode_rune(input_char, mem);
	}

	if (special_press & Special_Press_Escape) {
		if (current_state != Quark_State_Normal) {
			context->state = Quark_State_Normal;
		}
		return;
	}

	switch (current_state) {
		case Quark_State_Normal: {
			if (special_press & Special_Press_Insert) {
				context->state = Quark_State_Insert;
			}
			else if (special_press & Special_Press_Cmd) {
				context->state = Quark_State_Cmd;
			}
		} break;

		case Quark_State_Insert: {
		} break;

		case Quark_State_Cmd: {
			String8 cmd_string = gather_cmd_input(context, input_string, special_press);
			if (cmd_string.len) {
				LogInfo(STR_FMT, str8_fmt(cmd_string));
				context->state = Quark_State_Normal;
			}
		} break;
	}
}
