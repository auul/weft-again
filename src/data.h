#ifndef WEFT_DATA_H
#define WEFT_DATA_H

#include <stdint.h>

// Forward Declarations

typedef struct weft_str Weft_Str;
typedef struct weft_shuffle Weft_Shuffle;
typedef struct weft_list Weft_List;
typedef struct weft_builtin Weft_Builtin;
typedef struct weft_fn Weft_Fn;
typedef enum weft_data_type Weft_DataType;
typedef struct weft_data Weft_Data;

// Data Types

enum weft_data_type {
	WEFT_DATA_NIL,
	WEFT_DATA_INT,
	WEFT_DATA_FLOAT,
	WEFT_DATA_CHAR,
	WEFT_DATA_STR,
	WEFT_DATA_SHUFFLE,
	WEFT_DATA_LIST,
	WEFT_DATA_BUILTIN,
	WEFT_DATA_FN,
};

struct weft_data {
	Weft_DataType type;
	union {
		void *ptr;
		long inum;
		double fnum;
		uint32_t cnum;
	};
};

// Functions

Weft_Data data_nil(void);
Weft_Data data_int(long inum);
Weft_Data data_float(double fnum);
Weft_Data data_char(uint32_t cnum);
Weft_Data data_str(Weft_Str *str);
Weft_Data data_shuffle(Weft_Shuffle *shuffle);
Weft_Data data_list(Weft_List *list);
Weft_Data data_builtin(Weft_Builtin *builtin);
Weft_Data data_fn(Weft_Fn *fn);
void data_print(const Weft_Data data);

#endif
