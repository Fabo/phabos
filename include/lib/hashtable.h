/*
 * Copyright (C) 2014-2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <stddef.h>
#include <stdbool.h>

struct hashtable_node;
struct hashtable;

typedef unsigned int (*hashtable_hash_fct_t)(struct hashtable *ht, void *key);

typedef struct hashtable
{
    struct hashtable_node *table;
    size_t size;
    size_t count;
    hashtable_hash_fct_t hash;
} hashtable_t;

void hashtable_init(hashtable_t *ht, hashtable_hash_fct_t hash);
void hashtable_add(hashtable_t *ht, void *key, void *value);
void *hashtable_get(hashtable_t *ht, void *key);
bool hashtable_has(hashtable_t *ht, void *key);
void hashtable_remove(hashtable_t *ht, void *key);

#define hashtable_init_uint(ht) hashtable_init((ht), hash_uint);
#define hashtable_init_string(ht) hashtable_init((ht), hash_string);

unsigned int hash_uint(hashtable_t *ht, void *key);
unsigned int hash_string(hashtable_t *ht, void *key);

#endif /* __HASHTABLE_H__ */

