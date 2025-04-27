#include "shuffle.h"
#include "gc.h"

#include <stdio.h>

Weft_Shuffle *new_shuffle(unsigned in_count, unsigned out_count)
{
	Weft_Shuffle *shuffle =
		gc_alloc(sizeof(Weft_Shuffle) + out_count * sizeof(unsigned));
	shuffle->in_count = in_count;
	shuffle->out_count = out_count;

	return shuffle;
}

unsigned shuffle_get_in_count(const Weft_Shuffle *shuffle)
{
	return shuffle->in_count;
}

unsigned shuffle_get_out_count(const Weft_Shuffle *shuffle)
{
	return shuffle->out_count;
}

unsigned shuffle_get_out(const Weft_Shuffle *shuffle, unsigned index)
{
	return shuffle->out[index];
}

void shuffle_set_out(Weft_Shuffle *shuffle, unsigned index, unsigned value)
{
	shuffle->out[index] = value;
}

void shuffle_print(const Weft_Shuffle *shuffle)
{
	printf("{");
	for (unsigned i = 0; i < shuffle->in_count; i++) {
		printf("%u ", i);
	}
	printf("--");
	for (unsigned i = 0; i < shuffle->out_count; i++) {
		printf(" %u", shuffle->out[i]);
	}
	printf("}");
}
