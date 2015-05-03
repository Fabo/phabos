/*
 * Copyright (C) 2014-2015 Fabien Parent. All rights reserved.
 * Author: Fabien Parent <parent.f@gmail.com>
 *
 * Provided under the three clause BSD license found in the LICENSE file.
 */

#include <phabos/hashtable.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h> 

static const size_t HASHTABLE_MIN_SIZE = 8;

struct hashtable_node
{
    void *key;
    void *value;
    bool is_used : 1;
    bool is_deleted : 1;
};

static struct hashtable_node *hashtable_get_node(hashtable_t *ht, void *key,
                                                 bool find_deleted)
{
    unsigned h = ht->hash(ht, key);
    while ((ht->table[h].is_used && ht->compare(ht->table[h].key, key)) ||
           (ht->table[h].is_deleted && !find_deleted)) {
        h = (h + 1) % ht->size;
    }
    return &ht->table[h];
}

static void hashtable_resize(hashtable_t *ht, size_t size)
{
    struct hashtable_node *table;
    size_t old_size = ht->size;

    table = ht->table;
    ht->table = malloc(size * sizeof(struct hashtable_node));
    memset(ht->table, 0, size * sizeof(struct hashtable_node));
    ht->size = size;
    ht->count = 0;

    for (int i = 0; i < old_size; i++) {
        if (table[i].is_used)
            hashtable_add(ht, table[i].key, table[i].value);
    }

    free(table);
}

void hashtable_init(hashtable_t *ht, hashtable_hash_fct_t hash,
                    hashtable_key_compare_fct_t compare)
{
    assert(ht);
    assert(hash);

    memset(ht, 0, sizeof(*ht));
    ht->table = malloc(HASHTABLE_MIN_SIZE * sizeof(struct hashtable_node));
    memset(ht->table, 0, HASHTABLE_MIN_SIZE * sizeof(struct hashtable_node));
    ht->hash = hash;
    ht->compare = compare;
    ht->size = HASHTABLE_MIN_SIZE;
}

void hashtable_add(hashtable_t *ht, void *key, void *value)
{
    struct hashtable_node *node =  hashtable_get_node(ht, key, false);
    if (!node->is_used)
        node = hashtable_get_node(ht, key, true);

    node->key = key;
    node->value = value;

    if (!node->is_used) {
        node->is_used = true;
        ht->count += 1;
    }

    if (ht->count >= ht->size / 2)
        hashtable_resize(ht, ht->size * 2);
}

void *hashtable_get(hashtable_t *ht, void *key)
{
    struct hashtable_node *node =  hashtable_get_node(ht, key, false);
    return node->is_used ? node->value : NULL;
}

bool hashtable_has(hashtable_t *ht, void *key)
{
    return hashtable_get_node(ht, key, false)->is_used;
}

void hashtable_remove(hashtable_t *ht, void *key)
{
    struct hashtable_node *node =  hashtable_get_node(ht, key, false);
    if (!node->is_used)
        return;

    ht->count--;
    node->is_used = false;
    node->is_deleted = true;
    node->key = node->value = NULL;
}

unsigned int hash_uint(hashtable_t *ht, void *key)
{
    return (unsigned int) key % ht->size;
}

unsigned int hash_string(hashtable_t *ht, void *key)
{
    const char* data = (const char *) key;
    unsigned h = 0;

    for (int i = 0; data[i] != '\0'; i++)
        h += data[i];

    return h % ht->size;
}

int hashtable_key_compare_uint(const void *key1, const void *key2)
{
    return (unsigned int) key1 - (unsigned int) key2;
}

int hashtable_key_compare_string(const void *key1, const void *key2)
{
    return strcmp(key1, key2);
}