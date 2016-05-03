#ifndef TNT_SELECT_H_INCLUDED
#define TNT_SELECT_H_INCLUDED

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

#include <tarantool/tnt_stream.h>

/**
 * \file tnt_select.h
 * \brief Select request
 */

/**
 * \brief Construct select request and write it into stream
 *
 * \param s        stream object
 * \param space    space no
 * \param index    index no
 * \param limit    limit of tuples to select
 * \param offset   offset of tuples to select
 * \param iterator iterator to use for select
 * \param key      key for select
 *
 * \returns        number of bytes written to stream
 * \retval      -1 oom
 */
ssize_t
tnt_select(struct tnt_stream *s, uint32_t space, uint32_t index,
	   uint32_t limit, uint32_t offset, uint8_t iterator,
	   struct tnt_stream *key);

#endif /* TNT_SELECT_H_INCLUDED */
