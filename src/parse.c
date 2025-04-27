#include "parse.h"
#include "buf.h"
#include "char.h"
#include "file.h"
#include "gc.h"
#include "shuffle.h"
#include "str.h"

#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

// Constants

static const char delim_list[] = "(){}[]#:;";
static const char op_list[] = "[]:;";
static const char shuffle_error_list[] = "{)]:;";

#define FMT_ERROR "\e[91m"
#define FMT_RESET "\e[0m"

// Functions

#define len_of(const_str) (sizeof(const_str) - 1)

static Weft_ParseFile *new_parse_file(char *path, char *src)
{
	Weft_ParseFile *file = gc_alloc(sizeof(Weft_ParseFile));
	file->path = path;
	file->src = src;

	return file;
}

static char *copy_str_n(const char *src, size_t len)
{
	char *dest = gc_alloc(len + 1);
	memcpy(dest, src, len);
	dest[len] = 0;

	return dest;
}

static char *copy_str(const char *src)
{
	return copy_str_n(src, strlen(src));
}

Weft_ParseFile *parse_file_load(const char *path)
{
	FILE *f = file_open(path, "r");
	if (!f) {
		return NULL;
	}

	size_t src_len = file_len(f);
	char *src = gc_alloc(src_len + 1);
	fread(src, src_len, 1, f);
	fclose(f);

	return new_parse_file(copy_str(path), src);
}

Weft_ParseFile *parse_file_from_src(const char *src)
{
	return new_parse_file(NULL, copy_str(src));
}

static Weft_ParseToken tag_token(Weft_ParseFile *file,
                                 const char *src,
                                 size_t len,
                                 Weft_ParseType type)
{
	Weft_ParseToken token = {
		.file = file,
		.src = src,
		.len = len,
		.type = type,
	};
	return token;
}

static Weft_ParseToken
tag_indent(Weft_ParseFile *file, const char *src, size_t len)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_INDENT);
	if (src[0] == '\n') {
		token.indent = len - len_of("\n");
	} else {
		token.indent = len;
	}
	return token;
}

static Weft_ParseToken
tag_int(Weft_ParseFile *file, const char *src, size_t len, long inum)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_INT);
	token.inum = inum;

	return token;
}

static Weft_ParseToken
tag_float(Weft_ParseFile *file, const char *src, size_t len, double fnum)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_FLOAT);
	token.fnum = fnum;

	return token;
}

static Weft_ParseToken
tag_char(Weft_ParseFile *file, const char *src, size_t len, uint32_t cnum)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_CHAR);
	token.cnum = cnum;

	return token;
}

static Weft_ParseToken
tag_str(Weft_ParseFile *file, const char *src, size_t len, Weft_Str *str)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_STR);
	token.ptr = str;

	return token;
}

static Weft_ParseToken tag_shuffle(Weft_ParseFile *file,
                                   const char *src,
                                   size_t len,
                                   Weft_Shuffle *shuffle)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_SHUFFLE);
	token.ptr = shuffle;

	return token;
}

static Weft_ParseToken tag_list(Weft_ParseFile *file,
                                const char *src,
                                size_t len,
                                Weft_ParseList *list)
{
	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_LIST);
	token.ptr = list;

	return token;
}

static Weft_ParseList *get_list_end(Weft_ParseList *list)
{
	while (list->cdr) {
		list = list->cdr;
	}
	return list;
}

static Weft_ParseToken tag_block(Weft_ParseBlock *block)
{
	Weft_ParseFile *file = block->head.file;
	const char *src = block->head.src;

	const char *end;
	if (block->body) {
		Weft_ParseList *node = get_list_end(block->body);
		end = node->car.src + node->car.len;
	} else {
		end = block->head.src + block->head.len;
	}
	size_t len = end - src;

	Weft_ParseToken token = tag_token(file, src, len, WEFT_PARSE_BLOCK);
	token.ptr = block;

	return token;
}

void parse_token_print(const Weft_ParseToken token)
{
	switch (token.type) {
	case WEFT_PARSE_ERROR:
		printf("error(%zu)", token.len);
		break;
	case WEFT_PARSE_EMPTY:
		printf("empty(%zu)", token.len);
		break;
	case WEFT_PARSE_INDENT:
		printf("indent(%zu)", token.indent);
		break;
	case WEFT_PARSE_OP:
		printf("op(%c)", token.src[0]);
		break;
	case WEFT_PARSE_INT:
		printf("int(%li)", token.inum);
		break;
	case WEFT_PARSE_CHAR:
		printf("char(");
		char_print(token.cnum);
		printf(")");
		break;
	case WEFT_PARSE_STR:
		printf("str(");
		str_print(token.ptr);
		printf(")");
		break;
	case WEFT_PARSE_SHUFFLE:
		printf("shuffle(");
		shuffle_print(token.ptr);
		printf(")");
		break;
	case WEFT_PARSE_WORD:
		printf("word(%.*s)", (unsigned)token.len, token.src);
		break;
	case WEFT_PARSE_LIST:
		printf("list(");
		parse_list_print(token.ptr);
		printf(")");
		break;
	case WEFT_PARSE_BLOCK:
		parse_token_print(((Weft_ParseBlock *)token.ptr)->head);
		printf(":(");
		parse_list_print(((Weft_ParseBlock *)token.ptr)->body);
		printf(")");
		break;
	default:
		printf("%u(%zu)", token.type, token.len);
		break;
	}
}

void parse_list_print(const Weft_ParseList *list)
{
	if (!list) {
		return;
	}

	parse_token_print(list->car);
	list = list->cdr;

	while (list) {
		printf(" ");
		parse_token_print(list->car);
		list = list->cdr;
	}
}

Weft_ParseList *new_parse_list_node(Weft_ParseToken car, Weft_ParseList *cdr)
{
	Weft_ParseList *node = gc_alloc(sizeof(Weft_ParseList));
	node->car = car;
	node->cdr = cdr;

	return node;
}

Weft_ParseToken parse_list_pop(Weft_ParseList **list_p)
{
	Weft_ParseList *list = *list_p;
	*list_p = list->cdr;

	return list->car;
}

Weft_ParseBlock *new_parse_block(Weft_ParseToken head, Weft_ParseList *body)
{
	Weft_ParseBlock *block = gc_alloc(sizeof(Weft_ParseBlock));
	block->head = head;
	block->body = body;

	return block;
}

static const char *get_line_at(size_t *line_no, const char *src, const char *at)
{
	*line_no = 0;
	const char *line = src;

	while (src < at) {
		if (*src == '\n') {
			line = src + len_of("\n");
			*line_no += 1;
		}
		src++;
	}
	return line;
}

static void print_error_msg(const char *path,
                            size_t line_no,
                            size_t col_no,
                            const char *fmt,
                            va_list args)
{
	if (path) {
		fprintf(stderr, "%s:", path);
	}
	fprintf(stderr, "%zu:%zu: " FMT_ERROR "error: " FMT_RESET, line_no, col_no);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
}

static void print_error_line_no(size_t line_no)
{
	fprintf(stderr, " %5zu | ", line_no);
}

static void
print_error_line_left(size_t line_no, size_t col_no, const char *line)
{
	print_error_line_no(line_no);
	fprintf(stderr, "%.*s", (unsigned)col_no, line);
}

static size_t get_line_len(const char *src)
{
	size_t len = 0;
	while (src[len] && src[len] != '\n') {
		len++;
	}
	return len;
}

static void print_error_line_mid(size_t line_no, const char *src, size_t len)
{
	size_t line_len = get_line_len(src);
	while (line_len < len) {
		fprintf(
			stderr, FMT_ERROR "%.*s" FMT_RESET "\n", (unsigned)line_len, src);
		src += line_len + len_of("\n");
		len -= line_len + len_of("\n");

		line_no++;
		print_error_line_no(line_no);
		line_len = get_line_len(src);
	}
	fprintf(stderr, FMT_ERROR "%.*s" FMT_RESET, (unsigned)len, src);
}

static void print_error_line_right(const char *src)
{
	fprintf(stderr, "%.*s\n", (unsigned)get_line_len(src), src);
}

void parse_error(
	Weft_ParseFile *file, const char *src, size_t len, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	parse_error_v(file, src, len, fmt, args);
	va_end(args);
}

void parse_error_v(Weft_ParseFile *file,
                   const char *src,
                   size_t len,
                   const char *fmt,
                   va_list args)
{
	size_t line_no;
	const char *line = get_line_at(&line_no, file->src, src);
	size_t col_no = src - file->src;

	print_error_msg(file->path, line_no, col_no, fmt, args);
	print_error_line_left(line_no, col_no, line);
	print_error_line_mid(line_no, src, len);
	print_error_line_right(src + len);
}

Weft_ParseToken parse_error_token(Weft_ParseFile *file,
                                  const char *src,
                                  size_t len,
                                  const char *hlight_src,
                                  size_t hlight_len,
                                  const char *fmt,
                                  ...)
{
	va_list args;
	va_start(args, fmt);
	parse_error_v(file, hlight_src, hlight_len, fmt, args);
	va_end(args);

	return tag_token(file, src, len, WEFT_PARSE_ERROR);
}

static bool is_line_comment(const char *src)
{
	return src[0] == '#';
}

static Weft_ParseToken parse_line_comment(Weft_ParseFile *file, const char *src)
{
	size_t len = get_line_len(src);
	if (src[len]) {
		len++;
	}
	return tag_token(file, src, len, WEFT_PARSE_EMPTY);
}

static bool is_comment_open(const char *src)
{
	return src[0] == '(';
}

static bool is_comment_close(const char *src)
{
	return src[0] == ')';
}

static Weft_ParseToken parse_comment(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("(");
	size_t nest = 1;

	while (nest) {
		if (!src[len]) {
			return parse_error_token(
				file, src, len, src, len_of("("), "Unmatched (");
		} else if (is_comment_open(src + len)) {
			len += len_of("(");
			nest++;
		} else if (is_comment_close(src + len)) {
			len += len_of(")");
			nest--;
		} else if (is_line_comment(src + len)) {
			len += parse_line_comment(file, src + len).len;
		} else {
			len++;
		}
	}
	return tag_token(file, src, len, WEFT_PARSE_EMPTY);
}

static bool is_line_empty(const char *src)
{
	size_t nest = 0;
	while (*src && *src != '\n') {
		if (is_line_comment(src)) {
			return true;
		} else if (is_comment_open(src)) {
			nest++;
		} else if (is_comment_close(src)) {
			nest--;
		} else if (!isspace(*src) && !nest) {
			return false;
		}
		src++;
	}
	return true;
}

static bool is_indent(const char *src)
{
	return src[0] == '\n' && !is_line_empty(src + len_of("\n"));
}

Weft_ParseToken parse_empty(Weft_ParseFile *file, const char *src)
{
	size_t len = 0;
	while (true) {
		if (is_line_comment(src + len)) {
			len += parse_line_comment(file, src + len).len;
		} else if (is_comment_open(src + len)) {
			len += parse_comment(file, src + len).len;
		} else if (is_indent(src + len)) {
			return tag_token(file, src, len, WEFT_PARSE_EMPTY);
		} else if (isspace(src[len])) {
			len++;
		} else {
			return tag_token(file, src, len, WEFT_PARSE_EMPTY);
		}
	}
}

static Weft_ParseToken parse_indent(Weft_ParseFile *file, const char *src)
{
	size_t len = 0;
	size_t nest = 0;

	while (true) {
		if (is_comment_open(src + len)) {
			nest++;
		} else if (is_comment_close(src + len)) {
			nest--;
		} else if (!isspace(src[len]) && !nest) {
			return tag_indent(file, src, len);
		}
		len++;
	}
}

static bool is_op(const char *src)
{
	return strchr(op_list, *src);
}

static Weft_ParseToken parse_op(Weft_ParseFile *file, const char *src)
{
	return tag_token(file, src, 1, WEFT_PARSE_OP);
}

static bool is_delim(const char *src)
{
	return !*src || isspace(*src) || strchr(delim_list, *src);
}

static size_t get_word_len(const char *src)
{
	size_t len = 1;
	while (!is_delim(src + len)) {
		len++;
	}
	return len;
}

static const char *skip_sign(const char *src)
{
	return (src[0] == '-') ? src + len_of("-") : src;
}

static bool is_binary(const char *src)
{
	src = skip_sign(src);
	return src[0] == '0' && (src[1] == 'b' || src[1] == 'B');
}

static bool parse_sign(size_t *len, const char *src)
{
	if (src[0] == '-') {
		*len += 1;
		return true;
	}
	return false;
}

static bool is_bit(char c)
{
	return c == '0' || c == '1';
}

static int get_bit(char c)
{
	return c - '0';
}

static long push_bit(long inum, char c)
{
	return (inum << 1) | get_bit(c);
}

static Weft_ParseToken parse_binary(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("0b");
	bool negative = parse_sign(&len, src);

	if (!src[len]) {
		return parse_error_token(
			file, src, len, src, len, "Expected 0|1 in binary literal");
	}

	long inum = 0;

	while (is_bit(src[len])) {
		inum = push_bit(inum, src[len]);
		len++;
	}

	if (!is_delim(src + len)) {
		return parse_error_token(file,
		                         src,
		                         len + get_word_len(src + len),
		                         src + len,
		                         1,
		                         "Expected 0|1 in binary literal");
	} else if (negative) {
		return tag_int(file, src, len, -inum);
	}
	return tag_int(file, src, len, inum);
}

static bool is_hex(const char *src)
{
	src = skip_sign(src);
	return src[0] == '0' && (src[1] == 'x' || src[2] == 'X');
}

static bool is_nibble(char c)
{
	return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F') || isdigit(c);
}

static int get_nibble(char c)
{
	if (c >= 'a' && c <= 'f') {
		return c - 'a' + 10;
	} else if (c >= 'A' && c <= 'F') {
		return c - 'A' + 10;
	}
	return c - '0';
}

static long push_nibble(long inum, char c)
{
	return (inum << 4) | get_nibble(c);
}

static Weft_ParseToken parse_hex(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("0x");
	bool negative = parse_sign(&len, src);

	if (!src[len]) {
		return parse_error_token(file,
		                         src,
		                         len,
		                         src,
		                         len,
		                         "Expected 0-9|a-f|A-F in hexadecimal literal");
	}

	long inum = 0;

	while (is_nibble(src[len])) {
		inum = push_nibble(inum, src[len]);
		len++;
	}

	if (!is_delim(src + len)) {
		return parse_error_token(file,
		                         src,
		                         len + get_word_len(src + len),
		                         src + len,
		                         1,
		                         "Expected 0-9|a-f|A-F in hexadecimal literal");
	} else if (negative) {
		return tag_int(file, src, len, -inum);
	}
	return tag_int(file, src, len, inum);
}

static const char *skip_dot(const char *src)
{
	return (src[0] == '.') ? src + len_of(".") : src;
}

static bool is_decimal(const char *src)
{
	src = skip_sign(src);
	src = skip_dot(src);
	return isdigit(*src);
}

static int get_digit(char c)
{
	return c - '0';
}

static long push_digit(long inum, char c)
{
	return (10 * inum) + get_digit(c);
}

static Weft_ParseToken parse_decimal(Weft_ParseFile *file, const char *src)
{
	size_t len = 0;
	bool negative = parse_sign(&len, src);

	long inum = 0;
	long right = 0;
	bool dot = false;
	unsigned place = 0;

	while (src[len] == '.' || isdigit(src[len])) {
		if (src[len] == '.') {
			if (dot) {
				return parse_error_token(
					file,
					src,
					len + get_word_len(src + len),
					src + len,
					len_of("."),
					"Expected only one . in number literal");
			}
			dot = true;
		} else if (dot) {
			right = push_digit(right, src[len]);
			place++;
		} else {
			inum = push_digit(inum, src[len]);
		}
		len++;
	}

	if (!is_delim(src + len)) {
		return parse_error_token(file,
		                         src,
		                         len + get_word_len(src + len),
		                         src + len,
		                         1,
		                         "Expected 0-9|. in number literal");
	} else if (!dot) {
		if (negative) {
			return tag_int(file, src, len, -inum);
		}
		return tag_int(file, src, len, inum);
	}

	double fnum = (double)right;
	while (place) {
		fnum /= 10.0;
		place--;
	}
	fnum += (double)inum;

	if (negative) {
		return tag_float(file, src, len, -fnum);
	}
	return tag_float(file, src, len, fnum);
}

static bool is_char(const char *src)
{
	return src[0] == '\'';
}

static bool is_char_esc(const char *src)
{
	return src[0] == '\\';
}

static bool is_hex_esc(const char *src)
{
	switch (src[len_of("\\")]) {
	case 'x':
	case 'X':
	case 'u':
	case 'U':
		return true;
	default:
		return false;
	}
}

static int get_max_nibbles(const char *src)
{
	switch (src[len_of("\\")]) {
	case 'x':
	case 'X':
		return 2;
	case 'u':
	case 'U':
		return 6;
	default:
		return 0;
	}
}

static Weft_ParseToken parse_hex_esc(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("\\");
	if (!src[len]) {
		return parse_error_token(file,
		                         src,
		                         len,
		                         src,
		                         len,
		                         "Unexpected end of file in character escape");
	} else if (!is_nibble(src[len])) {
		return parse_error_token(
			file,
			src,
			len,
			src + len,
			1,
			"Expected 0-9|a-f|A-F in hex character escape");
	}
	int max_nibbles = get_max_nibbles(src);

	long inum = get_nibble(src[len]);
	len++;

	for (int i = 1; i < max_nibbles; i++) {
		if (is_nibble(src[len])) {
			inum = push_nibble(inum, src[len]);
			len++;
		}
	}

	if (inum > WEFT_CHAR_MAX) {
		return parse_error_token(
			file,
			src,
			len,
			src,
			len,
			"Value of character escape %li exceeds max character value %u",
			inum,
			WEFT_CHAR_MAX);
	}
	return tag_char(file, src, len, inum);
}

static bool is_dec_esc(const char *src)
{
	return isdigit(src[len_of("\\")]);
}

static Weft_ParseToken parse_dec_esc(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("\\");
	long inum = get_digit(src[len]);

	for (int i = 1; i < len_of("000"); i++) {
		if (isdigit(src[len])) {
			inum = push_digit(inum, src[len]);
			len++;
		}
	}

	if (inum > UCHAR_MAX) {
		return parse_error_token(
			file,
			src,
			len,
			src,
			len,
			"Character escape value %li exceeds maximum byte value %u",
			inum,
			UCHAR_MAX);
	}
	return tag_char(file, src, len, inum);
}

static Weft_ParseToken parse_char_esc(Weft_ParseFile *file, const char *src)
{
	uint32_t cnum;
	switch (src[len_of("\\")]) {
	case 'a':
		cnum = '\a';
		break;
	case 'b':
		cnum = '\b';
		break;
	case 'e':
		cnum = '\e';
		break;
	case 'f':
		cnum = '\f';
		break;
	case 'n':
		cnum = '\n';
		break;
	case 'r':
		cnum = '\r';
		break;
	case 't':
		cnum = '\t';
		break;
	case 'v':
		cnum = '\v';
		break;
	default:
		cnum = src[len_of("\\")];
		break;
	}
	return tag_char(file, src, len_of("\\a"), cnum);
}

static Weft_ParseToken
utf8_error(Weft_ParseFile *file, const char *src, unsigned width)
{
	switch (width) {
	case 1:
		return parse_error_token(
			file, src, 1, src, 0, "Invalid UTF-8 sequence: \\x%02x", src[0]);
	case 2:
		return parse_error_token(file,
		                         src,
		                         2,
		                         src,
		                         0,
		                         "Invalid UTF-8 sequence: \\x%02x\\x%02x",
		                         src[0],
		                         src[1]);
	case 3:
		return parse_error_token(
			file,
			src,
			3,
			src,
			0,
			"Invalid UTF-8 sequence: \\x%02x\\x%02x\\x%02x",
			src[0],
			src[1],
			src[2]);
	case 4:
		return parse_error_token(
			file,
			src,
			4,
			src,
			0,
			"Invalid UTF-8 sequence: \\x%02x\\x%02x\\x%02x\\x%02x",
			src[0],
			src[1],
			src[2],
			src[3]);
	default:
		return parse_error_token(
			file, src, 1, src, 0, "Invalid UTF-8 sequence");
	}
}

static Weft_ParseToken parse_utf8(Weft_ParseFile *file, const char *src)
{
	int width = char_get_utf8_width(src);
	if (width < 0) {
		return utf8_error(file, src, -width);
	}
	return tag_char(file, src, width, char_read_n(src, width));
}

static Weft_ParseToken parse_char_bare(Weft_ParseFile *file, const char *src)
{
	if (is_char_esc(src)) {
		if (is_hex_esc(src)) {
			return parse_hex_esc(file, src);
		} else if (is_dec_esc(src)) {
			return parse_dec_esc(file, src);
		}
		return parse_char_esc(file, src);
	}
	return parse_utf8(file, src);
}

static size_t get_char_len(const char *src)
{
	size_t len = 0;
	while (src[len] != '\'') {
		if (src[len] == '\\') {
			len++;
		}

		if (!src[len]) {
			return len;
		}
		len++;
	}
	return len + len_of("\\");
}

static Weft_ParseToken parse_char(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("'");
	if (!src[len]) {
		return parse_error_token(file,
		                         src,
		                         len,
		                         src,
		                         len,
		                         "Unexpected end of file in character literal");
	}

	Weft_ParseToken bare = parse_char_bare(file, src + len);
	len += bare.len;

	if (!src[len]) {
		return parse_error_token(file,
		                         src,
		                         len,
		                         src,
		                         len,
		                         "Unexpected end of file in character literal");
	} else if (src[len] != '\'') {
		return parse_error_token(file,
		                         src,
		                         len + get_char_len(src + len),
		                         src + len,
		                         1,
		                         "Expected ' after character literal");
	}
	len += len_of("'");

	bare.src = src;
	bare.len = len;
	return bare;
}

static bool is_str(const char *src)
{
	return src[0] == '"';
}

static Weft_Buf *push_char(Weft_Buf *buf, uint32_t cnum)
{
	char utf8[5] = {0};
	char_write(utf8, cnum);

	return buf_push(buf, utf8, strlen(utf8));
}

static Weft_Str *create_str_from_buf(Weft_Buf *buf)
{
	Weft_Str *str = new_str_n(buf_peek(buf, buf_get_at(buf)), buf_get_at(buf));
	free(buf);

	return str;
}

static Weft_ParseToken parse_str(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("\"");
	Weft_Buf *buf = new_buf(sizeof(char));

	while (src[len] != '"') {
		if (!src[len]) {
			return parse_error_token(
				file,
				src,
				len,
				src,
				len,
				"Unexpected end of file in string literal");
		}

		Weft_ParseToken bare = parse_char_bare(file, src + len);
		len += bare.len;

		if (bare.type == WEFT_PARSE_CHAR) {
			buf = push_char(buf, bare.cnum);
		}
	}
	len += len_of("\"");

	return tag_str(file, src, len, create_str_from_buf(buf));
}

static bool is_shuffle_open(const char *src)
{
	return src[0] == '{';
}

static bool is_shuffle_pivot(const char *src)
{
	return src[0] == '-' && src[1] == '-';
}

static bool is_shuffle_close(const char *src)
{
	return src[0] == '}';
}

static bool is_shuffle_delim(const char *src)
{
	return is_delim(src) || is_shuffle_pivot(src);
}

static size_t get_shuffle_member_len(const char *src)
{
	size_t len = 1;
	while (!is_shuffle_delim(src + len)) {
		len++;
	}
	return len;
}

static Weft_ParseToken parse_shuffle_member(Weft_ParseFile *file,
                                            const char *src)
{
	if (strchr(shuffle_error_list, src[0])) {
		return parse_error_token(
			file, src, 1, src, 1, "Invalid character in shuffle diagram");
	}

	return tag_token(
		file, src, get_shuffle_member_len(src), WEFT_PARSE_SHUFFLE);
}

static bool is_same_member(const Weft_ParseToken in, const Weft_ParseToken out)
{
	if (in.len != out.len) {
		return false;
	}

	for (size_t i = 0; i < in.len; i++) {
		if (in.src[i] != out.src[i]) {
			return false;
		}
	}
	return true;
}

static long get_shuffle_member_index(const Weft_Buf *in_buf,
                                     const Weft_ParseToken token)
{
	const Weft_ParseToken *member = buf_peek(in_buf, buf_get_at(in_buf));
	size_t in_count = buf_get_at(in_buf) / sizeof(Weft_ParseToken);

	for (long i = 0; i < in_count; i++) {
		if (is_same_member(member[i], token)) {
			return i;
		}
	}
	return -1;
}

static Weft_Shuffle *create_shuffle(Weft_Buf *in_buf, Weft_Buf *out_buf)
{
	unsigned in_count = buf_get_at(in_buf) / sizeof(Weft_ParseToken);
	free(in_buf);

	unsigned out_count = buf_get_at(out_buf) / sizeof(unsigned);
	const unsigned *out = buf_peek(out_buf, buf_get_at(out_buf));

	Weft_Shuffle *shuffle = new_shuffle(in_count, out_count);
	for (unsigned i = 0; i < out_count; i++) {
		shuffle_set_out(shuffle, i, out[i]);
	}

	return shuffle;
}

static Weft_ParseToken parse_shuffle(Weft_ParseFile *file, const char *src)
{
	size_t len = len_of("{");
	len += parse_empty(file, src + len).len;

	Weft_Buf *in_buf = new_buf(sizeof(Weft_ParseToken));

	while (!is_shuffle_pivot(src + len)) {
		if (!src[len]) {
			free(in_buf);
			return parse_error_token(
				file,
				src,
				len,
				src,
				len,
				"Unexpected end of file in shuffle diagram");
		}

		Weft_ParseToken member = parse_shuffle_member(file, src + len);
		if (member.type == WEFT_PARSE_SHUFFLE) {
			in_buf = buf_push(in_buf, &member, sizeof(Weft_ParseToken));
		}

		len += member.len;
		len += parse_empty(file, src + len).len;
	}

	len += len_of("--");
	len += parse_empty(file, src + len).len;

	Weft_Buf *out_buf = new_buf(sizeof(unsigned));

	while (!is_shuffle_close(src + len)) {
		if (!src[len]) {
			free(in_buf);
			free(out_buf);
			return parse_error_token(
				file,
				src,
				len,
				src,
				len,
				"Unexpected end of file in shuffle diagram");
		}

		Weft_ParseToken member = parse_shuffle_member(file, src + len);
		if (member.type == WEFT_PARSE_SHUFFLE) {
			long index = get_shuffle_member_index(in_buf, member);
			if (index >= 0) {
				unsigned index_u = index;
				out_buf = buf_push(out_buf, &index_u, sizeof(unsigned));
			}
		}

		len += member.len;
		len += parse_empty(file, src + len).len;
	}
	len += len_of("}");

	return tag_shuffle(file, src, len, create_shuffle(in_buf, out_buf));
}

static Weft_ParseToken parse_word(Weft_ParseFile *file, const char *src)
{
	return tag_token(file, src, get_word_len(src), WEFT_PARSE_WORD);
}

Weft_ParseToken parse_token(Weft_ParseFile *file, const char *src)
{
	if (isspace(src[0])) {
		return parse_indent(file, src);
	} else if (is_op(src)) {
		return parse_op(file, src);
	} else if (is_binary(src)) {
		return parse_binary(file, src);
	} else if (is_hex(src)) {
		return parse_hex(file, src);
	} else if (is_decimal(src)) {
		return parse_decimal(file, src);
	} else if (is_char(src)) {
		return parse_char(file, src);
	} else if (is_str(src)) {
		return parse_str(file, src);
	} else if (is_shuffle_open(src)) {
		return parse_shuffle(file, src);
	}
	return parse_word(file, src);
}

void parse_init(Weft_ParseState *P)
{
	P->file = NULL;
	P->src = NULL;

	P->indent = 0;
	P->indent_stack = new_buf(sizeof(size_t));

	P->token_stack = new_buf(sizeof(Weft_ParseToken));
	P->base = 0;
	P->base_stack = new_buf(sizeof(size_t));

	P->list = NULL;
	P->list_stack = new_buf(sizeof(Weft_ParseList *));
	P->node = NULL;
	P->node_stack = new_buf(sizeof(Weft_ParseList *));
}

void parse_exit(Weft_ParseState *P)
{
	P->file = NULL;
	P->src = NULL;

	P->indent = 0;
	P->indent_stack = buf_free(P->indent_stack);

	P->token_stack = buf_free(P->token_stack);
	P->base = 0;
	P->base_stack = buf_free(P->base_stack);

	P->list = NULL;
	P->list_stack = buf_free(P->list_stack);
	P->node = NULL;
	P->node_stack = buf_free(P->node_stack);
}

static void debug_indent_stack(const Weft_ParseState *P)
{
	printf("\nIndent Stack:");

	const size_t *indent =
		buf_peek(P->indent_stack, buf_get_at(P->indent_stack));
	size_t len = buf_get_at(P->indent_stack) / sizeof(size_t);

	for (size_t i = 0; i < len; i++) {
		printf(" %zu", indent[i]);
	}
}

static void debug_token_stack(const Weft_ParseState *P)
{
	printf("\nToken Stack: ");

	const Weft_ParseToken *token =
		buf_peek(P->token_stack, buf_get_at(P->token_stack));
	size_t len = buf_get_at(P->token_stack) / sizeof(Weft_ParseToken);
	size_t base = P->base / sizeof(Weft_ParseToken);

	for (size_t i = 0; i < base; i++) {
		parse_token_print(token[i]);
		printf(" ");
	}
	printf("|");
	for (size_t i = base; i < len; i++) {
		printf(" ");
		parse_token_print(token[i]);
	}
}

static void debug_list(Weft_ParseState *P)
{
	printf("\nList: ");
	parse_list_print(P->list);
}

void parse_debug(Weft_ParseState *P)
{
	if (P->file->path) {
		printf("\nFile: %s", P->file->path);
	}

	printf("\nIndent: %zu", P->indent);
	debug_indent_stack(P);

	debug_token_stack(P);
	debug_list(P);

	printf("\n");
}

static void push_indent(Weft_ParseState *P)
{
	P->indent_stack = buf_push(P->indent_stack, &P->indent, sizeof(size_t));
}

static void pop_indent(Weft_ParseState *P)
{
	P->indent_stack = buf_drop(P->indent_stack, sizeof(size_t));
}

static void push_token(Weft_ParseState *P, Weft_ParseToken token)
{
	P->token_stack = buf_push(P->token_stack, &token, sizeof(Weft_ParseToken));
}

static Weft_ParseToken pop_token(Weft_ParseState *P)
{
	Weft_ParseToken token;
	P->token_stack = buf_pop(&token, P->token_stack, sizeof(Weft_ParseToken));

	return token;
}

static void push_base(Weft_ParseState *P)
{
	P->base_stack = buf_push(P->base_stack, &P->base, sizeof(size_t));
	P->base = buf_get_at(P->token_stack);
}

static void pop_base(Weft_ParseState *P)
{
	P->base_stack = buf_pop(&P->base, P->base_stack, sizeof(size_t));
}

static void push_list(Weft_ParseState *P)
{
	P->list_stack = buf_push(P->list_stack, &P->list, sizeof(Weft_ParseList *));
	P->list = NULL;

	P->node_stack = buf_push(P->node_stack, &P->node, sizeof(Weft_ParseList *));
	P->node = NULL;
}

static Weft_ParseList *pop_list(Weft_ParseState *P)
{
	Weft_ParseList *list = P->list;
	P->list_stack = buf_pop(&P->list, P->list_stack, sizeof(Weft_ParseList *));
	P->node_stack = buf_pop(&P->node, P->node_stack, sizeof(Weft_ParseList *));

	return list;
}

static void output_token(Weft_ParseState *P, Weft_ParseToken token)
{
	if (P->list) {
		P->node->cdr = new_parse_list_node(token, NULL);
		P->node = P->node->cdr;
	} else {
		P->list = new_parse_list_node(token, NULL);
		P->node = P->list;
	}
}

static bool is_token_stack_empty(const Weft_ParseState *P)
{
	return buf_get_at(P->token_stack) == P->base;
}

static void flush_token_stack(Weft_ParseState *P)
{
	if (is_token_stack_empty(P)) {
		return;
	}
	output_token(P, pop_token(P));
}

static bool is_dedent(const Weft_ParseState *P, size_t indent)
{
	if (!buf_get_at(P->indent_stack) || !buf_get_at(P->token_stack)) {
		return false;
	}

	const Weft_ParseToken *token =
		buf_peek(P->token_stack, sizeof(Weft_ParseToken));
	if (token->type == WEFT_PARSE_OP) {
		return false;
	}

	const size_t *prev = buf_peek(P->indent_stack, sizeof(size_t));
	return indent <= *prev;
}

static void handle_dedent(Weft_ParseState *P)
{
	flush_token_stack(P);
	pop_indent(P);
	pop_base(P);

	Weft_ParseToken head = pop_token(P);
	Weft_ParseList *body = pop_list(P);

	return output_token(P, tag_block(new_parse_block(head, body)));
}

static void handle_indent(Weft_ParseState *P, size_t indent)
{
	while (is_dedent(P, indent)) {
		handle_dedent(P);
	}
	P->indent = indent;
}

static void handle_list_open(Weft_ParseState *P, Weft_ParseToken token)
{
	flush_token_stack(P);
	push_token(P, token);
	push_base(P);
	push_list(P);
}

static void handle_list_close(Weft_ParseState *P, Weft_ParseToken token)
{
	flush_token_stack(P);

	while (is_dedent(P, 0)) {
		handle_dedent(P);
	}

	pop_base(P);
	Weft_ParseToken open = pop_token(P);
	Weft_ParseList *list = pop_list(P);

	return output_token(
		P,
		tag_list(open.file, open.src, token.src + token.len - open.src, list));
}

static void handle_def(Weft_ParseState *P, Weft_ParseToken token)
{
	if (is_token_stack_empty(P)) {
		return parse_error(
			token.file, token.src, token.len, "Expected identifier before :");
	}

	push_indent(P);
	push_base(P);
	push_list(P);
}

static void handle_end(Weft_ParseState *P, Weft_ParseToken token)
{
	if (is_dedent(P, 0)) {
		handle_dedent(P);
	}
}

static void handle_op(Weft_ParseState *P, Weft_ParseToken token)
{
	switch (token.src[0]) {
	case '[':
		return handle_list_open(P, token);
	case ']':
		return handle_list_close(P, token);
	case ':':
		return handle_def(P, token);
	case ';':
		return handle_end(P, token);
	default:
		break;
	}
}

static void handle_word(Weft_ParseState *P, Weft_ParseToken token)
{
	flush_token_stack(P);
	push_token(P, token);
}

static void skip_token(Weft_ParseState *P, const Weft_ParseToken token)
{
	P->src += token.len;
	P->src += parse_empty(P->file, P->src).len;
}

static void handle_token(Weft_ParseState *P, Weft_ParseToken token)
{
	skip_token(P, token);

	switch (token.type) {
	case WEFT_PARSE_ERROR:
	case WEFT_PARSE_EMPTY:
		break;
	case WEFT_PARSE_INDENT:
		return handle_indent(P, token.indent);
	case WEFT_PARSE_OP:
		return handle_op(P, token);
	case WEFT_PARSE_WORD:
		return handle_word(P, token);
	default:
		return output_token(P, token);
	}
}

Weft_ParseList *parse(Weft_ParseState *P, Weft_ParseFile *file)
{
	P->file = file;
	P->src = file->src;

	if (is_line_empty(P->src)) {
		P->src += parse_empty(P->file, P->src).len;
	}
	handle_token(P, parse_indent(P->file, P->src));

	while (*P->src) {
		handle_token(P, parse_token(P->file, P->src));
	}

	flush_token_stack(P);

	while (is_dedent(P, 0)) {
		handle_dedent(P);
	}

	return P->list;
}
