#ifndef TNT_OBJECT_H_INCLUDED
#define TNT_OBJECT_H_INCLUDED

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
 * \file tnt_object.h
 * \brief Object for manipulating msgpack objects
 */

#include <stdarg.h>

/**
 * \brief for internal use
 */
struct tnt_sbo_stack {
	size_t   offset;
	uint32_t size;
	int8_t   type;
};

/**
 * \brief type of packing msgpack array/map
 *
 * - TNT_SBO_SIMPLE - without packing, demanding size to be specified
 * - TNT_SBO_SPARSE - 5 bytes always allocated for map/array, size is ignored
 * - TNT_SBO_PACKED - 1 byte is alloced for map/array, if needed more, then
 *                    everything is moved to n bytes, when called
 *                    "tnt_object_container_close"
 */
enum tnt_sbo_type {
	TNT_SBO_SIMPLE = 0,
	TNT_SBO_SPARSE,
	TNT_SBO_PACKED,
};

struct tnt_sbuf_object {
	struct tnt_sbo_stack *stack;
	uint8_t stack_size;
	uint8_t stack_alloc;
	enum tnt_sbo_type type;
};

#define TNT_OBJ_CAST(SB) ((struct tnt_sbuf_object *)(SB)->subdata)
#define TNT_SOBJ_CAST(S) TNT_OBJ_CAST(TNT_SBUF_CAST(S))

/**
 * \brief Set type of packing for objects
 *
 * Type must be set before first value was written
 *
 * \param s    tnt_object instance
 * \param type type of packing
 *
 * \returns status of operation
 * \retval  -1 (something was written before
 * \retval   0 success
 */
int
tnt_object_type(struct tnt_stream *s, enum tnt_sbo_type type);

/**
 * \brief create and initialize tnt_object
 *
 * tnt_object is used to create msgpack values: keys/tuples/args for
 * passing them into tnt_request or tnt_<operation>
 * if stream object is NULL, then new stream object will be created
 *
 * \param s object pointer
 *
 * \returns object pointer
 * \retval NULL error
 */
struct tnt_stream *
tnt_object(struct tnt_stream *s);

/**
 * \brief Add nil to an stream object
 */
ssize_t
tnt_object_add_nil(struct tnt_stream *s);

/**
 * \brief Add integer to an stream object
 */
ssize_t
tnt_object_add_int(struct tnt_stream *s, int64_t value);

/**
 * \brief Add string to an stream object
 */
ssize_t
tnt_object_add_str(struct tnt_stream *s, const char *str, uint32_t len);

/**
 * \brief Add null terminated string to an stream object
 */
ssize_t
tnt_object_add_strz(struct tnt_stream *s, const char *strz);

/**
 * \brief Add binary object to an stream object
 */
ssize_t
tnt_object_add_bin(struct tnt_stream *s, const void *bin, uint32_t len);

/**
 * \brief Add boolean to an stream object
 */
ssize_t
tnt_object_add_bool(struct tnt_stream *s, char value);

/**
 * \brief Add floating value to an stream object
 */
ssize_t
tnt_object_add_float(struct tnt_stream *s, float value);

/**
 * \brief Add double precision floating value to an stream object
 */
ssize_t
tnt_object_add_double(struct tnt_stream *s, double value);

/**
 * \brief Append array header to stream object
 * \sa tnt_sbo_type
 */
ssize_t
tnt_object_add_array(struct tnt_stream *s, uint32_t size);

/**
 * \brief Append map header to stream object
 * \sa tnt_sbo_type
 */
ssize_t
tnt_object_add_map(struct tnt_stream *s, uint32_t size);

/**
 * \brief Close array/map in case TNT_SBO_PACKED/TNT_SBO_SPARSE were used
 * \sa tnt_sbo_type
 */
ssize_t
tnt_object_container_close(struct tnt_stream *s);

/**
 * \brief create immutable tnt_object from given buffer
 */
struct tnt_stream *
tnt_object_as(struct tnt_stream *s, char *buf, size_t buf_len);

/**
 * \brief verify that object is valid msgpack structure
 * \param s object pointer
 * \param type -1 on check without validating type, otherwise `enum mp_type`
 */
int
tnt_object_verify(struct tnt_stream *s, int8_t type);

/**
 * \brief reset tnt_object to basic state
 * this function doesn't deallocate memory, but instead it simply sets all
 * pointers to beginning
 */
int
tnt_object_reset(struct tnt_stream *s);

/**
 * \brief create tnt_object from format string/values (va_list variation)
 *
 * \code{.c}
 * \*to get a msgpack array of two items: number 42 and map (0->"false, 2->"true")*\
 * tnt_object_format(s, "[%d {%d%s%d%s}]", 42, 0, "false", 1, "true");
 * \endcode
 *
 * \param s   tnt_object instance
 * \param fmt zero-end string, containing structure of resulting
 * msgpack and types of next arguments.
 * Format can contain '[' and ']' pairs, defining arrays,
 * '{' and '}' pairs, defining maps, and format specifiers, described below:
 * %d, %i - int
 * %u - unsigned int
 * %ld, %li - long
 * %lu - unsigned long
 * %lld, %lli - long long
 * %llu - unsigned long long
 * %hd, %hi - short
 * %hu - unsigned short
 * %hhd, %hhi - char (as number)
 * %hhu - unsigned char (as number)
 * %f - float
 * %lf - double
 * %b - bool
 * %s - zero-end string
 * %.*s - string with specified length
 * %% is ignored
 * %'smth else' assert and undefined behaviour
 * NIL - a nil value
 * all other symbols are ignored.
 *
 * \sa tnt_object_vformat
 * \sa tnt_object_format
 */
ssize_t
tnt_object_format(struct tnt_stream *s, const char *fmt, ...);

/**
 * \brief create tnt_object from format string/values
 * \sa tnt_object_vformat
 * \sa tnt_object_format
 */
ssize_t
tnt_object_vformat(struct tnt_stream *s, const char *fmt, va_list vl);

#endif /* TNT_OBJECT_H_INCLUDED */
