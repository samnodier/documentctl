#include "index_structure.h"
#include <stdbool.h>
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
  return new_node;
}

void trie_insert(trie_node_t *root, const char *word, int doc_id,
                 int page_num) {
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
  add_occurence_to_node(current, doc_id, page_num);
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

word_occurrence_t *create_occurrence(int doc_id, int page_num) {
  word_occurrence_t *new_occurrence =
      (word_occurrence_t *)malloc(sizeof(word_occurrence_t));

  if (new_occurrence == NULL) {
    return NULL;
  }

  new_occurrence->doc_id = doc_id;
  new_occurrence->page_num = page_num;
  new_occurrence->next = NULL;

  return new_occurrence;
}

void add_occurence_to_node(trie_node_t *node, int doc_id, int page_num) {
  // Check if the list is not empty
  if (node->occurrences != NULL) {
    // Look at the very first item (the most recent one added) [We are
    // prepending]
    if (node->occurrences->doc_id == doc_id &&
        node->occurrences->page_num == page_num) {
      return; // We have already recorded this word for this page. Skip!
    }
  }

  // Otherwise proceed with the process
  word_occurrence_t *new_occ = create_occurrence(doc_id, page_num);
  if (new_occ == NULL)
    return;
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
