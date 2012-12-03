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

#include <common/logging.h>
#include <common/common.h>

#include "serial.h"
#include "line.h"
#include "mqtt.h"
#include "router.h"
#include "midpublist.h"
#include "midsublist.h"
#include "midunsublist.h"
#include "addrsublist.h"
#include "escape.h"

extern server_context_t g_srvctx;

struct topic_msg
{
    const char *topic;
    const char *msg;
};

void addrsub_iterator(const char *addr_str, const char *topic_str, void *userdata)
{
    char escbuf[MAX_ESCAPED_STR];
    struct topic_msg *tm = (struct topic_msg *)userdata;

    if (0==strcmp(topic_str, tm->topic))
    {
        LOG_DEBUG("Dispatch %s:%s -> %s", tm->topic, tm->msg, addr_str);
        if (0 != serial_write_str(&g_srvctx.serialctx, "INF", true))
            goto fail;
        if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
            goto fail;
        if (0 != serial_write_str_escape(&g_srvctx.serialctx, tm->topic, escbuf, true))
            goto fail;
        if (0 != serial_write_str_escape(&g_srvctx.serialctx, tm->msg, escbuf, false))
            goto fail;
    }
    return;
fail:
    LOG_CRITICAL("write failed");
}

void cmd_message_cb(const char *topic, const char *msg)
{
    struct topic_msg tm;

    if (0!=strncmp(get_topic_prefix(), topic, strlen(get_topic_prefix())))
    {
        LOG_ERROR("Unwarranted MQTT msg %s:%s", topic, msg);
        return;
    }
    topic += strlen(get_topic_prefix());

    LOG_INFO("MQTT RX: %s = %s", topic, msg);

    tm.topic = topic;
    tm.msg = msg;
    addrsub_foreach(addrsub_iterator, &tm);
}

void cmd_subscribe_cb(int mid)
{
    char *id_str;
    char *addr_str;
    char *topic_str;
    char escbuf[MAX_ESCAPED_STR];

    if (0 != midsub_find(mid, &id_str, &addr_str, &topic_str))
    {
        LOG_ERROR("Unwarranted mid in MQTT sub rsp %d", mid);
        return;
    }

    LOG_INFO("MQTT SUBACK %s (%s)", topic_str, addr_str);

    if (0 != serial_write_str(&g_srvctx.serialctx, "SUBACK", true))
        goto fail;
    if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
        goto fail;
    if (0 != serial_write_str_escape(&g_srvctx.serialctx, id_str, escbuf, false))
        goto fail;

    addrsub_insert(addr_str, topic_str);
    midsub_remove(mid);
    return;
fail:
    LOG_CRITICAL("write failed");
}

void cmd_unsubscribe_cb(int mid)
{
    char escbuf[MAX_ESCAPED_STR];
    char *id_str;
    char *addr_str;
    char *topic_str;

    if (0 != midunsub_find(mid, &id_str, &addr_str, &topic_str))
    {
        LOG_ERROR("Unwarranted mid in MQTT sub rsp %d", mid);
        return;
    }

    LOG_INFO("MQTT UNSUBACK %s (%s)", topic_str, addr_str);

    if (0 != serial_write_str(&g_srvctx.serialctx, "UNSUBACK", true))
        goto fail;
    if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
        goto fail;
    if (0 != serial_write_str_escape(&g_srvctx.serialctx, id_str, escbuf, false))
        goto fail;

    addrsub_remove(addr_str, topic_str);
    midunsub_remove(mid);
    return;
fail:
    LOG_CRITICAL("write failed");
}


void cmd_publish_cb(int mid)
{
    char escbuf[MAX_ESCAPED_STR];
    char *id_str;
    char *addr_str;

    LOG_INFO("MQTT PUBACK");

    if (0 != midpub_find(mid, &id_str, &addr_str))
    {
        LOG_ERROR("Unwarranted mid in MQTT pub rsp %d", mid);
        return;
    }

    if (0 != serial_write_str(&g_srvctx.serialctx, "PUBACK", true))
        goto fail;
    if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
        goto fail;
    if (0 != serial_write_str_escape(&g_srvctx.serialctx, id_str, escbuf, false))
        goto fail;
    midpub_remove(mid);
    return;
fail:
    LOG_CRITICAL("write failed");
}

static void handle_command(int argc, char **argv)
{
    char escbuf[MAX_ESCAPED_STR];
    char mqtt_topic[4096];

    if (argc >= 5 && 0 == strcmp(argv[0], "PUB"))
    {
        int mid;
        char *id_str;
        char *addr_str;

        id_str = argv[4];
        addr_str = argv[1];

        snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", get_topic_prefix(), argv[2]);
        LOG_INFO("mqtt publish %s = %s", mqtt_topic, argv[3]);
        if (0 != mqtt_publish(&(g_srvctx.mqttctx), mqtt_topic, argv[3], 1, &mid))
        {
            LOG_ERROR("MQTT publish failed");
        }
        else
        {
            midpub_insert(mid, id_str, addr_str);           
        }
    }
    else
    if (argc >= 3 && 0 == strcmp(argv[0], "SUB"))
    {
        int mid;
        char *addr_str;
        char *topic_str;
        char *id_str;
        int count;

        id_str = argv[3];
        addr_str = argv[1];
        topic_str = argv[2];

        count = addrsub_count_topic(topic_str);

        if (count == 0)
        {
            snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", get_topic_prefix(), topic_str);
            if (0 != mqtt_subscribe(&(g_srvctx.mqttctx), mqtt_topic, 1, &mid))
            {
                LOG_ERROR("MQTT subscribe failed");
            }
            else
            {
                midsub_insert(mid, id_str, addr_str, topic_str);           
            }
        }
        else
        {
            if (0 != serial_write_str(&g_srvctx.serialctx, "UNSUBACK", true))
                goto fail;
            if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
                goto fail;
            if (0 != serial_write_str_escape(&g_srvctx.serialctx, id_str, escbuf, false))
                goto fail;
            addrsub_insert(addr_str, topic_str);
        }
    }
    else
    if (argc >= 4 && 0 == strcmp(argv[0], "UNSUB"))
    {
        int mid;
        char *id_str;
        char *addr_str;
        char *topic_str;
        int count;

        id_str = argv[3];
        addr_str = argv[1];
        topic_str = argv[2];

        count = addrsub_count_topic(topic_str);

        if (count == 0)
        {
            if (0 != serial_write_str(&g_srvctx.serialctx, "UNSUBACK", true))
                goto fail;
            if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
                goto fail;
            if (0 != serial_write_str_escape(&g_srvctx.serialctx, id_str, escbuf, false))
                goto fail;
        }
        else
        if (count > 1)
        {
            addrsub_remove(addr_str, topic_str);
            if (0 != serial_write_str(&g_srvctx.serialctx, "UNSUBACK", true))
                goto fail;
            if (0 != serial_write_str_escape(&g_srvctx.serialctx, addr_str, escbuf, true))
                goto fail;
            if (0 != serial_write_str_escape(&g_srvctx.serialctx, id_str, escbuf, false))
                goto fail;
        }
        else
        if (count == 1)
        {
            snprintf(mqtt_topic, sizeof(mqtt_topic), "%s%s", get_topic_prefix(), topic_str);
            if (0 != mqtt_unsubscribe(&(g_srvctx.mqttctx), mqtt_topic, &mid))
            {
                LOG_ERROR("MQTT subscribe failed");
            }
            else
            {
                midunsub_insert(mid, id_str, addr_str, topic_str);           
            }
        }
    }
    return;
fail:
    LOG_CRITICAL("write failed");
}

int cmd_process(const char *orig_line)
{
#define MAXARGS 16
    int argc = 0;
    char *argv[MAXARGS];
    char c;
    char linebuf[4096];
    char *line = linebuf;
    size_t len;
    bool in_escape = 0;
    int i;
    char *unescaped_argv[MAXARGS];
    char unescaped_args[MAXARGS][MAX_STR];

    LOG_INFO("rx: %s", orig_line);

    // editable copy
    len = strlen(orig_line) + 1;
    if (len > sizeof(linebuf))
        return 1;
    memcpy(linebuf, orig_line, len+1);

    // split on space, but not if unescaped
    argv[argc++] = line;
    while((argc < MAXARGS) && (c = *line) != 0)
    {
        if ('\\' == c)
            in_escape = true;
        else
        if (!in_escape && ' ' == c)   // separator
        {
            *(line) = 0;
            argv[argc++] = line+1;
        }
        else
        {
            in_escape = false;
        }
        line++;
    }

    for (i=0;i<argc;i++)
    {
        str_unescape(argv[i], unescaped_args[i]);
        unescaped_argv[i] = unescaped_args[i];
    }

    handle_command(argc, unescaped_argv);
    return 0;
}

