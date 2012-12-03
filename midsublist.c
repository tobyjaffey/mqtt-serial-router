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
#include "midsublist.h"
#include "router.h"

struct midsub_node
{
    RB_ENTRY(midsub_node) entry;
    int mid;
    char id_str[MAX_ID_STR];
    char addr_str[MAX_ADDR_STR];
    char topic_str[MAX_TOPIC_STR];
};

static int midsub_cmp(struct midsub_node *e1, struct midsub_node *e2)
{
    return (e1->mid < e2->mid ? -1 : e1->mid > e2->mid);
}

RB_HEAD(midsublist, midsub_node) midsub_head = RB_INITIALIZER(&midsub_head);
RB_PROTOTYPE(midsublist, midsub_node, entry, midsub_cmp);
RB_GENERATE(midsublist, midsub_node, entry, midsub_cmp);

int midsub_find(int mid, char **id_str, char **addr_str, char **topic_str)
{
    struct midsub_node key;
    struct midsub_node *res;
    key.mid = mid;
    res = RB_FIND(midsublist, &midsub_head, &key);
    if (NULL == res)
        return 1;
    *id_str = res->id_str;
    *addr_str = res->addr_str;
    *topic_str = res->topic_str;
    return 0;
}

int midsub_insert(int mid, const char *id_str, const char *addr_str, const char *topic_str)
{
    struct midsub_node *data;

    if (NULL == (data = malloc(sizeof(struct midsub_node))))
        return 1;

    data->mid = mid;
    strncpy(data->id_str, id_str, MAX_ID_STR);
    strncpy(data->addr_str, addr_str, MAX_ADDR_STR);
    strncpy(data->topic_str, topic_str, MAX_TOPIC_STR);

    if (NULL == RB_INSERT(midsublist, &midsub_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("middconn_insert mid collision");  // can't happen
        return 1;
    }
}

int midsub_remove(int mid)
{
    struct midsub_node key;
    struct midsub_node *res;
    key.mid = mid;
    res = RB_FIND(midsublist, &midsub_head, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(midsublist, &midsub_head, res);
    free(res);
    return 0;
}

void midsub_print(void)
{
    struct midsub_node *i;
    RB_FOREACH(i, midsublist, &midsub_head)
    {
        LOG_INFO("mid=%d -> id=%s addr_str=%s topic=%s\n", i->mid, i->id_str, i->addr_str, i->topic_str);
    }
}

void midsub_foreach(midsub_iterate_func f, void *userdata)
{
    struct midsub_node *i;
    RB_FOREACH(i, midsublist, &midsub_head)
    {
        f(i->mid, i->id_str, i->addr_str, i->topic_str, userdata);
    }
}


