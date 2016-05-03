
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
#include <stdint.h>
#include <string.h>

#include <tarantool/tnt_mem.h>
#include <tarantool/tnt_proto.h>
#include <tarantool/tnt_reply.h>
#include <tarantool/tnt_stream.h>
#include <tarantool/tnt_buf.h>

static void tnt_buf_free(struct tnt_stream *s) {
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	if (!sb->as && sb->data)
		tnt_mem_free(sb->data);
	if (sb->free)
		sb->free(s);
	tnt_mem_free(s->data);
	s->data = NULL;
}

static ssize_t
tnt_buf_read(struct tnt_stream *s, char *buf, size_t size) {
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	if (sb->data == NULL)
		return 0;
	if (sb->size == sb->rdoff)
		return 0;
	size_t avail = sb->size - sb->rdoff;
	if (size > avail)
		size = avail;
	memcpy(sb->data + sb->rdoff, buf, size);
	sb->rdoff += size;
	return size;
}

static char* tnt_buf_resize(struct tnt_stream *s, size_t size) {
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	size_t off = sb->size;
	size_t nsize = off + size;
	char *nd = realloc(sb->data, nsize);
	if (nd == NULL) {
		tnt_mem_free(sb->data);
		return NULL;
	}
	sb->data = nd;
	sb->alloc = nsize;
	return sb->data + off;
}

static ssize_t
tnt_buf_write(struct tnt_stream *s, const char *buf, size_t size) {
	if (TNT_SBUF_CAST(s)->as == 1) return -1;
	char *p = TNT_SBUF_CAST(s)->resize(s, size);
	if (p == NULL)
		return -1;
	memcpy(p, buf, size);
	TNT_SBUF_CAST(s)->size += size;
	s->wrcnt++;
	return size;
}

static ssize_t
tnt_buf_writev(struct tnt_stream *s, struct iovec *iov, int count) {
	if (TNT_SBUF_CAST(s)->as == 1) return -1;
	size_t size = 0;
	int i;
	for (i = 0 ; i < count ; i++)
		size += iov[i].iov_len;
	char *p = TNT_SBUF_CAST(s)->resize(s, size);
	if (p == NULL)
		return -1;
	for (i = 0 ; i < count ; i++) {
		memcpy(p, iov[i].iov_base, iov[i].iov_len);
		p += iov[i].iov_len;
	}
	TNT_SBUF_CAST(s)->size += size;
	s->wrcnt++;
	return size;
}

static int
tnt_buf_reply(struct tnt_stream *s, struct tnt_reply *r) {
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	if (sb->data == NULL)
		return -1;
	if (sb->size == sb->rdoff)
		return 1;
	size_t off = 0;
	int rc = tnt_reply(r, sb->data + sb->rdoff, sb->size - sb->rdoff, &off);
	if (rc == 0)
		sb->rdoff += off;
	return rc;
}

struct tnt_stream *tnt_buf(struct tnt_stream *s) {
	int allocated = s == NULL;
	s = tnt_stream_init(s);
	if (s == NULL)
		return NULL;
	/* allocating stream data */
	s->data = tnt_mem_alloc(sizeof(struct tnt_stream_buf));
	if (s->data == NULL) {
		if (allocated)
			tnt_stream_free(s);
		return NULL;
	}
	/* initializing interfaces */
	s->read       = tnt_buf_read;
	s->read_reply = tnt_buf_reply;
	s->write      = tnt_buf_write;
	s->writev     = tnt_buf_writev;
	s->free       = tnt_buf_free;
	/* initializing internal data */
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);
	sb->rdoff   = 0;
	sb->size    = 0;
	sb->alloc   = 0;
	sb->data    = NULL;
	sb->resize  = tnt_buf_resize;
	sb->free    = NULL;
	sb->subdata = NULL;
	sb->as      = 0;
	return s;
}

struct tnt_stream *tnt_buf_as(struct tnt_stream *s, char *buf, size_t buf_len)
{
	if (s == NULL) {
		s = tnt_buf(s);
		if (s == NULL)
			return NULL;
	}
	struct tnt_stream_buf *sb = TNT_SBUF_CAST(s);

	sb->data = buf;
	sb->size = buf_len;
	sb->alloc = buf_len;
	sb->as = 1;

	return s;
}
