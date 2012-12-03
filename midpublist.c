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
#include "midpublist.h"
#include "router.h"

struct midpub_node
{
    RB_ENTRY(midpub_node) entry;
    int mid;
    char id_str[MAX_ID_STR];
    char addr_str[MAX_ADDR_STR];
};

static int midpub_cmp(struct midpub_node *e1, struct midpub_node *e2)
{
    return (e1->mid < e2->mid ? -1 : e1->mid > e2->mid);
}

RB_HEAD(midpublist, midpub_node) midpub_head = RB_INITIALIZER(&midpub_head);
RB_PROTOTYPE(midpublist, midpub_node, entry, midpub_cmp);
RB_GENERATE(midpublist, midpub_node, entry, midpub_cmp);

int midpub_find(int mid, char **id_str, char **addr_str)
{
    struct midpub_node key;
    struct midpub_node *res;
    key.mid = mid;
    res = RB_FIND(midpublist, &midpub_head, &key);
    if (NULL == res)
        return 1;
    *id_str = res->id_str;
    *addr_str = res->addr_str;
    return 0;
}

int midpub_insert(int mid, const char *id_str, const char *addr_str)
{
    struct midpub_node *data;

    if (NULL == (data = malloc(sizeof(struct midpub_node))))
        return 1;

    data->mid = mid;
    strncpy(data->id_str, id_str, MAX_ID_STR);
    strncpy(data->addr_str, addr_str, MAX_ADDR_STR);

    if (NULL == RB_INSERT(midpublist, &midpub_head, data))    // NULL means OK
    {
        return 0;
    }
    else
    {
        LOG_CRITICAL("middconn_insert mid collision");  // can't happen
        return 1;
    }
}

int midpub_remove(int mid)
{
    struct midpub_node key;
    struct midpub_node *res;
    key.mid = mid;
    res = RB_FIND(midpublist, &midpub_head, &key);
    if (NULL == res)
        return 1;
    RB_REMOVE(midpublist, &midpub_head, res);
    free(res);
    return 0;
}

void midpub_print(void)
{
    struct midpub_node *i;
    RB_FOREACH(i, midpublist, &midpub_head)
    {
        LOG_INFO("mid=%d -> id=%s addr_str=%s\n", i->mid, i->id_str, i->addr_str);
    }
}

void midpub_foreach(midpub_iterate_func f, void *userdata)
{
    struct midpub_node *i;
    RB_FOREACH(i, midpublist, &midpub_head)
    {
        f(i->mid, i->id_str, i->addr_str, userdata);
    }
}


