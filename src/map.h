#ifndef WEFT_MAP_H
#define WEFT_MAP_H

#include <stddef.h>
#include <stdint.h>

// Forward Declarations

typedef struct weft_builtin Weft_Builtin;
typedef struct weft_fn Weft_Fn;
typedef struct weft_map_key Weft_MapKey;
typedef struct weft_map Weft_Map;

// Local Includes

#include "data.h"

// Data Types

struct weft_map_key {
	Weft_Map *map;
	uintptr_t value;
};

struct weft_map {
	Weft_MapKey *key;
	Weft_Map *left;
	Weft_Map *right;
};

// Functions

Weft_MapKey *new_map_key_builtin(Weft_Map *map, Weft_Builtin *builtin);
Weft_MapKey *new_map_key_fn(Weft_Map *map, Weft_Fn *fn);
Weft_Data map_key_get_data(Weft_MapKey *key);
Weft_MapKey *map_lookup_n(Weft_Map *map, const char *src, size_t len);
Weft_Map *map_insert(Weft_Map *map, Weft_MapKey *key);

#endif
