#ifndef WEFT_BUF_H
#define WEFT_BUF_H

#include <stddef.h>

// Forward Declarations

typedef struct weft_buf Weft_Buf;

// Data Types

struct weft_buf {
	size_t cap;
	size_t at;
	char raw[];
};

// Functions

Weft_Buf *new_buf(size_t cap);
size_t buf_get_at(const Weft_Buf *buf);
Weft_Buf *buf_free(Weft_Buf *buf);
Weft_Buf *buf_push(Weft_Buf *buf, const void *src, size_t size);
const void *buf_peek(const Weft_Buf *buf, size_t size);
Weft_Buf *buf_drop(Weft_Buf *buf, size_t size);
Weft_Buf *buf_pop(void *dest, Weft_Buf *buf, size_t size);

#endif
