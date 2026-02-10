#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include "trie.h"

int *get_doc_ids_from_search(word_occurrence_t *list, int *out_count);
void free_results(int *results);

#endif // !QUERY_ENGINE_H
