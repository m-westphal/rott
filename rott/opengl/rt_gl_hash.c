#include <assert.h>
#include <stdlib.h>
#include "../develop.h"
#if (DEBUG==1)
#include <math.h>
#endif
#include "SDL.h"
#include "rt_gl_cap.h"
#include "rt_gl_hash.h"

static struct bucket* lumphash_get_internal(lumphash *h, int lumpnum, size_t pos);

static void lumphash_get_stat(lumphash *h);

boolean
lumphash_init(lumphash *h, size_t items)
{
	assert(items > 0);
	h->buckets = calloc(items, sizeof(struct bucket *));
	if (!h->buckets) return false;

	h->hash_size = items;
	h->hash_fillcount = 0;
	h->size = 0;

	h->last_active = NULL;
	h->least_active = NULL;

	return true;
}


static void _remove_from_active_list(lumphash *h, struct bucket *b) {
		if (b->more_active != NULL)
			b->more_active->less_active = b->less_active;
		else
			h->last_active = b->less_active;

		if (b->less_active != NULL)
			(b->less_active)->more_active = b->more_active;
		else
			h->least_active = b->more_active;
}


static inline unsigned int hash_function(int a, unsigned int s) {
	return ( (unsigned int) a) % s;
}

static int
rehash( void *p)
{
	lumphash *h = (lumphash *) p;
	struct bucket **newbuckets;
	size_t i;
	size_t newsize;
	struct bucket **buckets = h->buckets;

#if (DEBUG==1)
	lumphash_get_stat(h);
#endif


	newsize = h->hash_size*2+17;

	printf("Rehash: %zu -> %zu\n", h->hash_size, newsize);
	assert( h->hash_size < newsize );
	assert( (h->hash_fillcount * 100 / h->hash_size) > 200) ;

	newbuckets = calloc(newsize, sizeof(struct bucket *));
	if (!newbuckets) {
		return 0;
	}

	for (i=0; i < h->hash_size; i++) {
		if (*buckets) {
			struct bucket *b = *buckets;
			while (b) {
				size_t hashval = hash_function (b->lumpnum, newsize);
				struct bucket *relocate = b;
				struct bucket *newslot;
				b = b->next;
				relocate->next = NULL;
				newslot = newbuckets[hashval];
				if (!newslot) {
			                newbuckets[hashval] = relocate;
				} else {
					while (newslot->next)
				                newslot = newslot->next;

				        newslot->next = relocate;
				}
			}
		}
		buckets++;
	}

	free(h->buckets);
	h->buckets = newbuckets;
	h->hash_size = newsize;

	return 1;
}


void
lumphash_free(lumphash *h)
{
	size_t i;
	struct bucket **buckets = h->buckets;

	for (i=0; i < h->hash_size; i++) {
		if (*buckets) {
			struct bucket *b = *buckets;
			while (b) {
				struct bucket *victim = b;
				rtglDeleteTextures(1, &b->texture_id);
				b = b->next;
				free(victim);
			}
		}
		buckets++;
	}
	h->size = 0;
	h->hash_size = 0;
	h->hash_fillcount = 0;
	free(h->buckets);
	h->last_active = NULL;
	h->least_active = NULL;
	h->buckets = NULL;
}


boolean
lumphash_add(lumphash *h, int lumpnum, GLuint id, const size_t size)
{
	struct bucket *oldbucket, *newbucket;
	unsigned int hash;

	assert(h->hash_size);
	assert(lumphash_get(h, lumpnum) == NULL);

	hash = hash_function(lumpnum, h->hash_size);

	newbucket = malloc(sizeof(struct bucket));
	if (!newbucket) {
		return false;
	}

	newbucket->lumpnum = lumpnum;
	newbucket->texture_id = id;
	newbucket->size = size;
	h->size += size;
	newbucket->next = NULL;
	newbucket->more_active = NULL;
	newbucket->less_active = h->last_active;
	if (h->last_active != NULL) h->last_active->more_active = newbucket;
	h->last_active = newbucket;
	if (h->least_active == NULL) h->least_active = newbucket;

	oldbucket = h->buckets[hash];
	h->hash_fillcount++;

	if (!oldbucket) {
		h->buckets[hash] = newbucket;
	}
	else {
		h->buckets[hash] = newbucket;
		newbucket->next = oldbucket;
	}

	if (((h->hash_fillcount * 100 / h->hash_size) > 200)) {
		rehash(h);
	}

	return true;
}


void
lumphash_delete(lumphash *h, int lumpnum)
{
	unsigned int hash;
	struct bucket *b;

	assert(h->hash_size);

	hash = hash_function(lumpnum, h->hash_size);

	b = h->buckets[hash];
	if (!b) {
		return;
	}

	if (lumpnum == b->lumpnum) {
		h->buckets[hash] = b->next;

		_remove_from_active_list(h, b);
		rtglDeleteTextures(1, &b->texture_id);
		h->size -= b->size;
		free(b);

		assert(h->hash_fillcount > 0);
		h->hash_fillcount--;
		return;
	}

	while (b->next) {
		struct bucket *bucket_prev = b;
		b = b->next;

		if (lumpnum == b->lumpnum) {
			bucket_prev->next = b->next;

			_remove_from_active_list(h, b);
			rtglDeleteTextures(1, &b->texture_id);
			h->size -= b->size;
			free(b);

			assert(h->hash_fillcount > 0);
			h->hash_fillcount--;
			return;
		}
	}
}


GLuint*
lumphash_get(lumphash *h, int lumpnum)
{
	unsigned int hash;
	struct bucket *b;

	assert(h->hash_size);

	hash = hash_function(lumpnum, h->hash_size);

	b = lumphash_get_internal(h, lumpnum, hash);

	if (b) {
		if (b->more_active != NULL) {
			(b->more_active)->less_active = b->less_active;

			if (b->less_active != NULL)
				(b->less_active)->more_active = b->more_active;
			else
				h->least_active = b->more_active;

			b->less_active = h->last_active;
			h->last_active->more_active = b;
			h->last_active = b;

			b->more_active = NULL;
		}

		return &b->texture_id;
	}

	return NULL;
}


static struct bucket*
lumphash_get_internal(lumphash *h, int lumpnum, size_t pos)
{
	struct bucket *b = h->buckets[pos];

	while (b) {
		if (lumpnum == b->lumpnum)
			return b;

		b = b->next;
	}

	return NULL;
}


static void lumphash_get_stat(lumphash *h) {
#if (DEBUG==1)
	size_t bucket_nr[h->hash_size], max_length = 0, used_buckets = 0, i, acctime = 0;
	unsigned int length[6] = {0,0,0,0,0,0};
	float e, var = 0;

	/* assuming equal distribution! */

	for (i = 0; i < h->hash_size; i++) {
		bucket_nr[i] = 0;

		struct bucket *b = h->buckets[i];

		while (b) {
			b = b->next;
			bucket_nr[i]++;
		}

		if (bucket_nr[i] > 0) {
			used_buckets++;
			if (bucket_nr[i] < 6)
				length[bucket_nr[i]]++;
			else
				length[0]++;

			if (bucket_nr[i] > max_length)
				max_length = bucket_nr[i];

			acctime += (bucket_nr[i] * (bucket_nr[i] + 1)) / 2;
		}
	}

	//e = (float) h->hash_fillcount / (float) used_buckets ;
	e = (float) acctime / (float) h->hash_fillcount;

	for (i = 0; i < h->hash_size; i++)
		if (bucket_nr[i] != 0) {
			var += e * (float) bucket_nr[i] * ( e - (float) bucket_nr[i] - 1.0f)
				+ ( (float) bucket_nr[i] * ( (float) bucket_nr[i] + 1.0f) * (2.0f * (float) bucket_nr[i] + 1.0f)) / 6.0f;
		}

	var /= (float) h->hash_fillcount;

	printf("BU: %f, EA: %f, StdDev: %f, WA: %u, [%f,%f,%f,%f,%f ... %f]\n", (float)  used_buckets / (float) h->hash_size,
			e, sqrtf(var), max_length,
			(float) length[1] / used_buckets,
			(float) length[2] / used_buckets,
			(float) length[3] / used_buckets,
			(float) length[4] / used_buckets,
			(float) length[5] / used_buckets,
			(float) length[0] / used_buckets
			);

#endif
}
