#ifndef ARRAY_H
#define ARRAY_H

#define DEFAULT_SIZE 8

#include <stdlib.h>
#include <stdio.h>

struct array {
	int len;
	int size;
	int item_size;
	void *data;
};

#define grow(array)	\
	(array)->size *= 1.5;	\
	(array)->data = realloc((array)->data, (array)->size * (array)->item_size);

#define array_init(array, size_of)	\
	(array)->item_size = (size_of);	\
	(array)->size = DEFAULT_SIZE;	\
	(array)->len = 0;		\
	(array)->data = malloc(size_of * (array)->size);

#define array_push(array, v) 						\
	do {								\
		if ((array)->len == (array)->size) {			\
			grow((array));					\
		}							\
		((typeof(v)*) (array)->data)[(array)->len] = (v);	\
		++(array)->len;						\
	} while (0)

#define array_get(array, type, index) ((type *)(array)->data)[(index)]

#define array_pop(array)	(--(array)->len);

#define array_clear(array)	((array)->len = 0);

#define array_free(array)	free((array)->data);

#endif
