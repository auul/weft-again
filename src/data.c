#include "data.h"
#include "builtin.h"
#include "char.h"
#include "fn.h"
#include "list.h"
#include "shuffle.h"
#include "str.h"

#include <stdio.h>

static Weft_Data tag_ptr(Weft_DataType type, void *ptr)
{
	Weft_Data data = {
		.type = type,
		.ptr = ptr,
	};
	return data;
}

Weft_Data data_nil(void)
{
	return tag_ptr(WEFT_DATA_NIL, NULL);
}

Weft_Data data_int(long inum)
{
	Weft_Data data = {
		.type = WEFT_DATA_INT,
		.inum = inum,
	};
	return data;
}

Weft_Data data_float(double fnum)
{
	Weft_Data data = {
		.type = WEFT_DATA_FLOAT,
		.fnum = fnum,
	};
	return data;
}

Weft_Data data_char(uint32_t cnum)
{
	Weft_Data data = {
		.type = WEFT_DATA_CHAR,
		.cnum = cnum,
	};
	return data;
}

Weft_Data data_str(Weft_Str *str)
{
	return tag_ptr(WEFT_DATA_STR, str);
}

Weft_Data data_shuffle(Weft_Shuffle *shuffle)
{
	return tag_ptr(WEFT_DATA_SHUFFLE, shuffle);
}

Weft_Data data_list(Weft_List *list)
{
	return tag_ptr(WEFT_DATA_LIST, list);
}

Weft_Data data_builtin(Weft_Builtin *builtin)
{
	return tag_ptr(WEFT_DATA_BUILTIN, builtin);
}

Weft_Data data_fn(Weft_Fn *fn)
{
	return tag_ptr(WEFT_DATA_FN, fn);
}

void data_print(const Weft_Data data)
{
	switch (data.type) {
	case WEFT_DATA_NIL:
		printf("nil");
		break;
	case WEFT_DATA_INT:
		printf("%li", data.inum);
		break;
	case WEFT_DATA_FLOAT:
		printf("%g", data.fnum);
		break;
	case WEFT_DATA_CHAR:
		char_print(data.cnum);
		break;
	case WEFT_DATA_STR:
		str_print(data.ptr);
		break;
	case WEFT_DATA_SHUFFLE:
		shuffle_print(data.ptr);
		break;
	case WEFT_DATA_LIST:
		list_print(data.ptr);
		break;
	case WEFT_DATA_BUILTIN:
		builtin_print(data.ptr);
		break;
	case WEFT_DATA_FN:
		fn_print(data.ptr);
		break;
	default:
		printf("%u:%p", data.type, data.ptr);
		break;
	}
}
