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
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdbool.h>
#include <ev.h>
#include <fcntl.h>

#include <common/logging.h>

#include "serial.h"
#include "line.h"
#include "mqtt.h"
#include "router.h"
#include "cmd.h"
#include "escape.h"

extern server_context_t g_srvctx;

static int serialOpen(const char *port)
{
	int fd;
	struct termios t_opt;

	if ((fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK)) < 0)
    {
		LOG_ERROR("Could not open serial port %s", port);
		return -1;
	}

	fcntl(fd, F_SETFL, 0);
	tcgetattr(fd, &t_opt);
	cfsetispeed(&t_opt, B115200);
	cfsetospeed(&t_opt, B115200);
	t_opt.c_cflag |= (CLOCAL | CREAD);
    t_opt.c_cflag &= ~PARENB;
	t_opt.c_cflag &= ~CSTOPB;
	t_opt.c_cflag &= ~CSIZE;
	t_opt.c_cflag |= CS8;
	t_opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	t_opt.c_iflag &= ~(IXON | IXOFF | IXANY);
	t_opt.c_oflag &= ~OPOST;
	t_opt.c_cc[VMIN] = 0;
	t_opt.c_cc[VTIME] = 10;
	tcflush(fd, TCIOFLUSH);
	tcsetattr(fd, TCSANOW, &t_opt);

	return fd;
}

static void serial_ev_cb(struct ev_loop *loop, struct ev_io *watcher, int revents)
{
    ssize_t count;
    uint8_t c;

    if(EV_ERROR & revents)
    {
        LOG_ERROR("got invalid event");
        return;
    }

    if (EV_READ & revents)
    {
        // Receive message from client socket
        count = read(watcher->fd, &c, 1);

        if(count <= 0)
        {
            LOG_ERROR("read error");
            return;
        }
        else
        {
            line_putc(c);
        }
    }
}

void line_rx(const char *line)
{
    cmd_process(line);
}

int serial_write(serial_context_t *serialctx, const char *buf, size_t len)
{
#if 1
    ssize_t count;
    while(len)
    {
        int fd = serialctx->fd == 0 ? 1 : serialctx->fd;
        count = write(fd, buf, len);
        if (count < 0)
            return -1;
        len -= count;
        buf += count;
    }
#else
    while(len--)
        printf("(%c)", *buf++);
#endif
    return 0;
}

int serial_init(struct ev_loop *loop, serial_context_t *serialctx, const char *device)
{
    line_init();    // FIXME, this uses a global instance

    if (0==strcmp(device, "-"))
    {
        serialctx->fd = STDIN_FILENO;
    }
    else
    if ((serialctx->fd = serialOpen(device)) < 0)
    {
        LOG_ERROR("Failed to open %s", device);
        return 1;
    }

    serialctx->loop = loop;

    if (NULL == (serialctx->w_console = (struct ev_io *)malloc(sizeof(struct ev_io))))
    {
        LOG_ERROR("serial_init: out of mem");
        return 1;
    }

    // hook in callback
    if (NULL != serialctx->w_console)
    {
        ev_io_init(serialctx->w_console, serial_ev_cb, serialctx->fd, EV_READ);
        ev_io_start(loop, serialctx->w_console);
    }

    return 0;
}

int serial_write_str(serial_context_t *serialctx, const char *str, bool trailing_space)
{
    int rc;
    if ((rc = serial_write(serialctx, str, strlen(str)))!=0)
        goto out;
    if ((rc = serial_write(serialctx, trailing_space ? " " : "\n", 1))!=0)
        goto out;
out:
    return rc;
}

int serial_write_str_escape(serial_context_t *serialctx, const char *str, char *escbuf, bool trailing_space)
{
    int rc;
    str_escape(str, escbuf);
    if ((rc = serial_write(serialctx, escbuf, strlen(escbuf)))!=0)
        goto out;
    if ((rc = serial_write(serialctx, trailing_space ? " " : "\n", 1))!=0)
        goto out;
out:
    return rc;
}

