#ifndef QUERY_ENGINE_H
#define QUERY_ENGINE_H

#include "index_structure.h"

typedef struct SearchEngine search_engine_t;

int *get_search_results(search_engine_t *engine, const char *word,
                        int *found_count);

int *get_doc_ids_from_search(word_occurrence_t *list, int *out_count);
void free_results(int *results);

#endif // !QUERY_ENGINE_H
