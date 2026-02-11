#include "toolkit_core.h"
#include "index_structure.h"
#include "pdf_processor.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

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

int engine_serialize(search_engine_t *engine, char *filepath) {
  // 1. Open the file for writing in binary mode
  FILE *fp;
  fp = fopen(filepath, "wb");

  if (fp == NULL) {
    perror("Error opening file");
    return -1;
  }

  // 2. Write header info for the root node
  uint32_t MAGIC = 0xD0C0C0DE;
  fwrite(&MAGIC, sizeof(uint32_t), 1, fp);

  // 3. Write the version number
  uint16_t VERSION = 1;
  fwrite(&VERSION, sizeof(uint16_t), 1, fp);
  fwrite(&engine->doc_count, sizeof(int), 1, fp);

  // 4. Write document paths
  for (int i = 0; i < engine->doc_count; i++) {
    char *file_path = engine->document_map[i];
    int len = strlen(file_path);
    fwrite(&len, sizeof(int), 1, fp);
    fwrite(file_path, sizeof(char), len, fp);
  }

  // 5. Write ROOT metadata (but not using trie_node_serialize)
  trie_node_t *root = engine->index_root;
  fwrite(&root->isEndOfWord, sizeof(bool), 1, fp);
  int root_children_num = trie_children_count(root);
  fwrite(&root_children_num, sizeof(int), 1, fp);

  // 6. Serialize all children recursively

  for (int i = 0; i < ALPHABET_SIZE; i++) {
    if (root->children[i] != NULL) {
      trie_node_serialize(root->children[i], i, fp);
    }
  }

  fclose(fp);
  return 0;
}

// Deserialize and read from the file
search_engine_t *engine_deserialize(char *filepath) {
  // 1. Open the file for reading in binary mode
  FILE *fp;
  fp = fopen(filepath, "rb"); // Try opening the file

  if (fp == NULL) { // return if not possible
    return NULL;
  }

  // 2. Read and verify the magic number
  uint32_t MAGIC;
  fread(&MAGIC, sizeof(uint32_t), 1, fp);
  if (MAGIC != 0xD0C0C0DE) { // Check if this is the right file format
    fclose(fp);
    return NULL; // Return, this is wrong or corrupt file
  }

  // 3. Read the VERSION number
  uint16_t VERSION;
  fread(&VERSION, sizeof(uint16_t), 1, fp);

  // 4. Allocate a new search_engine_t
  search_engine_t *engine = malloc(sizeof(search_engine_t));
  if (engine == NULL) {
    fclose(fp);
    return NULL;
  }

  // 5. Read the document count
  int doc_count;
  fread(&doc_count, sizeof(int), 1, fp);
  engine->doc_count = doc_count;

  // 6. Rebuild the document_map array
  char **document_map = malloc(sizeof(char *) * doc_count);
  for (int i = 0; i < engine->doc_count; i++) {
    int len;
    fread(&len, sizeof(int), 1, fp);
    char *file_path = malloc(len + 1);
    fread(file_path, sizeof(char), len, fp);
    file_path[len] = '\0';
    document_map[i] = file_path;
  }

  engine->document_map = document_map;
  engine->doc_capacity = doc_count;

  // 7. Create and read the root node metadata
  trie_node_t *root = create_node();
  bool isEndOfWord;
  fread(&isEndOfWord, sizeof(bool), 1, fp);
  int root_children_num;
  fread(&root_children_num, sizeof(int), 1, fp);

  // 8. Deserialize all of root children
  for (int i = 0; i < root_children_num; i++) {
    trie_node_t *child = trie_node_deserialize(fp);
    if (child != NULL) {
      root->children[child->char_index] = child;
    }
  }
  engine->index_root = root;

  fclose(fp);
  return engine;
}

const char *engine_get_document_path(search_engine_t *engine, int doc_id) {
  return engine->document_map[doc_id];
}
