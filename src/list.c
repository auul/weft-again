#include "list.h"
#include "gc.h"

#include <stdio.h>

Weft_List *new_list_node(Weft_Data car, Weft_List *cdr)
{
	Weft_List *node = gc_alloc(sizeof(Weft_List));
	node->car = car;
	node->cdr = cdr;

	return node;
}

void list_print(const Weft_List *list)
{
	printf("[");
	list_print_bare(list);
	printf("]");
}

void list_print_bare(const Weft_List *list)
{
	if (!list) {
		return;
	}

	data_print(list->car);
	list = list->cdr;

	while (list) {
		printf(" ");
		data_print(list->car);
		list = list->cdr;
	}
}

Weft_Data list_pop(Weft_List **list_p)
{
	Weft_List *list = *list_p;
	if (!list) {
		return data_nil();
	}
	*list_p = list->cdr;

	return list->car;
}
