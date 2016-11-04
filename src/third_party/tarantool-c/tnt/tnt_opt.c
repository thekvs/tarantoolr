/*
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <sys/uio.h>

#include <sys/types.h>
#include <sys/time.h>

#include <uri.h>

#include <tarantool/tnt_mem.h>
#include <tarantool/tnt_net.h>

int
tnt_opt_init(struct tnt_opt *opt)
{
	memset(opt, 0, sizeof(struct tnt_opt));
	opt->recv_buf = 16384;
	opt->send_buf = 16384;
	opt->tmout_connect.tv_sec = 16;
	opt->tmout_connect.tv_usec = 0;
	opt->uri = tnt_mem_alloc(sizeof(struct uri));
	if (!opt->uri) return -1;
	return 0;
}

void
tnt_opt_free(struct tnt_opt *opt)
{
	if (opt->uristr)
		tnt_mem_free((void *)opt->uristr);
	tnt_mem_free((void *)opt->uri);
}

int
tnt_opt_set(struct tnt_opt *opt, enum tnt_opt_type name, va_list args)
{
	struct timeval *tvp;
	switch (name) {
	case TNT_OPT_URI:
		if (opt->uristr) tnt_mem_free((void *)opt->uristr);
		opt->uristr = tnt_mem_dup(va_arg(args, char*));
		if (opt->uristr == NULL)
			return TNT_EMEMORY;
		if (uri_parse(opt->uri, opt->uristr) == -1)
			return TNT_EFAIL;
		break;
	case TNT_OPT_TMOUT_CONNECT:
		tvp = va_arg(args, struct timeval*);
		memcpy(&opt->tmout_connect, tvp, sizeof(struct timeval));
		break;
	case TNT_OPT_TMOUT_RECV:
		tvp = va_arg(args, struct timeval*);
		memcpy(&opt->tmout_recv, tvp, sizeof(struct timeval));
		break;
	case TNT_OPT_TMOUT_SEND:
		tvp = va_arg(args, struct timeval*);
		memcpy(&opt->tmout_send, tvp, sizeof(struct timeval));
		break;
	case TNT_OPT_SEND_CB:
		opt->send_cb = va_arg(args, void*);
		break;
	case TNT_OPT_SEND_CBV:
		opt->send_cbv = va_arg(args, void*);
		break;
	case TNT_OPT_SEND_CB_ARG:
		opt->send_cb_arg = va_arg(args, void*);
		break;
	case TNT_OPT_SEND_BUF:
		opt->send_buf = va_arg(args, int);
		break;
	case TNT_OPT_RECV_CB:
		opt->recv_cb = va_arg(args, void*);
		break;
	case TNT_OPT_RECV_CB_ARG:
		opt->recv_cb_arg = va_arg(args, void*);
		break;
	case TNT_OPT_RECV_BUF:
		opt->recv_buf = va_arg(args, int);
		break;
	default:
		return TNT_EFAIL;
	}
	return TNT_EOK;
}
