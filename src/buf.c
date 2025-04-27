#include "buf.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Weft_Buf *new_buf(size_t cap)
{
	Weft_Buf *buf = malloc(sizeof(Weft_Buf) + cap);
	if (!buf) {
		fprintf(stderr,
		        "Failed to allocate %zu bytes: %s\n",
		        sizeof(Weft_Buf) + cap,
		        strerror(errno));
		exit(1);
	}

	buf->cap = cap;
	buf->at = 0;

	return buf;
}

size_t buf_get_at(const Weft_Buf *buf)
{
	return buf->at;
}

Weft_Buf *buf_free(Weft_Buf *buf)
{
	if (!buf) {
		return NULL;
	}
	free(buf);

	return NULL;
}

static Weft_Buf *realloc_buf(Weft_Buf *buf, size_t cap)
{
	buf = realloc(buf, sizeof(Weft_Buf) + cap);
	if (!buf) {
		fprintf(stderr,
		        "Failed to allocate %zu bytes: %s\n",
		        sizeof(Weft_Buf) + cap,
		        strerror(errno));
		exit(1);
	}
	buf->cap = cap;

	return buf;
}

static size_t expand_cap(const Weft_Buf *buf, size_t request)
{
	return 2 * (buf->at + request);
}

Weft_Buf *buf_push(Weft_Buf *buf, const void *src, size_t size)
{
	if (buf->at + size > buf->cap) {
		buf = realloc_buf(buf, expand_cap(buf, size));
	}

	memmove(buf->raw + buf->at, src, size);
	buf->at += size;

	return buf;
}

const void *buf_peek(const Weft_Buf *buf, size_t size)
{
	return buf->raw + buf->at - size;
}

static bool is_shrinkable(const Weft_Buf *buf)
{
	return buf->at <= buf->cap / 4;
}

static size_t shrink_cap(const Weft_Buf *buf)
{
	return (buf->at + buf->cap) / 2;
}

static Weft_Buf *shrink_buf(Weft_Buf *buf)
{
	if (is_shrinkable(buf)) {
		return realloc_buf(buf, shrink_cap(buf));
	}
	return buf;
}

Weft_Buf *buf_drop(Weft_Buf *buf, size_t size)
{
	buf->at -= size;
	return shrink_buf(buf);
}

Weft_Buf *buf_pop(void *dest, Weft_Buf *buf, size_t size)
{
	memcpy(dest, buf_peek(buf, size), size);
	return buf_drop(buf, size);
}
