#ifndef WEFT_EVAL_H
#define WEFT_EVAL_H

#include <stdbool.h>

// Forward Declarations

typedef struct weft_buf Weft_Buf;
typedef struct weft_list Weft_List;
typedef struct weft_eval_state Weft_EvalState;

// Data Types

struct weft_eval_state {
	Weft_List *ctrl;
	Weft_Buf *stack;
	Weft_Buf *nest;
};

// Functions

void eval_init(Weft_EvalState *W);
void eval_exit(Weft_EvalState *W);
bool eval(Weft_EvalState *W, Weft_List *ctrl);

#endif
