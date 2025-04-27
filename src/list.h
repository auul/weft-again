#ifndef WEFT_LIST_H
#define WEFT_LIST_H

// Forward Declarations

typedef struct weft_list Weft_List;

// Local Includes

#include "data.h"

// Data Types

struct weft_list {
	Weft_Data car;
	Weft_List *cdr;
};

// Functions

Weft_List *new_list_node(Weft_Data car, Weft_List *cdr);
void list_print(const Weft_List *list);
void list_print_bare(const Weft_List *list);
Weft_Data list_pop(Weft_List **list_p);

#endif
