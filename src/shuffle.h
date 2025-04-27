#ifndef WEFT_SHUFFLE_H
#define WEFT_SHUFFLE_H

// Forward Declarations

typedef struct weft_shuffle Weft_Shuffle;

// Data Types

struct weft_shuffle {
	unsigned in_count;
	unsigned out_count;
	unsigned out[];
};

// Functions

Weft_Shuffle *new_shuffle(unsigned in_count, unsigned out_count);
unsigned shuffle_get_in_count(const Weft_Shuffle *shuffle);
unsigned shuffle_get_out_count(const Weft_Shuffle *shuffle);
unsigned shuffle_get_out(const Weft_Shuffle *shuffle, unsigned index);
void shuffle_set_out(Weft_Shuffle *shuffle, unsigned index, unsigned value);
void shuffle_print(const Weft_Shuffle *shuffle);

#endif
