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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdbool.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/select.h>
#include <termios.h>
#include <sys/select.h>
#include <common/logging.h>

#include "router.h"
#include "mqtt.h"
#include "serial.h"
#include "escape.h"

#define CLIENT_NAME "smr"

server_context_t g_srvctx;

static char *device_name = NULL;
static char *server = NULL;
static int port = -1;
static bool verbose = false;
static char *topic_prefix = NULL;

static struct option long_options[] =
{
    {"help",    no_argument, 0, 'h'},
    {"device",     required_argument, 0, 'd'},
    {"server",     required_argument, 0, 's'},
    {"port",       required_argument, 0, 'p'},
    {"verbose",    no_argument, 0, 'v'},
    {"topic",      required_argument, 0, 't'},
    {0, 0, 0, 0}
};

const char *get_topic_prefix(void)
{
    return topic_prefix;
}

void timeout_1hz_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
}

void timeout_10hz_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
}

void idle_cb(struct ev_loop *loop, struct ev_idle *w, int revents)
{
}

void usage(void)
{
    fprintf(stderr, "MQTT router\n");
    fprintf(stderr, "router -v -d - -s test.mosquitto.org -p 1883 -t prefix/\n");
    fprintf(stderr, "  --verbose                    -v     Chatty mode\n");
    fprintf(stderr, "  --device=/dev/ttyAMA0        -d     Serial device\n");
    fprintf(stderr, "  --server=test.mosquitto.org  -s     MQTT broker host\n");
    fprintf(stderr, "  --port=1883                  -p     MQTT broker port\n");
    fprintf(stderr, "  --topic=topic/tree/          -t     MQTT topic prefix\n");
}


int parse_options(int argc, char **argv)
{
    int c;
    int option_index;
    size_t len;

    while(1)
    {
        c = getopt_long (argc, argv, "t:vhd:s:p:", long_options, &option_index);
        if (c == -1)
            break;
        switch(c)
        {
            case 'h':
                return 1;
            break;
            case 'd':
                device_name = strdup(optarg);
            break;
            case 't':
                topic_prefix = strdup(optarg);
                len = strlen(topic_prefix);
                if (len == 0 || topic_prefix[len-1] != '/')
                { 
                    fprintf(stderr, "prefix must end in /");
                    return 1;
                }
            break;
            case 's':
                server = strdup(optarg);
            break;
            case 'p':
                port = atoi(optarg);
            break;
            case 'v':
                verbose = true;
            break;
            default:
                return 1;
            break;
        }
    }

    if (NULL == device_name || NULL == server || -1 == port || NULL == topic_prefix)
        return 1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (0 != parse_options(argc, argv))
    {
        usage();
        return 1;
    }

    if (!verbose)
        log_setlevel(LOGLEVEL_CRITICAL);

    memset(&g_srvctx, 0, sizeof(g_srvctx));
    g_srvctx.loop = ev_default_loop(0);
    ev_set_userdata(g_srvctx.loop, &g_srvctx);

    if (NULL == (g_srvctx.timer_1hz = (struct ev_timer *)malloc(sizeof(struct ev_timer))))
        LOG_CRITICAL("out of mem");
    if (NULL == (g_srvctx.timer_10hz = (struct ev_timer *)malloc(sizeof(struct ev_timer))))
        LOG_CRITICAL("out of mem");
    if (NULL == (g_srvctx.idle_watcher = (struct ev_idle *)malloc(sizeof(struct ev_idle))))
        LOG_CRITICAL("out of mem");

    ev_timer_init(g_srvctx.timer_1hz, (void *)timeout_1hz_cb, 0, 1);
    ev_set_priority(g_srvctx.timer_1hz, EV_MAXPRI);
    ev_timer_start(g_srvctx.loop, g_srvctx.timer_1hz);
    ev_timer_init(g_srvctx.timer_10hz, (void *)timeout_10hz_cb, 0, 0.1);
    ev_set_priority(g_srvctx.timer_10hz, EV_MAXPRI);
    ev_timer_start(g_srvctx.loop, g_srvctx.timer_10hz);
    ev_idle_init(g_srvctx.idle_watcher, idle_cb);

    if (0 != serial_init(g_srvctx.loop, &(g_srvctx.serialctx), device_name))
    {
        LOG_CRITICAL("Failed to open %s", device_name);
    }

    if (0 != mqtt_connect(&(g_srvctx.mqttctx), CLIENT_NAME, server, port))
    {
        LOG_CRITICAL("failed to connect to server");
    }

    while(1)
    {
        ev_loop(g_srvctx.loop, 0);
    }

    return 0;
}


