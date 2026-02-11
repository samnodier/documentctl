#include "index_structure.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Creates a new Trie node
trie_node_t *create_node(void) {
  trie_node_t *new_node = (trie_node_t *)malloc(sizeof(trie_node_t));

  if (new_node == NULL) {
    return NULL;
  }
  new_node->isEndOfWord = false;
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    new_node->children[i] = NULL;
  }
  new_node->occurrences = NULL;
  new_node->char_index = -1;
  return new_node;
}

void trie_insert(trie_node_t *root, const char *word, int doc_id, int page_num,
                 long byte_offset) {

#ifdef DEBUG_MODE
  printf("[DEBUG INSERT] word='%s' doc=%d page=%d offset=%ld\n", word, doc_id,
         page_num, byte_offset);
#endif /* ifdef DEBUG_MODE                                                     \
        */
  trie_node_t *current = root;
  while (*word != '\0') {
    unsigned char c = *word;
    // Calculate index for the character
    int idx = c;
    if (current->children[idx] == NULL) {
      current->children[idx] = create_node();
    }
    current = current->children[idx];
    word++;
  }
  current->isEndOfWord = true;
  add_occurence_to_node(current, doc_id, page_num, byte_offset);
}

void trie_free(trie_node_t *node) {
  if (node == NULL)
    return;

  // Tell all 128 potential alphabet children to free themselves
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    if (node->children[i] != NULL) {
      trie_free(node->children[i]);
    }
  }

  // Free the linked list of occurrences for this word
  word_occurrence_t *curr_occur = node->occurrences;
  while (curr_occur != NULL) {
    word_occurrence_t *next = curr_occur->next;
    free(curr_occur);
    curr_occur = next;
  }

  // Finally free the node itself
  free(node);
}

word_occurrence_t *create_occurrence(int doc_id, int page_num,
                                     long byte_offset) {
  word_occurrence_t *new_occurrence =
      (word_occurrence_t *)malloc(sizeof(word_occurrence_t));

  if (new_occurrence == NULL) {
    return NULL;
  }

  new_occurrence->doc_id = doc_id;
  new_occurrence->page_num = page_num;
  new_occurrence->byte_offset = byte_offset;
  new_occurrence->next = NULL;

  return new_occurrence;
}

// For serialization to prevent duplicates
void add_occurence_to_node(trie_node_t *node, int doc_id, int page_num,
                           long byte_offset) {
  // Check if the list is not empty
  if (node->occurrences != NULL) {
    // Look at the very first item (the most recent one added) [We are
    // prepending]
    if (node->occurrences->doc_id == doc_id &&
        node->occurrences->page_num == page_num &&
        node->occurrences->byte_offset == byte_offset) {
      return; // We have already recorded this word for this page. Skip!
    }
  }

  // Otherwise proceed with the process
  word_occurrence_t *new_occ = create_occurrence(doc_id, page_num, byte_offset);
  if (new_occ == NULL)
    return;
  new_occ->next = node->occurrences;
  node->occurrences = new_occ;
}

// For deserialization because there's no dulicates
// Stop checking if occurrences are equal or anything like that

void relink_occurence(trie_node_t *node, int doc_id, int page_num,
                      long byte_offset) {
  word_occurrence_t *new_occ = create_occurrence(doc_id, page_num, byte_offset);
  if (new_occ == NULL)
    return;

  // Just prepend - no checking is needed
  new_occ->next = node->occurrences;
  node->occurrences = new_occ;
}

word_occurrence_t *trie_search(trie_node_t *root, const char *word) {
  trie_node_t *current = root;
  while (*word != '\0') {
    unsigned char c = *word;
    int idx = c;
    if (current->children[idx] == NULL) {
      return NULL;
    }
    current = current->children[idx];
    word++;
  }
  if (current->isEndOfWord == true) {
    return current->occurrences;
  }
  return NULL;
}

// Count the number of non NULL children in the trie node
int trie_children_count(trie_node_t *node) {
  int counter = 0;
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    if (node->children[i] != NULL) {
      counter++;
    }
  }
  return counter;
}

/* Visits every node and writes its data
 * Writes directly to the file pointer (FILE *fp)
 * This file pointer was opened in the engine_serialize (toolkit_core)
 */
int trie_node_serialize(trie_node_t *node, int char_index, FILE *fp) {
  if (node == NULL)
    return -1;

  fwrite(&char_index, sizeof(int), 1, fp);
  fwrite(&node->isEndOfWord, sizeof(bool), 1, fp);

  int child_count = trie_children_count(node);
  fwrite(&child_count, sizeof(int), 1, fp);

  // If this node is the end of the word
  int occurs = 0;
  if (node->isEndOfWord) {
    // Count the occurrences
    word_occurrence_t *curr = node->occurrences;
    while (curr != NULL) {
      occurs++;
      curr = curr->next;
    }
  }

  fwrite(&occurs, sizeof(int), 1, fp);
  word_occurrence_t *curr = node->occurrences;
  if (occurs > 0) {
    while (curr != NULL) {
      fwrite(&curr->doc_id, sizeof(int), 1, fp);
      fwrite(&curr->page_num, sizeof(int), 1, fp);
      fwrite(&curr->byte_offset, sizeof(long), 1, fp);
      curr = curr->next;
    }
  }

  // Serialize children
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    if (node->children[i] != NULL) {
      trie_node_serialize(node->children[i], i, fp);
    }
  }

  return 0;
}

/*
 * Visits the binary data in a file (FILE *fp)
 * Reads its data and store it into a respective variable
 */
trie_node_t *trie_node_deserialize(FILE *fp) {
  // 1. Read the nodes char index
  int char_index;
  fread(&char_index, sizeof(int), 1, fp);

  // 2. Allocate a new trie node
  trie_node_t *node = create_node();
  if (node == NULL) // If it fails
    return NULL;

  node->char_index = char_index;
  // 3. Read isEndOfWord
  fread(&node->isEndOfWord, sizeof(bool), 1, fp);

  // 4. Read number of children
  int child_count;
  fread(&child_count, sizeof(int), 1, fp);

  // 5. Read number of occurrences
  int occurs;
  fread(&occurs, sizeof(int), 1, fp);

  // 6. Rebuild the occurrences list
  if (occurs > 0) {
    for (int i = 0; i < occurs; i++) {
      int doc_id, page_num;
      fread(&doc_id, sizeof(int), 1, fp);
      fread(&page_num, sizeof(int), 1, fp);
      long byte_offset;
      fread(&byte_offset, sizeof(long), 1, fp);
      relink_occurence(node, doc_id, page_num, byte_offset);
    }
  }

  // 7. Deserialize children
  for (int i = 0; i < child_count; i++) {
    trie_node_t *child = trie_node_deserialize(fp);
    if (child != NULL) {
      node->children[child->char_index] = child;
    }
  }

  return node;
}
