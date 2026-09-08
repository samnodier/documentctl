#ifndef INDEX_STRUCTURE_H
#define INDEX_STRUCTURE_H

#include <stdbool.h>
#include <stdio.h>

#define ALPHABET_SIZE 128

typedef struct WordOccurence {
  int doc_id;
  int page_num;
  long byte_offset;
  struct WordOccurence *next;
} word_occurrence_t;

typedef struct {
  int doc_id;
  int page_num;
  long byte_offset;
} occurrence_transfer_t;

typedef struct TrieNode {
  struct TrieNode *children[ALPHABET_SIZE];
  bool isEndOfWord;
  word_occurrence_t *occurrences;
  int char_index;
} trie_node_t;

trie_node_t *create_node(void);
word_occurrence_t *create_occurrence(int doc_id, int page_num,
                                     long byte_offset);
void add_occurence_to_node(trie_node_t *node, int doc_id, int page_num,
                           long byte_offset);
void relink_occurence(trie_node_t *node, int doc_id, int page_num,
                      long byte_offset);
void trie_insert(trie_node_t *root, const char *word, int doc_id, int page_num,
                 long byte_offset);
word_occurrence_t *trie_search(trie_node_t *root, const char *word);
void trie_free(trie_node_t *node);
int trie_children_count(trie_node_t *node);
int trie_node_serialize(trie_node_t *node, int char_index, FILE *fp);
trie_node_t *trie_node_deserialize(FILE *fp);

#endif // !INDEX_STRUCTURE_H
