#include "compile.h"
#include "eval.h"
#include "list.h"
#include "parse.h"

#include <stdio.h>

int main(int argc, char **args)
{
	if (argc < 2) {
		return 0;
	}

	Weft_ParseFile *file = parse_file_load(args[1]);
	if (!file) {
		return 0;
	}

	Weft_ParseState P;
	parse_init(&P);
	Weft_ParseList *pl = parse(&P, file);
	parse_exit(&P);

	Weft_CompileState C;
	compile_init(&C);
	Weft_List *ctrl = compile(&C, pl);
	compile_exit(&C);

	Weft_EvalState W;
	eval_init(&W);
	eval(&W, ctrl);
	eval_exit(&W);

	return 0;
}
