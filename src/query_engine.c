#include "search.h"
#include "engine.h"
#include "trie.h"
#include <stdlib.h>

int *get_doc_ids_from_search(word_occurrence_t *list, int *out_count) {
  // Count how many results we have
  int count = 0;
  word_occurrence_t *curr = list;
  while (curr) {
    count++;
    curr = curr->next;
  }

  // Create a flat integer array
  int *ids = calloc(count, sizeof(int));
  curr = list;
  for (int i = 0; i < count; i++) {
    ids[i] = curr->doc_id;
    curr = curr->next;
  }

  *out_count = count; // tell python how many IDs are there in the array
  return ids;
}

// This returns a flat array if IDs that python can easily read
// This is because c traverse an array using a pointer to the first element
// But python doesn't
int *get_search_results(search_engine_t *engine, const char *word,
                        int *found_count) {
  word_occurrence_t *list = trie_search(engine->index_root, word);
  if (list == NULL) {
    *found_count = 0;
    return NULL;
  }

  // Count links
  int count = 0;
  word_occurrence_t *curr = list;
  while (curr) {
    count++;
    curr = curr->next;
  }

  // Allocate flat array (space for doc_id and page_num)
  int *results = malloc(sizeof(int) * count * 2);
  curr = list;
  for (int i = 0; i < count; i++) {
    results[i * 2] = curr->doc_id;
    results[i * 2 + 1] = curr->page_num;
    curr = curr->next;
  }

  *found_count = count;
  return results;
}

void free_results(int *results) {
  if (results != NULL) {
    free(results);
  }
}
