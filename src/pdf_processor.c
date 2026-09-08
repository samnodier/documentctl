#include "pdf_processor.h"
#include "debug.h"
#include "toolkit_core.h"
#include <ctype.h>
#include <glib.h>
#include <poppler.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void index_pdf_content(search_engine_t *engine, int doc_id,
                       const char *filepath) {
    DEBUG_PRINT("[DEBUG PDF] Opening: %s\n", filepath);

    char *abs_path = g_canonicalize_filename(filepath, NULL);
    GError *error = NULL;
    gchar *uri = g_filename_to_uri(abs_path, NULL, &error);
    g_free(abs_path);

    if (!uri) {
        DEBUG_PRINT("[DEBUG PDF] FAILED to convert filepath to URI!\n");
        if (error) {
            DEBUG_PRINT("[DEBUG PDF] Error: %s\n", error->message);
            g_error_free(error);
        }
        return;
    }
    DEBUG_PRINT("[DEBUG PDF] URI: %s\n", uri);

    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);
    if (doc == NULL) {
        DEBUG_PRINT("[DEBUG PDF] FAILED to open document!\n");
        if (error) {
            DEBUG_PRINT("[DEBUG PDF] Error: %s\n", error->message);
            g_error_free(error);
        }
        return;
    }

    char *t = poppler_document_get_title(doc);
    char *a = poppler_document_get_author(doc);
    int num_pages = poppler_document_get_n_pages(doc);

    pthread_mutex_lock(&engine->trie_lock);
    engine->metadata_map[doc_id].title = t ? t : strdup("Unknown Title");
    engine->metadata_map[doc_id].author = a ? a : strdup("Unknown Author");
    pthread_mutex_unlock(&engine->trie_lock);

    DEBUG_PRINT("[DEBUG PDF] Successfully opened, pages: %d\n", num_pages);
    for (int i = 0; i < num_pages; i++) {
        PopplerPage *page = poppler_document_get_page(doc, i);
        if (!page)
            continue;

        char *page_text = poppler_page_get_text(page);
        if (page_text) {
            char word[100];
            int w_idx = 0;
            long start_offset = 0;
            size_t page_len = strlen(page_text);

            for (size_t j = 0; j < page_len; j++) {
                unsigned char c = (unsigned char)page_text[j];
                if (isalnum(c)) {
                    if (w_idx == 0) {
                        start_offset = (long)j;
                    }
                    if (w_idx < 99) {
                        word[w_idx++] = (char)tolower(c);
                    }
                } else {
                    if (w_idx > 0) {
                        word[w_idx] = '\0';

                        pthread_mutex_lock(&engine->trie_lock);
                        trie_insert(engine->index_root, word, doc_id, i,
                                    start_offset);
                        pthread_mutex_unlock(&engine->trie_lock);

                        w_idx = 0;
                    }
                }
            }

            if (w_idx > 0) {
                word[w_idx] = '\0';

                pthread_mutex_lock(&engine->trie_lock);
                trie_insert(engine->index_root, word, doc_id, i, start_offset);
                pthread_mutex_unlock(&engine->trie_lock);
            }
            g_free(page_text);
        }
        g_object_unref(page);
    }
    g_object_unref(doc);
}

void *thread_chunk_worker(void *arg) {
    thread_chunk_t *chunk = (thread_chunk_t *)arg;

    for (int i = chunk->start_index; i < chunk->end_index; i++) {
        char *path = chunk->engine->document_map[i];
        printf("[Thread %lu] Indexing: %s\n", (unsigned long)pthread_self(),
               path);
        fflush(stdout);
        index_pdf_content(chunk->engine, i, path);
    }

    free(chunk);
    return NULL;
}

char *get_snippet(const char *filepath, int page_num, long byte_offset) {
    GError *error = NULL;
    char *abs_path = g_canonicalize_filename(filepath, NULL);
    gchar *uri = g_filename_to_uri(abs_path, NULL, &error);
    g_free(abs_path);

    if (!uri) {
        if (error)
            g_error_free(error);
        return NULL;
    }

    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);
    g_free(uri);
    if (doc == NULL) {
        if (error)
            g_error_free(error);
        return NULL;
    }

    PopplerPage *page = poppler_document_get_page(doc, page_num);
    if (!page) {
        g_object_unref(doc);
        return NULL;
    }

    char *page_text = poppler_page_get_text(page);
    if (!page_text) {
        g_object_unref(page);
        g_object_unref(doc);
        return NULL;
    }

    size_t page_len = strlen(page_text);
    long start = (byte_offset > 30) ? (byte_offset - 30) : 0;
    long end = (byte_offset + 30 < (long)page_len) ? (byte_offset + 30)
                                                   : (long)page_len;

    while (start > 0 && page_text[start] != ' ' && page_text[start] != '\n') {
        start--;
    }

    while (end < (long)page_len && page_text[end] != ' ' &&
           page_text[end] != '\n') {
        end++;
    }

    size_t length_to_copy = (size_t)(end - start);
    char *result = malloc(length_to_copy + 1);
    if (result == NULL) {
        g_free(page_text);
        g_object_unref(page);
        g_object_unref(doc);
        return NULL;
    }

    memcpy(result, page_text + start, length_to_copy);
    result[length_to_copy] = '\0';

    g_free(page_text);
    g_object_unref(page);
    g_object_unref(doc);
    return result;
}

void free_snippet(char *snippet) { free(snippet); }
