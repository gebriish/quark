#ifndef Q_BUFFER_H
#define Q_BUFFER_H

function Q_Buffer *open_buffer_from_path(String8 path);
function void close_buffer(Q_Buffer *buf);
function void save_buffer(Q_Buffer *buf);

//////////////////////////
// @section: internal helpers

function u128 pack_file_id(File_Id id);

#endif // Q_BUFFER_H
