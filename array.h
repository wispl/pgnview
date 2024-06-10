#ifndef ARRAY_H
#define ARRAY_H

#define DEFAULT_SIZE 8
#define DEFAULT_GROWTH_FACTOR 2

#include <stdlib.h>

#define array_define(name, item_type) \
	struct name {		      \
		int len;              \
		int size;             \
		item_type *data;      \
	}

#define item_size(array) (sizeof((array)->data[0]))

#define grow(array)                                                                         \
	do {                                                                                \
		(array)->size *= DEFAULT_GROWTH_FACTOR;                                     \
		(array)->data = realloc((array)->data, (array)->size * item_size((array))); \
	} while(0)

#define array_init(array)                                                   \
	do {                                                                \
		(array)->size = DEFAULT_SIZE;                               \
		(array)->len = 0;                                           \
		(array)->data = malloc((array)->size * item_size((array))); \
	} while (0)

#define ARRAY(name)                                                  \
	(name) = {                                                   \
		.size = DEFAULT_SIZE,                                \
		.len = 0,                                            \
		.data = malloc((name).size * sizeof((name).data[0])) \
	}								

#define array_push(array, v)                          \
	do {                                          \
		if ((array)->len == (array)->size) {  \
			grow((array));                \
		}                                     \
		(array)->data[(array)->len] = (v);    \
		++(array)->len;			      \
	} while (0)

#define array_get(array, index) (array)->data[(index)]

#define array_pop(array)	(--(array)->len)

#define array_clear(array)	((array)->len = 0)

#define array_free(array)	free((array)->data)

#endif
