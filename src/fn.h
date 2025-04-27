#ifndef WEFT_FN_H
#define WEFT_FN_H

#include <stddef.h>

// Forward Declarations

typedef struct weft_list Weft_List;
typedef struct weft_fn Weft_Fn;

// Data Types

struct weft_fn {
	Weft_List *list;
	char name[];
};

// Functions

Weft_Fn *new_fn_n(const char *name, size_t name_len, Weft_List *list);
void fn_print(const Weft_Fn *fn);

#endif
