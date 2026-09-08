#include "index_structure.h"
#include "query_engine.h"
#include "toolkit_core.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void test_trie_basic_logic() {
  printf("Running: test_trie_basic_logic... ");
  trie_node_t *root = create_node();

  // Test prepending logic: Page 10 should come BEFORE Page 5
  trie_insert(root, "intelligence", 1, 5, 100);
  trie_insert(root, "intelligence", 1, 10, 200);

  word_occurrence_t *occ = trie_search(root, "intelligence");
  assert(occ != NULL);
  assert(occ->page_num == 10);
  assert(occ->byte_offset == 200);
  assert(occ->next->page_num == 5);
  assert(occ->next->byte_offset == 100);

  trie_free(root);
  printf("PASSED!\n");
}

void test_serialization_roundtrip() {
  printf("Running: test_serialization_roundtrip... ");

  // 1. Create engine and insert test data
  search_engine_t *engine1 = engine_create();
  assert(engine1 != NULL);

  // Add dummy documents
  engine1->doc_count = 2;
  engine1->document_map[0] = strdup("/test/doc1.pdf");
  engine1->document_map[1] = strdup("/test/doc2.pdf");

  // Insert words (sam as the original test style)
  trie_insert(engine1->index_root, "intelligence", 1, 5, 100);
  trie_insert(engine1->index_root, "intelligence", 1, 10, 200);
  trie_insert(engine1->index_root, "toolkit", 0, 3, 150);
  trie_insert(engine1->index_root, "algorithm", 0, 7, 500);

  // 2. Serialize
  const char *test_file = "tests/test_data/text_index.db";
  int result = engine_serialize(engine1, (char *)test_file);
  assert(result == 0);

  // 3. Deserialize into New Engine
  search_engine_t *engine2 = engine_deserialize((char *)test_file);
  assert(engine2 != NULL);

  // 4. Verify document map
  assert(engine2->doc_count == 2);
  assert(strcmp(engine2->document_map[0], "/test/doc1.pdf") == 0);

  // 5. Search and verify data
  word_occurrence_t *intel = trie_search(engine2->index_root, "intelligence");
  assert(intel != NULL);
  assert(intel->page_num == 5);
  assert(intel->byte_offset == 100);
  assert(intel->next != NULL);
  assert(intel->next->page_num == 10);
  assert(intel->next->byte_offset == 200);

  // 6. Cleanup
  engine_free(engine1);
  engine_free(engine2);

  printf("PASSED!\n");
}

void test_corrupted_files() {
  printf("Running: test_corrupted_file... ");

  // Create fake file with wrong magic number
  const char *bad_file = "tests/test_data/bad_index.db";
  FILE *fp = fopen(bad_file, "wb");

  assert(fp != NULL);

  uint32_t wrong_magic = 0xDEADBEEF;
  fwrite(&wrong_magic, sizeof(uint32_t), 1, fp);
  fclose(fp);

  // Should return NULL, not crash
  search_engine_t *engine = engine_deserialize((char *)bad_file);
  assert(engine == NULL);

  printf("PASSED!\n");
}

void test_empty_engine() {
  printf("Running: test_empty_engine... ");

  search_engine_t *engine1 = engine_create();

  assert(engine1 != NULL);

  const char *test_file = "tests/test_data/empty_index.db";
  int result = engine_serialize(engine1, (char *)test_file);
  assert(result == 0);

  search_engine_t *engine2 = engine_deserialize((char *)test_file);
  assert(engine2 != NULL);
  assert(engine2->doc_count == 0);

  word_occurrence_t *none = trie_search(engine2->index_root, "anything");
  assert(none == NULL);

  engine_free(engine1);
  engine_free(engine2);

  printf("PASSED\n");
}

void test_query_engine_array_packing() {
  printf("Running: test_query_engine_array_packing... ");

  // Create a dummy engine for this test
  search_engine_t engine;
  engine.index_root = create_node();

  trie_insert(engine.index_root, "toolkit", 1, 1, 15);

  int count = 0;
  occurrence_transfer_t *results =
      get_search_results(&engine, "toolkit", &count);

  assert(count == 1);
  assert(results != NULL);

  // Verify interleaved packing: [doc_id, page_num]
  assert(results[0].doc_id == 1);       // doc_id
  assert(results[0].page_num == 1);     // page_num
  assert(results[0].byte_offset == 15); // byte_offset

  free(results);
  trie_free(engine.index_root);
  printf("PASSED!\n");
}

int main() {
  printf("\n");
  printf("╔════════════════════════════════════════════╗\n");
  printf("║      SERIALIZATION TEST SUITE              ║\n");
  printf("╚════════════════════════════════════════════╝\n");
  printf("\n");

  // Create test data directory
  system("mkdir -p tests/test_data");

  // Run tests
  test_trie_basic_logic();
  test_serialization_roundtrip();
  test_corrupted_files();
  test_empty_engine();
  test_query_engine_array_packing();
  printf("\n");
  printf("╔════════════════════════════════════════════╗\n");
  printf("║         ALL TESTS PASSED! ✅               ║\n");
  printf("╚════════════════════════════════════════════╝\n");
  printf("\n");

  return 0;
}
