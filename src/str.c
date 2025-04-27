#include "str.h"
#include "char.h"
#include "gc.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

Weft_Str *new_str_n(const char *src, size_t len)
{
	Weft_Str *str = gc_alloc(sizeof(Weft_Str) + len + 1);
	memcpy(str->ch, src, len);
	str->ch[len] = 0;

	return str;
}

void str_print(const Weft_Str *str)
{
	printf("\"");
	str_print_bare(str);
	printf("\"");
}

static bool is_span_char(char c)
{
	if (c == '"') {
		return false;
	}
	return c == ' ' || char_is_utf8(c) || (isgraph(c) && !isspace(c));
}

static unsigned get_print_span(const char *ch)
{
	unsigned span = 0;
	while (is_span_char(ch[span])) {
		span++;
	}
	return span;
}

void str_print_bare(const Weft_Str *str)
{
	const char *ch = str->ch;
	while (*ch) {
		unsigned span = get_print_span(ch);
		if (span) {
			printf("%.*s", span, ch);
			ch += span;
		} else if (*ch == '"') {
			printf("\\\"");
			ch++;
		} else {
			char_print_esc(*ch);
			ch++;
		}
	}
}
