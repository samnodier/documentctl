#include "toolkit_core.h"
#include "index_structure.h"
#include "pdf_processor.h"
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

doc_metadata_t *engine_get_metadata(search_engine_t *engine, int doc_id) {
    if (doc_id < 0 || doc_id >= engine->doc_count)
        return NULL;
    return &engine->metadata_map[doc_id];
}

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
        trie_free(engine->index_root);
        free(engine);
        return NULL;
    }
    engine->document_map = doc_map;
    engine->metadata_map =
        malloc(sizeof(doc_metadata_t) * engine->doc_capacity);
    // Initialize the titles and authors
    for (int i = 0; i < engine->doc_capacity; i++) {
        engine->metadata_map[i].title = NULL;
        engine->metadata_map[i].author = NULL;
    }
    pthread_mutex_init(&engine->trie_lock, NULL);
    return engine;
}

void engine_free(search_engine_t *engine) {
    if (engine == NULL)
        return;

    trie_free(engine->index_root);

    // Destroy the thread
    pthread_mutex_destroy(&engine->trie_lock);

    // Free all strings in the document_map
    for (int i = 0; i < engine->doc_count; i++) {
        free(engine->document_map[i]);
    }

    // Free all titles and authors
    for (int i = 0; i < engine->doc_count; i++) {
        free(engine->metadata_map[i].title);
        free(engine->metadata_map[i].author);
    }

    // Free the array of pointers
    free(engine->document_map);

    // Free the engine shell
    free(engine);
}

// Incorporate chunking to allow the threader to work on multiple files
void engine_index_all_chunked(search_engine_t *engine) {
    if (engine->doc_count == 0) {
        return;
    }

    long n_procs = sysconf(_SC_NPROCESSORS_ONLN);

    long num_threads = (n_procs > 0) ? (int)n_procs : 8;

    // Don't create more threads than we have files for
    if (num_threads > engine->doc_count) {
        num_threads = engine->doc_count;
    }

    pthread_t *threads = malloc(sizeof(pthread_t) * num_threads);

    if (threads == NULL) {
        return;
    }

    int files_per_thread = engine->doc_count / num_threads;

    for (int i = 0; i < num_threads; i++) {
        thread_chunk_t *chunk = malloc(sizeof(thread_chunk_t));
        if (chunk == NULL) {
            return; // In the future handle this better
        }

        chunk->engine = engine;
        chunk->start_index = i * files_per_thread;

        // The last thread takes any remaining files (handing the remainder)
        if (i == num_threads - 1) {
            chunk->end_index = engine->doc_count;
        } else {
            chunk->end_index = chunk->start_index + files_per_thread;
        }

        // Create the threads
        if (pthread_create(&threads[i], NULL, thread_chunk_worker, chunk) !=
            0) {
            perror("Failed to create thread");
            free(chunk);
        }
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
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

    // 4. Write document paths, title and author
    for (int i = 0; i < engine->doc_count; i++) {
        char *file_path = engine->document_map[i];
        int len = strlen(file_path);
        fwrite(&len, sizeof(int), 1, fp);
        fwrite(file_path, sizeof(char), len, fp);

        // Handle title
        int title_len = engine->metadata_map[i].title
                            ? strlen(engine->metadata_map[i].title)
                            : 0;
        fwrite(&title_len, sizeof(int), 1, fp);
        if (title_len > 0) {
            fwrite(engine->metadata_map[i].title, sizeof(char), title_len, fp);
        }

        // Handle author
        int author_len = engine->metadata_map[i].author
                             ? strlen(engine->metadata_map[i].author)
                             : 0;
        fwrite(&author_len, sizeof(int), 1, fp);
        if (author_len > 0) {
            fwrite(engine->metadata_map[i].author, sizeof(char), author_len,
                   fp);
        }
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

    pthread_mutex_init(&engine->trie_lock, NULL);

    // 5. Read the document count
    int doc_count;
    fread(&doc_count, sizeof(int), 1, fp);
    engine->doc_count = doc_count;

    // 6. Rebuild the document_map and metadata_map arrays
    char **document_map = malloc(sizeof(char *) * doc_count);
    engine->metadata_map = malloc(sizeof(doc_metadata_t) * doc_count);

    if (document_map == NULL || engine->metadata_map == NULL) {
        if (document_map)
            free(document_map);
        if (engine->metadata_map)
            free(engine->metadata_map);
        fclose(fp);
        return NULL;
    }

    for (int i = 0; i < engine->doc_count; i++) {
        int len;
        fread(&len, sizeof(int), 1, fp);
        char *file_path = malloc(len + 1);
        fread(file_path, sizeof(char), len, fp);
        file_path[len] = '\0';
        document_map[i] = file_path;

        // Rebuild title
        int t_len;
        fread(&t_len, sizeof(int), 1, fp);
        if (t_len > 0) {
            char *title = malloc(t_len + 1);
            fread(title, sizeof(char), t_len, fp);
            title[t_len] = '\0';
            engine->metadata_map[i].title = title;
        } else {
            engine->metadata_map[i].title = strdup("Unknown Title");
        }

        // Rebuild author
        int a_len;
        fread(&a_len, sizeof(int), 1, fp);
        if (a_len > 0) {
            char *author = malloc(a_len + 1);
            fread(author, sizeof(char), a_len, fp);
            author[a_len] = '\0';
            engine->metadata_map[i].author = author;
        } else {
            engine->metadata_map[i].author = strdup("Unknown Author");
        }
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
