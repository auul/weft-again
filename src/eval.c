#include "eval.h"
#include "buf.h"
#include "builtin.h"
#include "data.h"
#include "fn.h"
#include "list.h"

#include <stddef.h>

void eval_init(Weft_EvalState *W)
{
	W->ctrl = NULL;
	W->stack = new_buf(sizeof(Weft_Data));
	W->nest = new_buf(sizeof(Weft_List *));
}

void eval_exit(Weft_EvalState *W)
{
	W->ctrl = NULL;
	W->stack = buf_free(W->stack);
	W->nest = buf_free(W->nest);
}

static bool eval_builtin(Weft_EvalState *W, Weft_Builtin *builtin)
{
	return builtin->fn(W);
}

static void eval_fn(Weft_EvalState *W, Weft_Fn *fn)
{
	W->nest = buf_push(W->nest, &W->ctrl, sizeof(Weft_List *));
	W->ctrl = fn->list;
}

bool eval(Weft_EvalState *W, Weft_List *ctrl)
{
	W->ctrl = ctrl;

	do {
		while (W->ctrl) {
			Weft_Data data = list_pop(&W->ctrl);
			switch (data.type) {
			case WEFT_DATA_BUILTIN:
				if (!eval_builtin(W, data.ptr)) {
					return false;
				}
				break;
			case WEFT_DATA_FN:
				eval_fn(W, data.ptr);
				break;
			default:
				W->stack = buf_push(W->stack, &data, sizeof(Weft_Data));
				break;
			}
		}

		while (buf_get_at(W->nest) && !W->ctrl) {
			W->nest = buf_pop(&W->ctrl, W->nest, sizeof(Weft_List *));
		}
	} while (W->ctrl);

	return true;
}
