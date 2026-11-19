
function Q_Buffer *
q_open_buffer_from_path(String8 path)
{
  FileProperty props = file_property(path);
  if (props.kind != FileKind_Regular)
  {
    return 0;
  }

  FileId id = props.id;

  U128 packed_key =
  {
    .lo = id.u64[0],
    .hi = id.u64[1],
  };

  U64 hash = u128_hash(packed_key);
  U64 index = hash % CArrayLength(quark_state->buffer_table);

  Q_Buffer *buffer = 0;
  for (Q_Buffer *node = quark_state->buffer_table[index].first; node; node = node->hash_next)
  {
    if (file_id_matches(node->props.id, id))
    {
      buffer = node;
      break;
    }
  }

  if (!buffer)
  {
    Arena *arena  = arena_reserve(Gigabytes(4));
    buffer = arena_push(arena, Q_Buffer, 1);
    buffer->arena = arena;
    buffer->path  = str8_push_copy(buffer->arena, path);
    buffer->props = props;

#if OS_WINDOW
    S32 file_sep = '\\';
#else 
    S64 file_sep = '/';
#endif

    S64 slash_index = str8_find_last(buffer->path, file_sep);
    buffer->name = str8_skip(buffer->path, slash_index+1);

    buffer->original = load_entire_file(arena, path);

    DLLPushBack_N(quark_state->buffer_table[index].first, quark_state->buffer_table[index].last, buffer, hash_next, hash_prev);
    DLLPushBack(quark_state->first_buffer, quark_state->last_buffer, buffer);
    quark_state->buffer_count += 1;
  }
  else
  {
    buffer->props = props;
    if (str8_matches(buffer->path, path) == false)
    {
      buffer->path = str8_push_copy(buffer->arena, path);
    }

    if (buffer->props.modified < props.modified)
    {
      // @todo: reload the data.
    }
  }

  return buffer;
}

function void 
q_close_buffer(Q_Buffer *buf)
{
  if (!buf || quark_state->buffer_count <= 0) 
  {
    return;
  }

  U128 packed_key = 
  {
    .lo = buf->props.id.u64[0],
    .hi = buf->props.id.u64[1],
  };

  U64 hash = u128_hash(packed_key);
  U64 index = hash % CArrayLength(quark_state->buffer_table);

  DLLRemove_N(quark_state->buffer_table[index].first, quark_state->buffer_table[index].last, buf, hash_next, hash_prev);
  DLLRemove(quark_state->first_buffer, quark_state->last_buffer, buf);
  quark_state->buffer_count -= 1;

  arena_release(buf->arena);
}

function void 
q_save_buffer(Q_Buffer *buf)
{
  // @todo(gaureesh): implement
}

function void 
q_push_command(Q_CommandHook hook)
{
  Q_Command *node = quark_state->free_command_nodes;
  if (quark_state->free_command_nodes)
  {
    SLLStackPop(quark_state->free_command_nodes);
    MemZeroStruct(node);
  }
  else
  {
    node = arena_push(quark_state->arena, Q_Command, 1);
  }

  node->hook = hook;
  SLLQueuePush(quark_state->first_command, quark_state->last_command, node);
}

