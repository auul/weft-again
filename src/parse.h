#ifndef WEFT_PARSE_H
#define WEFT_PARSE_H

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>

// Forward Declarations

typedef struct weft_buf Weft_Buf;
typedef struct weft_parse_file Weft_ParseFile;
typedef enum weft_parse_type Weft_ParseType;
typedef struct weft_parse_token Weft_ParseToken;
typedef struct weft_parse_list Weft_ParseList;
typedef struct weft_parse_block Weft_ParseBlock;
typedef struct weft_parse_state Weft_ParseState;

// Data Types

struct weft_parse_file {
	char *path;
	char *src;
};

enum weft_parse_type {
	WEFT_PARSE_ERROR,
	WEFT_PARSE_EMPTY,
	WEFT_PARSE_INDENT,
	WEFT_PARSE_OP,
	WEFT_PARSE_INT,
	WEFT_PARSE_FLOAT,
	WEFT_PARSE_CHAR,
	WEFT_PARSE_STR,
	WEFT_PARSE_SHUFFLE,
	WEFT_PARSE_WORD,

	WEFT_PARSE_LIST,
	WEFT_PARSE_BLOCK,
};

struct weft_parse_token {
	Weft_ParseFile *file;
	const char *src;
	size_t len;

	Weft_ParseType type;
	union {
		void *ptr;
		size_t indent;
		long inum;
		double fnum;
		uint32_t cnum;
	};
};

struct weft_parse_list {
	Weft_ParseToken car;
	Weft_ParseList *cdr;
};

struct weft_parse_block {
	Weft_ParseToken head;
	Weft_ParseList *body;
};

struct weft_parse_state {
	Weft_ParseFile *file;
	const char *src;

	size_t indent;
	Weft_Buf *indent_stack;

	Weft_Buf *token_stack;
	size_t base;
	Weft_Buf *base_stack;

	Weft_ParseList *list;
	Weft_Buf *list_stack;
	Weft_ParseList *node;
	Weft_Buf *node_stack;
};

// Functions

Weft_ParseFile *parse_file_load(const char *path);
Weft_ParseFile *parse_file_from_src(const char *src);
void parse_token_print(const Weft_ParseToken token);
Weft_ParseToken parse_list_pop(Weft_ParseList **list_p);
void parse_list_print(const Weft_ParseList *list);

void parse_error(
	Weft_ParseFile *file, const char *src, size_t len, const char *fmt, ...);
void parse_error_v(Weft_ParseFile *file,
                   const char *src,
                   size_t len,
                   const char *fmt,
                   va_list args);
Weft_ParseToken parse_empty(Weft_ParseFile *file, const char *src);
Weft_ParseToken parse_token(Weft_ParseFile *file, const char *src);
void parse_init(Weft_ParseState *P);
void parse_exit(Weft_ParseState *P);
Weft_ParseList *parse(Weft_ParseState *P, Weft_ParseFile *file);

#endif
