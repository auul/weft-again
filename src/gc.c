#include "gc.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Globals

static Weft_GC *gc_head;

// Functions

static Weft_GC *get_tag(void *ptr)
{
	return (Weft_GC *)ptr - 1;
}

static Weft_GC *get_tag_prev(Weft_GC *tag)
{
	return (Weft_GC *)(tag->prev >> 1);
}

static void set_tag_prev(Weft_GC *tag, Weft_GC *prev)
{
	tag->prev = (uintptr_t)prev << 1;
}

static bool is_tag_marked(const Weft_GC *tag)
{
	return tag->prev & 1;
}

static void mark_tag(Weft_GC *tag)
{
	tag->prev |= 1;
}

static Weft_GC *pop_tag(Weft_GC *tag)
{
	Weft_GC *prev = get_tag_prev(tag);
	free(tag);

	return prev;
}

void *gc_alloc(size_t size)
{
	Weft_GC *tag = malloc(sizeof(Weft_GC) + size);
	if (!tag) {
		fprintf(stderr,
		        "Failed to allocate %zu bytes: %s\n",
		        sizeof(Weft_GC) + size,
		        strerror(errno));
		exit(1);
	}

	set_tag_prev(tag, gc_head);
	gc_head = tag;

	return tag->ptr;
}

bool gc_mark(void *ptr)
{
	if (!ptr) {
		return true;
	}

	Weft_GC *tag = get_tag(ptr);
	if (is_tag_marked(tag)) {
		return true;
	}
	mark_tag(tag);

	return false;
}

void gc_collect(void)
{
	while (gc_head && !is_tag_marked(gc_head)) {
		gc_head = pop_tag(gc_head);
	}

	for (Weft_GC *tag = gc_head; tag; tag = get_tag_prev(tag)) {
		while (get_tag_prev(tag) && !is_tag_marked(get_tag_prev(tag))) {
			set_tag_prev(tag, pop_tag(get_tag_prev(tag)));
		}
	}
}
