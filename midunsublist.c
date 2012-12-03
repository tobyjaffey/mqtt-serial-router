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
#include "midunsublist.h"
#include "router.h"

struct midunsub_node
{
    RB_ENTRY(midunsub_node) entry;
    int mid;
    char id_str[MAX_ID_STR];
    char addr_str[MAX_ADDR_STR];
    char topic_str[MAX_TOPIC_STR];
};

static int midunsub_cmp(struct midunsub_node *e1, struct midunsub_node *e2)
{
    return (e1->mid < e2->mid ? -1 : e1->mid > e2->mid);
}

RB_HEAD(midunsublist, midunsub_node) midunsub_head = RB_INITIALIZER(&midunsub_head);
RB_PROTOTYPE(midunsublist, midunsub_node, entry, midunsub_cmp);
RB_GENERATE(midunsublist, midunsub_node, entry, midunsub_cmp);

int midunsub_find(int mid, char **id_str, char **addr_str, char **topic_str)
{
    struct midunsub_node key;
    struct midunsub_node *res;
    key.mid = mid;
    res = RB_FIND(midunsublist, &midunsub_head, &key);
    if (NULL == res)
        return 1;
    *id_str = res->id_str;
    *addr_str = res->addr_str;
    *topic_str = res->topic_str;
    return 0;
}

int midunsub_insert(int mid, const char *id_str, const char *addr_str, const char *topic_str)
{
    struct midunsub_node *data;

    if (NULL == (data = malloc(sizeof(struct midunsub_node))))
        return 1;

    data->mid = mid;
    strncpy(data->id_str, id_str, MAX_ID_STR);
    strncpy(data->addr_str, addr_str, MAX_ADDR_STR);
    strncpy(data->topic_str, topic_str, MAX_TOPIC_STR);

    if (NULL == RB_INSERT(midunsublist, &midunsub_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("middconn_insert mid collision");  // can't happen
        return 1;
    }
}

int midunsub_remove(int mid)
{
    struct midunsub_node key;
    struct midunsub_node *res;
    key.mid = mid;
    res = RB_FIND(midunsublist, &midunsub_head, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(midunsublist, &midunsub_head, res);
    free(res);
    return 0;
}

void midunsub_print(void)
{
    struct midunsub_node *i;
    RB_FOREACH(i, midunsublist, &midunsub_head)
    {
        LOG_INFO("mid=%d -> id=%s addr_str=%s topic=%s\n", i->mid, i->id_str, i->addr_str, i->topic_str);
    }
}

void midunsub_foreach(midunsub_iterate_func f, void *userdata)
{
    struct midunsub_node *i;
    RB_FOREACH(i, midunsublist, &midunsub_head)
    {
        f(i->mid, i->id_str, i->addr_str, i->topic_str, userdata);
    }
}


