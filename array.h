#ifndef ARRAY_H
#define ARRAY_H

#define DEFAULT_SIZE 8

#include <stdlib.h>

#define array_define(name, item_type)	\
	struct name {			\
		int len;		\
		int size;		\
		item_type *data;	\
	};

#define grow(array)	\
	(array)->size *= 1.5;	\
	(array)->data = realloc((array)->data, (array)->size * sizeof((array)->data[0]));

#define array_init(array)		\
	(array)->size = DEFAULT_SIZE;	\
	(array)->len = 0;		\
	(array)->data = malloc((array)->size * sizeof((array)->data[0]));

#define array_push(array, v) 				\
	do {						\
		if ((array)->len == (array)->size) {	\
			grow((array));			\
		}					\
		(array)->data[(array)->len] = (v);	\
		++(array)->len;				\
	} while (0)

#define array_get(array, index) (array)->data[(index)]

#define array_pop(array)	(--(array)->len);

#define array_clear(array)	((array)->len = 0);

#define array_free(array)	free((array)->data);

#endif
