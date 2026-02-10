#include "toolkit_core.h"
#include "pdf_processor.h"
#include <stdio.h>
#include <stdlib.h>

search_engine_t *engine_create() {
  search_engine_t *engine = malloc(sizeof(search_engine_t));
  if (engine == NULL) {
    return NULL;
  }

  engine->index_root = create_node(); // Start the trie
  engine->doc_count = 0;
  engine->doc_capacity = 100; // Start with a space for 100 PDFs
  char **doc_map = malloc(sizeof(char *) * engine->doc_capacity);
  if (doc_map == NULL) {
    return NULL;
  }
  engine->document_map = doc_map;
  return engine;
}

void engine_free(search_engine_t *engine) {
  if (engine == NULL)
    return;

  trie_free(engine->index_root);

  // Free all strings in the document_map
  for (int i = 0; i < engine->doc_count; i++) {
    free(engine->document_map[i]);
  }

  // Free the array of pointers
  free(engine->document_map);

  // Free the engine shell
  free(engine);
}

void engine_index_all(search_engine_t *engine) {
  for (int i = 0; i < engine->doc_count; i++) {
    const char *path = engine->document_map[i];
    // call the indexing function here for each path!
    printf("Indexing [%d/%d]: %s\n", i + 1, engine->doc_count, path);
    index_pdf_content(engine, i, path);
  }
}

const char *engine_get_document_path(search_engine_t *engine, int doc_id) {
  return engine->document_map[doc_id];
}
