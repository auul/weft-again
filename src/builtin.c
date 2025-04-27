#include "builtin.h"
#include "gc.h"

#include <stdio.h>
#include <string.h>

Weft_Builtin *
new_builtin_n(const char *name, size_t name_len, bool (*fn)(Weft_EvalState *))
{
	Weft_Builtin *builtin = gc_alloc(sizeof(Weft_Builtin) + name_len + 1);
	memcpy(builtin->name, name, name_len);
	builtin->name[name_len] = 0;
	builtin->fn = fn;

	return builtin;
}

void builtin_print(const Weft_Builtin *builtin)
{
	printf("%s", builtin->name);
}
