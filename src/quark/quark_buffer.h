#ifndef Q_BUFFER_H
#define Q_BUFFER_H

function Q_Buffer *q_open_buffer_from_path(String8 path);
function void      q_close_buffer(Q_Buffer *buf);
function void      q_save_buffer(Q_Buffer *buf);

#endif // Q_BUFFER_H
