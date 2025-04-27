#include "map.h"
#include "builtin.h"
#include "fn.h"
#include "gc.h"

#include <stdbool.h>

static bool is_value_builtin(uintptr_t value)
{
	return value & 1;
}

static void *get_value_ptr(uintptr_t value)
{
	return (void *)(value >> 1);
}

static const char *get_key_name(const Weft_MapKey *key)
{
	if (is_value_builtin(key->value)) {
		Weft_Builtin *builtin = get_value_ptr(key->value);
		return builtin->name;
	}

	Weft_Fn *fn = get_value_ptr(key->value);
	return fn->name;
}

static int compare_key_n(const Weft_MapKey *key, const char *src, size_t len)
{
	const char *name = get_key_name(key);
	for (size_t i = 0; i < len; i++) {
		if (name[i] > src[i]) {
			return -1;
		} else if (name[i] < src[i]) {
			return 1;
		}
	}

	if (name[len]) {
		return -1;
	}
	return 0;
}

static Weft_MapKey *new_map_key(Weft_Map *map, uintptr_t value)
{
	Weft_MapKey *key = gc_alloc(sizeof(Weft_MapKey));
	key->map = map;
	key->value = value;

	return key;
}

Weft_MapKey *new_map_key_builtin(Weft_Map *map, Weft_Builtin *builtin)
{
	return new_map_key(map, ((uintptr_t)builtin << 1) | 1);
}

Weft_MapKey *new_map_key_fn(Weft_Map *map, Weft_Fn *fn)
{
	return new_map_key(map, (uintptr_t)fn << 1);
}

Weft_Data map_key_get_data(Weft_MapKey *key)
{
	if (is_value_builtin(key->value)) {
		return data_builtin(get_value_ptr(key->value));
	}
	return data_fn(get_value_ptr(key->value));
}

Weft_MapKey *map_lookup_n(Weft_Map *map, const char *src, size_t len)
{
	while (map) {
		int cmp = compare_key_n(map->key, src, len);
		if (cmp < 0) {
			map = map->left;
		} else if (cmp > 0) {
			map = map->right;
		} else {
			return map->key;
		}
	}
	return NULL;
}

static int compare_key(const Weft_MapKey *left, const Weft_MapKey *right)
{
	if (left == right) {
		return 0;
	}

	const char *left_name = get_key_name(left);
	const char *right_name = get_key_name(right);

	size_t i;
	for (i = 0; left_name[i]; i++) {
		if (left_name[i] > right_name[i]) {
			return -1;
		} else if (left_name[i] < right_name[i]) {
			return 1;
		}
	}

	if (right_name[i]) {
		return 1;
	}
	return 0;
}

static Weft_Map *new_node(Weft_MapKey *key, Weft_Map *left, Weft_Map *right)
{
	Weft_Map *node = gc_alloc(sizeof(Weft_Map));
	node->key = key;
	node->left = left;
	node->right = right;

	return node;
}

static Weft_Map *leaf_node(Weft_MapKey *key)
{
	return new_node(key, NULL, NULL);
}

static Weft_Map *clone_node(Weft_Map *node)
{
	return new_node(node->key, node->left, node->right);
}

Weft_Map *map_insert(Weft_Map *map, Weft_MapKey *key)
{
	if (!map) {
		return leaf_node(key);
	}

	map = clone_node(map);
	Weft_Map *node = map;

	while (true) {
		int cmp = compare_key(node->key, key);
		if (cmp < 0) {
			if (!node->left) {
				node->left = leaf_node(key);
				return map;
			}

			node->left = clone_node(node->left);
			node = node->left;
		} else if (cmp > 0) {
			if (!node->right) {
				node->right = leaf_node(key);
				return map;
			}

			node->right = clone_node(node->right);
			node = node->right;
		} else {
			node->key = key;
			return map;
		}
	}
}
