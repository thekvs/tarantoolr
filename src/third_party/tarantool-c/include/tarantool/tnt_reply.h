#ifndef TNT_REPLY_H_INCLUDED
#define TNT_REPLY_H_INCLUDED

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

/**
 * \file tnt_reply.h
 * \brief Basic reply structure (parsing responses, e.t.c)
 */

/**
 * \brief Callback for recv reply from buffer
 *
 * \param ptr  pointer to buffer and offset
 * \param dst  copy reply to
 * \param size size to recv to
 *
 * \returns size of bytes written
 * \retval  -1 error
 */
typedef ssize_t (*tnt_reply_t)(void *ptr, char *dst, ssize_t size);

/*!
 * \brief basic reply structure
 */
struct tnt_reply {
	int alloc;		/*!< allocation mark */
	uint64_t bitmap;	/*!< bitmap of field IDs that was read */
	const char *buf;	/*!< points to beginning of buffer */
	size_t buf_size;	/*!< size of query buffer */
	uint64_t code;		/*!< response code (0 is success, error number if not) */
	uint64_t sync;		/*!< synchronization id */
	uint64_t schema_id;	/*!< unique schema id */
	const char *error;	/*!< error message (NULL if not present) */
	const char *error_end;	/*!< end of error message (NULL if not present) */
	const char *data;	/*!< tuple data (NULL if not present) */
	const char *data_end;	/*!< end if tuple data (NULL if not present) */
};

/*!
 * \brief Get error number
 */
#define TNT_REPLY_ERR(R) ((R)->code)

/*!
 * \brief Allocate and init reply object
 *
 * if reply pointer is NULL, then new stream will be created
 *
 * \param r reply object pointer
 *
 * \returns reply object pointer
 * \retval  NULL memory allocation failure
 */
struct tnt_reply *
tnt_reply_init(struct tnt_reply *r);

/*!
 * \brief Free previously inited reply object
 *
 * \param r reply object pointer
 */
void
tnt_reply_free(struct tnt_reply *r);

/*!
 * \brief process buffer as iproto reply
 *
 * \param[in]  r    reply object pointer
 * \param[in]  buf  buffer data pointer
 * \param[in]  size buffer data size
 * \param[out] off  returned offset, may be NULL
 *
 * if reply is fully read, then zero is returned and offset set to the
 * end of reply data in buffer.
 *
 * if reply is not complete, then 1 is returned and offset set to the
 * size needed to read.
 *
 * if there were error while parsing reply, -1 is returned.
 *
 * \returns status of processing reply
 * \retval 0  read reply
 * \retval 1  need 'offset' bytes more
 * \retval -1 error while parsing request
 */
int
tnt_reply(struct tnt_reply *r, char *buf, size_t size, size_t *off);

/*!
 * \brief Process iproto reply with supplied recv function
 *
 * \param r   reply object pointer
 * \param rcv supplied recv function
 * \param ptr recv function argument
 *
 * \returns status of parsing
 * \retval  0 ok
 * \retval -1 error, while parsing response
 */
int
tnt_reply_from(struct tnt_reply *r, tnt_reply_t rcv, void *ptr);

#endif /* TNT_REPLY_H_INCLUDED */
