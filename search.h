#ifndef SEARCH_H
#define SEARCH_H

#include "trie.h"

int *get_doc_ids_from_search(word_occurrence_t *list, int *out_count);
void free_results(int *results);

#endif // !SEARCH_H
