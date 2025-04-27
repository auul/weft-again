#include "fn.h"
#include "gc.h"

#include <stdio.h>
#include <string.h>

Weft_Fn *new_fn_n(const char *name, size_t name_len, Weft_List *list)
{
	Weft_Fn *fn = gc_alloc(sizeof(Weft_Fn) + name_len + 1);
	memcpy(fn->name, name, name_len);
	fn->name[name_len] = 0;
	fn->list = list;

	return fn;
}

void fn_print(const Weft_Fn *fn)
{
	printf("%s", fn->name);
}
