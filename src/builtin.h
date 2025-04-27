#ifndef WEFT_BUILTIN_H
#define WEFT_BUILTIN_H

#include <stdbool.h>
#include <stddef.h>

// Forward Declarations

typedef struct weft_eval_state Weft_EvalState;
typedef struct weft_builtin Weft_Builtin;

// Data Types

struct weft_builtin {
	bool (*fn)(Weft_EvalState *);
	char name[];
};

// Functions

Weft_Builtin *
new_builtin_n(const char *name, size_t name_len, bool (*fn)(Weft_EvalState *));
void builtin_print(const Weft_Builtin *builtin);

#endif
