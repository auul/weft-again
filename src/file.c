#include "file.h"
#include "gc.h"

#include <errno.h>
#include <string.h>

FILE *file_open(const char *path, const char *mode)
{
	FILE *f = fopen(path, mode);
	if (!f) {
		fprintf(stderr,
		        "Failed to open file '%s' for %s: %s\n",
		        path,
		        (mode[0] == 'r') ? "reading" : "writing",
		        strerror(errno));
	}
	return f;
}

static char *copy_str_n(const char *src, size_t len)
{
	char *dest = gc_alloc(len + 1);
	memcpy(dest, src, len);
	dest[len] = 0;

	return dest;
}

FILE *file_open_n(const char *path, size_t path_len, const char *mode)
{
	return file_open(copy_str_n(path, path_len), mode);
}

void file_close(FILE *f)
{
	if (!f) {
		return;
	}
	fclose(f);
}

size_t file_len(FILE *f)
{
	size_t at = ftell(f);
	fseek(f, 0, SEEK_END);
	size_t len = ftell(f);
	fseek(f, at, SEEK_SET);

	return len;
}
