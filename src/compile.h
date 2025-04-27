#ifndef WEFT_COMPILE_H
#define WEFT_COMPILE_H

// Forward Declarations

typedef struct weft_buf Weft_Buf;
typedef struct weft_list Weft_List;
typedef struct weft_parse_list Weft_ParseList;
typedef struct weft_map Weft_Map;
typedef struct weft_compile_state Weft_CompileState;

// Data Types

struct weft_compile_state {
	Weft_Map *map;

	Weft_List *list;
	Weft_Buf *list_stack;
	Weft_List *node;
	Weft_Buf *node_stack;
};

// Functions

void compile_init(Weft_CompileState *C);
void compile_exit(Weft_CompileState *C);
Weft_List *compile(Weft_CompileState *C, Weft_ParseList *list);

#endif
