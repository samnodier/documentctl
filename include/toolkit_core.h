#ifndef TOOLKIT_CORE_H
#define TOOLKIT_CORE_H

#include "index_structure.h"
#include <pthread.h>

typedef struct SearchEngine {
    trie_node_t *index_root;
    char **document_map;
    int doc_count;
    int doc_capacity;
    pthread_mutex_t trie_lock;
} search_engine_t;

search_engine_t *engine_create();
void engine_free(search_engine_t *engine);
void engine_index_all_chunked(search_engine_t *engine);
const char *engine_get_document_path(search_engine_t *engine, int doc_id);
int engine_serialize(search_engine_t *engine, char *filepath);
search_engine_t *engine_deserialize(char *filepath);

#endif // !TOOLKIT_CORE_H
