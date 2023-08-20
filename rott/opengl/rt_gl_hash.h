#ifndef RTGL_HASH_HDR_
#define RTGL_HASH_HDR_

#include <assert.h>
#include <stdlib.h>
#include "../rt_def.h"

struct bucket {
	int lumpnum;
	GLuint texture_id;
	struct bucket *next;
	struct bucket *less_active;
	struct bucket *more_active;
	size_t size;
};


typedef struct {
	struct bucket **buckets;
	struct bucket *last_active;
	struct bucket *least_active;
	size_t hash_size;
	size_t hash_fillcount;
	size_t size;
} lumphash;


boolean lumphash_init(lumphash *h, size_t items);

void lumphash_free(lumphash *h);

boolean lumphash_add(lumphash *h, int lumpnum, GLuint id, const size_t size);

void lumphash_delete(lumphash *h, int lumpnum);

GLuint* lumphash_get(lumphash *h, int lumpnum);

#endif
