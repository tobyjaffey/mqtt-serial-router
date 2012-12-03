/*
* Copyright (c) 2012, Toby Jaffey <toby@sensemote.com>
*
* Permission to use, copy, modify, and distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
#include <common/common.h>
#include <common/logging.h>
#include <ev.h>
#include <mosquitto.h>

#include "router.h"
#include "evhelper.h"
#include "mqtt.h"
#include "cmd.h"

extern server_context_t g_srvctx;

#if LIBMOSQUITTO_VERSION_NUMBER < 1000005
#define MOSQ_MID_T uint16_t
#else
#define MOSQ_MID_T int
#endif

static void mqtt_reconnect_timeout_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    ev_timer_helper_t *watcher = (ev_timer_helper_t *)w;
    mqtt_context_t *mqttctx = (mqtt_context_t *)watcher->userdata;
   
    if (EV_TIMEOUT & revents)
    {
        if (!mqttctx->connected)
        {
            if (g_srvctx.mqtt_server[0] != 0)
            {
                LOG_INFO("Attempting MQTT reconnect %s:%d", g_srvctx.mqtt_server, g_srvctx.mqtt_port);
                mqtt_connect(mqttctx, g_srvctx.mqtt_name, g_srvctx.mqtt_server, g_srvctx.mqtt_port);
            }
        }
    }
}

static void mqtt_timeout_cb(struct ev_loop *loop, struct ev_io *w, int revents)
{
    ev_timer_helper_t *watcher = (ev_timer_helper_t *)w;
    mqtt_context_t *mqttctx = (mqtt_context_t *)watcher->userdata;
   
    if (EV_TIMEOUT & revents)
    {
        if (mqttctx->connected)
        {
            if (MOSQ_ERR_SUCCESS != mosquitto_loop_misc(mqttctx->mosq))
            {
                LOG_ERROR("mosquitto_loop_misc fail");
                mqtt_disconnect(mqttctx);
            }

#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
            if (mosquitto_want_write(mqttctx->mosq))
#endif
                ev_io_start(g_srvctx.loop, &mqttctx->write_watcher.io);
        }
    }
}

static void mqtt_ev_cb(struct ev_loop *loop, ev_io *w, int revents)
{
    ev_io_helper_t *watcher = (ev_io_helper_t *)w;
    mqtt_context_t *mqttctx = (mqtt_context_t *)watcher->userdata;

    if (EV_READ & revents)
    {
        if (MOSQ_ERR_SUCCESS != mosquitto_loop_read(mqttctx->mosq
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
        , 1
#endif
        ))
        {
            LOG_ERROR("read fail");
            mqtt_disconnect(mqttctx);
        }
    }

    if (EV_WRITE & revents)
    {
        if (MOSQ_ERR_SUCCESS != mosquitto_loop_write(mqttctx->mosq
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
        , 1
#endif
        ))
        {
            LOG_ERROR("write fail");
            mqtt_disconnect(mqttctx);
        }

        if (!mosquitto_want_write(mqttctx->mosq))
            ev_io_stop(g_srvctx.loop, &mqttctx->write_watcher.io);
    }
}

static void mosq_connect_cb(
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
struct mosquitto *mosq,
#endif
void *userdata, int rc)
{
    mqtt_context_t *mqttctx = (mqtt_context_t *)userdata;

    if (rc != 0)
        LOG_ERROR("mqtt connect failure %d", rc);
    else
    {
        LOG_INFO("Connected");
        mqttctx->connected = true;
    }
}

static void mosq_message_cb(
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
struct mosquitto *mosq,
#endif
void *userdata, const struct mosquitto_message *msg)
{
    if (msg->payloadlen)
    {
        LOG_DEBUG("mqtt rx %s %s", msg->topic, msg->payload);
        cmd_message_cb((const char *)msg->topic, (const char *)msg->payload);
    }
    else
    {
        LOG_DEBUG("mqtt rx %s (null)", msg->topic);
        cmd_message_cb((const char *)msg->topic, NULL);
    }
}

static void mosq_unsubscribe_cb(
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
struct mosquitto *mosq,
#endif
void *userdata, MOSQ_MID_T mid)
{
    LOG_DEBUG("unsub OK mid=%d", mid);
    cmd_unsubscribe_cb(mid);
}

static void mosq_subscribe_cb(
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
struct mosquitto *mosq,
void *userdata, MOSQ_MID_T mid, int qos_count, const int *granted_qos)
#else
void *userdata, MOSQ_MID_T mid, int qos_count, const uint8_t *granted_qos)
#endif
{
    LOG_DEBUG("sub OK mid=%d", mid);
    cmd_subscribe_cb(mid);
}

static void mosq_publish_cb(
#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
struct mosquitto *mosq,
#endif
void *userdata, MOSQ_MID_T mid)
{
    LOG_DEBUG("pub OK mid=%d", mid);
    cmd_publish_cb(mid);
}


int mqtt_init(struct ev_loop *loop, mqtt_context_t *mqttctx)
{
    if (MOSQ_ERR_SUCCESS != mosquitto_lib_init())
        return -1;

    mqttctx->connected = false;

    mqttctx->reconnect_timer.userdata = (void *)mqttctx;
    ev_timer_init(&mqttctx->reconnect_timer.timer, (void *)mqtt_reconnect_timeout_cb, 0, 1);
    ev_timer_start(g_srvctx.loop, &mqttctx->reconnect_timer.timer);

    return 0;
}

int mqtt_connect(mqtt_context_t *mqttctx, const char *name, const char *host, uint16_t port)    // FIXME, blocks
{
    if (NULL != mqttctx->mosq)
        mosquitto_destroy(mqttctx->mosq);


#if LIBMOSQUITTO_VERSION_NUMBER >= 1000005
    if (NULL == (mqttctx->mosq = mosquitto_new(name, true, (void *)mqttctx)))
#else
    if (NULL == (mqttctx->mosq = mosquitto_new(name, (void *)mqttctx)))
#endif
        return 1;

    mosquitto_connect_callback_set(mqttctx->mosq, mosq_connect_cb);
    mosquitto_message_callback_set(mqttctx->mosq, mosq_message_cb);
    mosquitto_subscribe_callback_set(mqttctx->mosq, mosq_subscribe_cb);
    mosquitto_unsubscribe_callback_set(mqttctx->mosq, mosq_unsubscribe_cb);
    mosquitto_publish_callback_set(mqttctx->mosq, mosq_publish_cb);

    if (0 != strcmp(host, g_srvctx.mqtt_name))
        strncpy(g_srvctx.mqtt_name, name, sizeof(g_srvctx.mqtt_name));
    if (0 != strcmp(host, g_srvctx.mqtt_server))
        strncpy(g_srvctx.mqtt_server, host, sizeof(g_srvctx.mqtt_server));
    g_srvctx.mqtt_port = port;

#if LIBMOSQUITTO_VERSION_NUMBER < 1000005
    if (0 != mosquitto_connect(mqttctx->mosq, g_srvctx.mqtt_server, g_srvctx.mqtt_port, 5, true)) // FIXME, this should be async, with below in cb
#else
    if (0 != mosquitto_connect(mqttctx->mosq, g_srvctx.mqtt_server, g_srvctx.mqtt_port, 5)) // FIXME, this should be async, with below in cb
#endif
    {
        LOG_INFO("mqtt connect failed");
        return 1;
    }

    mqttctx->read_watcher.userdata = (void *)mqttctx;
    ev_io_init(&mqttctx->read_watcher.io, mqtt_ev_cb, mosquitto_socket(mqttctx->mosq), EV_READ);
    ev_io_start(g_srvctx.loop, &mqttctx->read_watcher.io);

    mqttctx->write_watcher.userdata = (void *)mqttctx;
    ev_io_init(&mqttctx->write_watcher.io, mqtt_ev_cb, mosquitto_socket(mqttctx->mosq), EV_WRITE);

    mqttctx->timer.userdata = (void *)mqttctx;
    ev_timer_init(&mqttctx->timer.timer, (void *)mqtt_timeout_cb, 0, 0.1);
    ev_timer_start(g_srvctx.loop, &mqttctx->timer.timer);

    return 0;
}

int mqtt_disconnect(mqtt_context_t *mqttctx)
{
    mosquitto_disconnect(mqttctx->mosq);
    ev_io_stop(g_srvctx.loop, &mqttctx->write_watcher.io);
    ev_io_stop(g_srvctx.loop, &mqttctx->read_watcher.io);
    ev_timer_stop(g_srvctx.loop, &mqttctx->timer.timer);
    mqttctx->connected = false;

    LOG_INFO("mqtt_disconnect");
    //webapi_mqtt_failall_cb();    // fail any connections waiting on a pub response

    return 0;
}


int mqtt_publish(mqtt_context_t *mqttctx, const char *topic, const char *msg, int qos, int *mid)
{
    int rc;
    MOSQ_MID_T m;

    rc = mosquitto_publish(mqttctx->mosq, &m, topic, strlen(msg), (const uint8_t *)msg, qos, true);
    LOG_DEBUG("mqtt_publish topic=%s msg=%s rc=%d", topic, msg, rc);

    if (NULL != mid)
        *mid = m;

    if (mosquitto_want_write(mqttctx->mosq))
        ev_io_start(g_srvctx.loop, &mqttctx->write_watcher.io);

    return rc;
}

int mqtt_subscribe(mqtt_context_t *mqttctx, const char *topic, int qos, int *mid)
{
    int rc;
    MOSQ_MID_T m;

    rc = mosquitto_subscribe(mqttctx->mosq, &m, topic, qos);
    LOG_DEBUG("mqtt_subscribe topic=%s rc=%d", topic, rc);

    if (NULL != mid)
        *mid = m;

    if (mosquitto_want_write(mqttctx->mosq))
        ev_io_start(g_srvctx.loop, &mqttctx->write_watcher.io);

    return rc;
}

int mqtt_unsubscribe(mqtt_context_t *mqttctx, const char *topic, int *mid)
{
    int rc;
    MOSQ_MID_T m;

    rc = mosquitto_unsubscribe(mqttctx->mosq, &m, topic);
    LOG_DEBUG("mqtt_unsubscribe topic=%s rc=%d", topic, rc);

    if (NULL != mid)
        *mid = m;

    if (mosquitto_want_write(mqttctx->mosq))
        ev_io_start(g_srvctx.loop, &mqttctx->write_watcher.io);

    return rc;
}

