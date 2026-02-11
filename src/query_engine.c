#include "query_engine.h"
#include "index_structure.h"
#include "toolkit_core.h"
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
occurrence_transfer_t *get_search_results(search_engine_t *engine,
                                          const char *word, int *found_count) {
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
  occurrence_transfer_t *results =
      malloc(sizeof(occurrence_transfer_t) * count);

  curr = list;
  for (int i = 0; i < count; i++) {
    results[i].doc_id = curr->doc_id;
    results[i].page_num = curr->page_num;
    results[i].byte_offset = curr->byte_offset;
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
