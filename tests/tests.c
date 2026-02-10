#include "index_structure.h"
#include "query_engine.h"
#include "toolkit_core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_trie_basic_logic() {
  printf("Running: test_trie_basic_logic... ");
  trie_node_t *root = create_node();

  // Test prepending logic: Page 10 should come BEFORE Page 5
  trie_insert(root, "intelligence", 1, 5);
  trie_insert(root, "intelligence", 1, 10);

  word_occurrence_t *occ = trie_search(root, "intelligence");
  assert(occ != NULL);
  assert(occ->page_num == 10);
  assert(occ->next->page_num == 5);

  trie_free(root);
  printf("PASSED!\n");
}

void test_query_engine_array_packing() {
  printf("Running: test_query_engine_array_packing... ");

  // Create a dummy engine for this test
  search_engine_t engine;
  engine.index_root = create_node();

  trie_insert(engine.index_root, "toolkit", 1, 1);

  int count = 0;
  int *results = get_search_results(&engine, "toolkit", &count);

  assert(count == 1);
  assert(results != NULL);

  // Verify interleaved packing: [doc_id, page_num]
  assert(results[0] == 1); // doc_id
  assert(results[1] == 1); // page_num

  free(results);
  trie_free(engine.index_root);
  printf("PASSED!\n");
}

int main() {
  printf("--- STARTING DOCUMENT INTELLIGENCE TEST SUITE ---\n");
  test_trie_basic_logic();
  test_query_engine_array_packing();
  printf("--- ALL TESTS PASSED ---\n");
  return 0;
}
