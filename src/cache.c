/* Goxel 3D voxels editor
 *
 * copyright (c) 2017 Guillaume Chereau <guillaume@noctua-software.com>
 *
 * Goxel is free software: you can redistribute it and/or modify it under the
 * terms of the GNU General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later
 * version.

 * Goxel is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.

 * You should have received a copy of the GNU General Public License along with
 * goxel.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "goxel.h"

typedef struct item item_t;
struct item {
    UT_hash_handle  hh;
    char            key[32];
    void            *data;
    uint64_t        last_used;
    int             (*delfunc)(void *data);
};

struct cache {
    item_t *items;
    uint64_t clock;
    int size;
    int max_size;
};

cache_t *cache_create(int size)
{
    cache_t *cache = calloc(1, sizeof(*cache));
    cache->max_size = size;
    return cache;
}

static int sort_cmp(void *a, void *b)
{
    return cmp(((item_t*)a)->last_used, ((item_t*)b)->last_used);
}

static void cleanup(cache_t *cache)
{
    item_t *item;
    HASH_SORT(cache->items, sort_cmp);
    while (cache->size >= cache->max_size) {
        item = cache->items;
        HASH_DEL(cache->items, item);
        item->delfunc(item->data);
        free(item);
        cache->size--;
    }
}

void cache_add(cache_t *cache, const void *key, int len, void *data,
               int (*delfunc)(void *data))
{
    item_t *item = calloc(1, sizeof(*item));
    assert(len <= sizeof(item->key));
    memcpy(item->key, key, len);
    item->data = data;
    item->last_used = cache->clock++;
    item->delfunc = delfunc;
    HASH_ADD(hh, cache->items, key, len, item);
    cache->size++;
    if (cache->size >= cache->max_size) cleanup(cache);
}

void *cache_get(cache_t *cache, const void *key, int keylen)
{
    item_t *item;
    HASH_FIND(hh, cache->items, key, keylen, item);
    if (!item) return NULL;
    item->last_used = cache->clock++;
    return item->data;
}