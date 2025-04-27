#ifndef WEFT_CHAR_H
#define WEFT_CHAR_H

#include <stdbool.h>
#include <stdint.h>

// Constants

static const uint32_t WEFT_CHAR_MAX = 2097151;

// Functions

void char_print(uint32_t cnum);
void char_print_esc(uint32_t cnum);
int char_get_utf8_width(const char *src);
uint32_t char_read_n(const char *src, unsigned width);
char *char_write(char *dest, uint32_t cnum);
bool char_is_utf8(char c);

#endif
