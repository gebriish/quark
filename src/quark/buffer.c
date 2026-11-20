
function Q_Buffer *
open_buffer_from_path(String8 path)
{
  File_Info info = file_get_info(path);
  if (info.kind != File_Kind_Regular)
  {
    return 0;
  }

  File_Id id = info.id;
  u128 packed_key = pack_file_id(id);
  u64 hash = u128_hash(packed_key);
  u64 index = hash % CArrayLength(quark_state->buffer_table);

  Q_Buffer *buffer = 0;
  for (Q_Buffer *node = quark_state->buffer_table[index].first; node; node = node->hash_next)
  {
    if (file_id_matches(node->file_info.id, id))
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
    buffer->file_info = info;
    buffer->name = file_name_from_path(buffer->path);
    buffer->original = load_entire_file(arena, path);

    DLLPushBack_N(quark_state->buffer_table[index].first, quark_state->buffer_table[index].last, buffer, hash_next, hash_prev);
    DLLPushBack(quark_state->first_buffer, quark_state->last_buffer, buffer);
    quark_state->buffer_count += 1;
  }
  else
  {
    buffer->file_info = info;
    if (str8_matches(buffer->path, path) == false)
    {
      buffer->path = str8_push_copy(buffer->arena, path);
    }

    if (buffer->file_info.modified < info.modified)
    {
      // @todo: reload the data.
    }
  }

  return buffer;
}

function void 
close_buffer(Q_Buffer *buf)
{
  if (!buf || quark_state->buffer_count <= 0) 
  {
    return;
  }

  u128 packed_key = pack_file_id(buf->file_info.id);
  u64 hash = u128_hash(packed_key);
  u64 index = hash % CArrayLength(quark_state->buffer_table);

  DLLRemove_N(quark_state->buffer_table[index].first, quark_state->buffer_table[index].last, buf, hash_next, hash_prev);
  DLLRemove(quark_state->first_buffer, quark_state->last_buffer, buf);
  quark_state->buffer_count -= 1;

  arena_release(buf->arena);
}

function void 
save_buffer(Q_Buffer *buf)
{
  // @todo(gaureesh): implement
}

function u128
pack_file_id(File_Id id)
{
  u128 packed = {
    .lo = id.v[0],
    .hi = id.v[1],
  };
  return packed;
}
