#ifndef ENGINE_H
#define ENGINE_H

#include "trie.h"
#include <stdlib.h>

typedef struct SearchEngine {
  trie_node_t *index_root;
  char **document_map;
  int doc_count;
  int doc_capacity;
} search_engine_t;

search_engine_t *engine_create();
void engine_free(search_engine_t *engine);

#endif // !ENGINE_H
