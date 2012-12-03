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
#include "escape.h"

char *str_escape(const char *in, char *out)
{
    char *orig_out = out;
    while(*in)
    {
        switch(*in)
        {
            case ' ':
            case '\r':
            case '\n':
            case '"':
            case '\\':
                *out++ = '\\';
            break;
        }
        *out++ = *in++;
    }
    *out = 0;
    return orig_out;
}

char *str_unescape(const char *in, char *out)
{
    char *orig_out = out;
    while(*in)
    {
        if (*in == '\\')
        {
            in++;
            switch(*in)
            {
                case ' ':
                    *out++ = ' ';
                break;
                case 'r':
                    *out++ = '\r';
                break;
                case 'n':
                    *out++ = '\n';
                break;
                case '"':
                    *out++ = '"';
                break;
                case '\\':
                    *out++ = '\\';
                break;
                default:
                    LOG_WARN("unknown escape \\%c", *in);
                break;
            }
            in++;
        }
        else
        {
            *out++ = *in++;
        }
    }
    *out = 0;
    return orig_out;
}

