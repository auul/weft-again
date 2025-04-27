#include "char.h"

#include <ctype.h>
#include <stdio.h>

// Constants

static const uint8_t UTF8_XBYTE = 128;
static const uint8_t UTF8_2BYTE = 192;
static const uint8_t UTF8_3BYTE = 224;
static const uint8_t UTF8_4BYTE = 240;

static const uint32_t UTF8_XMASK = 192;
static const uint32_t UTF8_2MASK = 224;
static const uint32_t UTF8_3MASK = 240;
static const uint32_t UTF8_4MASK = 248;

static const unsigned UTF8_SHIFT = 6;
static const uint32_t UTF8_1MAX = 127;
static const uint32_t UTF8_2MAX = 2047;
static const uint32_t UTF8_3MAX = 65535;

// Functions

void char_print(uint32_t cnum)
{
	if (cnum == '\'') {
		printf("'\\''");
		return;
	}

	printf("'");
	char_print_esc(cnum);
	printf("'");
}

static void print_utf8(uint32_t cnum)
{
	if (cnum > UTF8_3MAX) {
		printf("%c%c%c%c",
		       (uint8_t)(cnum >> (3 * UTF8_SHIFT)) | UTF8_4BYTE,
		       (uint8_t)((cnum >> (2 * UTF8_SHIFT)) & ~UTF8_XMASK) | UTF8_XBYTE,
		       (uint8_t)((cnum >> UTF8_SHIFT) & ~UTF8_XMASK) | UTF8_XBYTE,
		       (uint8_t)(cnum & ~UTF8_XMASK) | UTF8_XBYTE);
	} else if (cnum > UTF8_2MAX) {
		printf("%c%c%c",
		       (uint8_t)(cnum >> (2 * UTF8_SHIFT)) | UTF8_3BYTE,
		       (uint8_t)((cnum >> UTF8_SHIFT) & ~UTF8_XMASK) | UTF8_XBYTE,
		       (uint8_t)(cnum & ~UTF8_XMASK) | UTF8_XBYTE);
	} else if (cnum > UTF8_1MAX) {
		printf("%c%c",
		       (uint8_t)(cnum >> UTF8_SHIFT) | UTF8_2BYTE,
		       (uint8_t)(cnum & ~UTF8_XMASK) | UTF8_XBYTE);
	} else {
		printf("%c", cnum);
	}
}

void char_print_esc(uint32_t cnum)
{
	switch (cnum) {
	case '\a':
		printf("\\a");
		break;
	case '\b':
		printf("\\b");
		break;
	case '\e':
		printf("\\e");
		break;
	case '\f':
		printf("\\f");
		break;
	case '\n':
		printf("\\n");
		break;
	case '\r':
		printf("\\r");
		break;
	case '\t':
		printf("\\t");
		break;
	case '\v':
		printf("\\v");
		break;
	case '\\':
		printf("\\\\");
		break;
	case ' ':
		printf(" ");
		break;
	default:
		if (cnum > UTF8_1MAX) {
			print_utf8(cnum);
		} else if (isspace(cnum) || !isgraph(cnum)) {
			printf("\\x%02x", cnum);
		} else {
			printf("%c", cnum);
		}
		break;
	}
}

int char_get_utf8_width(const char *src)
{
	if ((src[0] & UTF8_4MASK) == UTF8_4BYTE) {
		if (!src[1]) {
			return -1;
		} else if (!src[2] || (src[1] & UTF8_XMASK) != UTF8_XBYTE) {
			return -2;
		} else if (!src[3] || (src[2] & UTF8_XMASK) != UTF8_XBYTE) {
			return -3;
		} else if ((src[3] & UTF8_XMASK) != UTF8_XBYTE) {
			return -4;
		}

		uint32_t cnum = char_read_n(src, 4);
		if (cnum > WEFT_CHAR_MAX) {
			return -4;
		}
		return 4;
	} else if ((src[0] & UTF8_3MASK) == UTF8_3BYTE) {
		if (!src[1]) {
			return -1;
		} else if (!src[2] || (src[1] & UTF8_XMASK) != UTF8_XBYTE) {
			return -2;
		} else if ((src[2] & UTF8_XMASK) != UTF8_XBYTE) {
			return -3;
		}
		return 3;
	} else if ((src[0] & UTF8_2MASK) == UTF8_2BYTE) {
		if (!src[1]) {
			return -1;
		} else if ((src[1] & UTF8_XMASK) != UTF8_XBYTE) {
			return -2;
		}
		return 2;
	} else if (src[0] & UTF8_XBYTE) {
		return -1;
	}
	return 1;
}

uint32_t char_read_n(const char *src, unsigned width)
{
	switch (width) {
	case 1:
		return src[0];
	case 2:
		return (((uint32_t)src[0] & ~UTF8_2MASK) << UTF8_SHIFT)
		     | ((uint32_t)src[1] & ~UTF8_XMASK);
	case 3:
		return (((uint32_t)src[0] & ~UTF8_3MASK) << (2 * UTF8_SHIFT))
		     | (((uint32_t)src[1] & ~UTF8_XMASK) << UTF8_SHIFT)
		     | ((uint32_t)src[2] & ~UTF8_XMASK);
	case 4:
		return (((uint32_t)src[0] & ~UTF8_4MASK) << (3 * UTF8_SHIFT))
		     | (((uint32_t)src[1] & ~UTF8_XMASK) << (2 * UTF8_SHIFT))
		     | (((uint32_t)src[2] & ~UTF8_XMASK) << UTF8_SHIFT)
		     | ((uint32_t)src[3] & ~UTF8_XMASK);
	default:
		return 0;
	}
}

char *char_write(char *dest, uint32_t cnum)
{
	if (cnum > UTF8_3MAX) {
		dest[0] = (uint8_t)(cnum >> (3 * UTF8_SHIFT)) | UTF8_4BYTE;
		dest[1] =
			(uint8_t)((cnum >> (2 * UTF8_SHIFT)) & ~UTF8_XMASK) | UTF8_XBYTE;
		dest[2] = (uint8_t)((cnum >> UTF8_SHIFT) & ~UTF8_XMASK) | UTF8_XBYTE;
		dest[3] = (uint8_t)(cnum & ~UTF8_XMASK) | UTF8_XBYTE;
		return dest + 4;
	} else if (cnum > UTF8_2MAX) {
		dest[0] = (uint8_t)(cnum >> (2 * UTF8_SHIFT)) | UTF8_3BYTE;
		dest[1] = (uint8_t)((cnum >> UTF8_SHIFT) & ~UTF8_XMASK) | UTF8_XBYTE;
		dest[2] = (uint8_t)(cnum & ~UTF8_XMASK) | UTF8_XBYTE;
		return dest + 3;
	} else if (cnum > UTF8_1MAX) {
		dest[0] = (uint8_t)(cnum >> UTF8_SHIFT) | UTF8_2BYTE;
		dest[1] = (uint8_t)(cnum & ~UTF8_XMASK) | UTF8_XBYTE;
		return dest + 2;
	} else {
		dest[0] = cnum;
		return dest + 1;
	}
}

bool char_is_utf8(char c)
{
	return ((c & UTF8_4MASK) == UTF8_4BYTE) | ((c & UTF8_3MASK) == UTF8_3BYTE)
	    || ((c & UTF8_2MASK) == UTF8_2BYTE) || ((c & UTF8_XMASK) == UTF8_XBYTE);
}
