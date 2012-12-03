#ifndef SERIAL_H
#define SERIAL_H 1

#include <stdint.h>
#include <ev.h>

typedef struct
{
    struct ev_loop *loop;
    struct ev_io *w_console;
    int fd;
} serial_context_t;

int serial_init(struct ev_loop *loop, serial_context_t *serialctx, const char *device);
int serial_write(serial_context_t *serialctx, const char *buf, size_t len);
int serial_write_str(serial_context_t *serialctx, const char *str, bool trailing_space);
int serial_write_str_escape(serial_context_t *serialctx, const char *str, char *escbuf, bool trailing_space);

#endif

