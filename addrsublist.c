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
#include "tree.h"
#include "addrsublist.h"

#define MAX_ADDR_STR 256
#define MAX_TOPIC_STR 256
#define MAX_ADDR_TOPIC_STR (MAX_ADDR_STR+MAX_TOPIC_STR+1)

struct addrsub_node
{
    RB_ENTRY(addrsub_node) entry;
    char addr_topic_str[MAX_ADDR_TOPIC_STR];
    char addr_str[MAX_ADDR_STR];
    char topic_str[MAX_TOPIC_STR];
};

static int addrsub_cmp(struct addrsub_node *e1, struct addrsub_node *e2)
{
    return strcmp(e1->addr_topic_str, e2->addr_topic_str);
}

RB_HEAD(addrsublist, addrsub_node) addrsub_head = RB_INITIALIZER(&addrsub_head);
RB_PROTOTYPE(addrsublist, addrsub_node, entry, addrsub_cmp);
RB_GENERATE(addrsublist, addrsub_node, entry, addrsub_cmp);

int addrsub_insert(const char *addr_str, const char *topic_str)
{
    struct addrsub_node *data;
    struct addrsub_node key;

    snprintf(key.addr_topic_str, MAX_ADDR_TOPIC_STR, "%s %s", addr_str, topic_str);
    if (NULL != RB_FIND(addrsublist, &addrsub_head, &key))
        return 1;

    if (NULL == (data = malloc(sizeof(struct addrsub_node))))
        return 1;

    strncpy(data->addr_str, addr_str, MAX_ADDR_STR);
    strncpy(data->topic_str, topic_str, MAX_TOPIC_STR);
    snprintf(data->addr_topic_str, MAX_ADDR_TOPIC_STR, "%s %s", addr_str, topic_str);

    if (NULL == RB_INSERT(addrsublist, &addrsub_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("addrsub_insert collision");  // can't happen
        free(data);
        return 1;
    }
}

int addrsub_remove(const char *addr_str, const char *topic_str)
{
    struct addrsub_node key;
    struct addrsub_node *res;

    snprintf(key.addr_topic_str, MAX_ADDR_TOPIC_STR, "%s %s", addr_str, topic_str);

    res = RB_FIND(addrsublist, &addrsub_head, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(addrsublist, &addrsub_head, res);
    free(res);
    return 0;
}

void addrsub_print(void)
{
    struct addrsub_node *i;
    RB_FOREACH(i, addrsublist, &addrsub_head)
    {
        LOG_INFO("addr_str=%s topic=%s\n", i->addr_str, i->topic_str);
    }
}

void addrsub_foreach(addrsub_iterate_func f, void *userdata)
{
    struct addrsub_node *i;
    RB_FOREACH(i, addrsublist, &addrsub_head)
    {
        f(i->addr_str, i->topic_str, userdata);
    }
}

struct count_topic
{
    int count;
    const char *topic;
};

static void addrsub_count(const char *addr_str, const char *topic_str, void *userdata)
{
    struct count_topic *ct = (struct count_topic *)userdata;
    if (0 == strcmp(topic_str, ct->topic))
        ct->count++;
}

int addrsub_count_topic(const char *topic)
{   // FIXME, horribly inefficient
    struct count_topic ct = {0, topic};
    addrsub_foreach(addrsub_count, &ct);
    return ct.count;
}


