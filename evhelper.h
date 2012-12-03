#ifndef EV_HELPER_H
#define EV_HELPER_H

#include <ev.h>

struct ev_io_helper
{
    ev_io io;
    void *userdata;
};
typedef struct ev_io_helper ev_io_helper_t;

struct ev_timer_helper
{
    ev_timer timer;
    void *userdata;
};
typedef struct ev_timer_helper ev_timer_helper_t;

#endif

