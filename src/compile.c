#include "compile.h"
#include "buf.h"
#include "data.h"
#include "fn.h"
#include "list.h"
#include "map.h"
#include "parse.h"

#include <stddef.h>

void compile_init(Weft_CompileState *C)
{
	C->map = NULL;

	C->list = NULL;
	C->list_stack = new_buf(sizeof(Weft_List *));
	C->node = NULL;
	C->node_stack = new_buf(sizeof(Weft_List *));
}

void compile_exit(Weft_CompileState *C)
{
	C->map = NULL;

	C->list = NULL;
	C->list_stack = buf_free(C->list_stack);
	C->node = NULL;
	C->node_stack = buf_free(C->node_stack);
}

static void output_data(Weft_CompileState *C, Weft_Data data)
{
	if (C->list) {
		C->node->cdr = new_list_node(data, NULL);
		C->node = C->node->cdr;
	} else {
		C->list = new_list_node(data, NULL);
		C->node = C->list;
	}
}

static Weft_Data strip_token(Weft_ParseToken token)
{
	switch (token.type) {
	case WEFT_PARSE_INT:
		return data_int(token.inum);
	case WEFT_PARSE_FLOAT:
		return data_float(token.fnum);
	case WEFT_PARSE_CHAR:
		return data_char(token.cnum);
	case WEFT_PARSE_STR:
		return data_str(token.ptr);
	case WEFT_PARSE_SHUFFLE:
		return data_shuffle(token.ptr);
	default:
		return data_nil();
	}
}

static void handle_block(Weft_CompileState *C, Weft_ParseBlock *block)
{
	Weft_CompileState temp;
	compile_init(&temp);
	temp.map = C->map;

	Weft_List *body = compile(&temp, block->body);
	Weft_MapKey *key = new_map_key_fn(
		temp.map, new_fn_n(block->head.src, block->head.len, body));
	C->map = map_insert(C->map, key);

	compile_exit(&temp);
}

static void handle_lookup(Weft_CompileState *C, Weft_ParseToken token)
{
	Weft_MapKey *key = map_lookup_n(C->map, token.src, token.len);
	if (!key) {
		return parse_error(token.file,
		                   token.src,
		                   token.len,
		                   "%.*s is undefined",
		                   (unsigned)token.len,
		                   token.src);
	}
	return output_data(C, map_key_get_data(key));
}

Weft_List *compile(Weft_CompileState *C, Weft_ParseList *src)
{
	do {
		while (src) {
			Weft_ParseToken token = parse_list_pop(&src);
			switch (token.type) {
			case WEFT_PARSE_WORD:
				handle_lookup(C, token);
				break;
			case WEFT_PARSE_LIST:
				C->list_stack =
					buf_push(C->list_stack, &C->list, sizeof(Weft_List *));
				C->list = NULL;
				C->node_stack =
					buf_push(C->node_stack, &C->node, sizeof(Weft_List *));
				C->node = NULL;
				break;
			case WEFT_PARSE_BLOCK:
				handle_block(C, token.ptr);
				break;
			default:
				output_data(C, strip_token(token));
				break;
			}
		}

		while (buf_get_at(C->list_stack) && !src) {
			Weft_List *list = C->list;
			C->list_stack =
				buf_pop(&C->list, C->list_stack, sizeof(Weft_List *));
			C->node_stack =
				buf_pop(&C->node, C->node_stack, sizeof(Weft_List *));
			output_data(C, data_list(list));
		}
	} while (src);

	return C->list;
}
