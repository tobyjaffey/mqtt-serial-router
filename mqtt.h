#ifndef MQTT_H
#define MQTT_H 1

#include "evhelper.h"

struct mqtt_context
{
    struct mosquitto *mosq;
    ev_io_helper_t read_watcher;
    ev_io_helper_t write_watcher;
    ev_timer_helper_t timer;
    ev_timer_helper_t reconnect_timer;
    bool connected;
};
typedef struct mqtt_context mqtt_context_t;

extern int mqtt_init(struct ev_loop *loop, mqtt_context_t *mqctx);
extern int mqtt_connect(mqtt_context_t *mqctx, const char *name, const char *host, uint16_t port);
extern int mqtt_disconnect(mqtt_context_t *mqctx);
extern int mqtt_publish(mqtt_context_t *mqctx, const char *topic, const char *msg, int qos, int *mid);
extern int mqtt_subscribe(mqtt_context_t *mqttctx, const char *topic, int qos, int *mid);
extern int mqtt_unsubscribe(mqtt_context_t *mqttctx, const char *topic, int *mid);

#endif

