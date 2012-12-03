#ifndef ROUTER_H
#define ROUTER_H 1

#include <ev.h>
#include <common/common.h>
#include "mqtt.h"
#include "serial.h"

#define MAX_STR 256
#define MAX_ESCAPED_STR (MAX_STR * 2)

#define MAX_ID_STR MAX_STR
#define MAX_ADDR_STR MAX_STR
#define MAX_TOPIC_STR MAX_STR


struct server_context
{
    struct ev_loop *loop;

    ev_timer *timer_1hz;
    ev_timer *timer_10hz;
    struct ev_idle *idle_watcher;

    mqtt_context_t mqttctx;
    char mqtt_server[512];
    uint16_t mqtt_port;
    char mqtt_name[256];

    serial_context_t serialctx;
};
typedef struct server_context server_context_t;

const char *get_topic_prefix(void);

#endif

