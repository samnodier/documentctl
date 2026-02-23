#include "pdf_processor.h"
#include "debug.h"
#include "glib-object.h"
#include "poppler-document.h"
#include "poppler-page.h"
#include "toolkit_core.h"
#include <ctype.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <poppler.h>
#include <stdlib.h>

void index_pdf_content(search_engine_t *engine, int doc_id,
                       const char *filepath) {
    DEBUG_PRINT("[DEBUG PDF] Opening: %s\n", filepath);

    // Convert to absolute path first
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
    DEBUG_PRINT("[DEBUG PDF] URI: %s\n", uri); // ADD THIS

    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);

    g_free(uri);
    if (doc == NULL) {
        DEBUG_PRINT("[DEBUG PDF] FAILED to open document!\n"); // ADD THIS
        if (error) {
            DEBUG_PRINT("[DEBUG PDF] Error: %s\n", error->message); // ADD THIS
        }
        g_error_free(error);
        return; // ADD THIS - you were missing it!
    }

    char *t = poppler_document_get_title(doc);
    char *a = poppler_document_get_author(doc);
    int num_pages = poppler_document_get_n_pages(doc);

    pthread_mutex_lock(&engine->trie_lock);
    engine->metadata_map[doc_id].title = t ? t : strdup("Unknown Title");
    engine->metadata_map[doc_id].author = a ? a : strdup("Unknown Author");
    pthread_mutex_unlock(&engine->trie_lock);

    DEBUG_PRINT("[DEBUG PDF] Successfully opened, pages: %d\n",
                poppler_document_get_n_pages(doc));
    for (int i = 0; i < num_pages; i++) {
        PopplerPage *page = poppler_document_get_page(doc, i);
        if (!page)
            continue;

        char *page_text = poppler_page_get_text(page);
        if (page_text) {
            char word[100];
            int w_idx = 0;         // Separate index for our small word buffer
            long start_offset = 0; // To track the beginning of a word

            for (size_t j = 0; j < strlen(page_text); j++) {
                char c = page_text[j];
                if (isalnum(c)) {
                    if (w_idx == 0) {     // start of the word
                        start_offset = j; // Record the current index
                    }
                    if (w_idx < 99) {
                        word[w_idx++] = tolower(c);
                    }
                } else {
                    // We hit a space or punctuation
                    if (w_idx > 0) {        // We have letters
                        word[w_idx] = '\0'; // Terminate the string

                        pthread_mutex_lock(&engine->trie_lock);
                        trie_insert(engine->index_root, word, doc_id, i,
                                    start_offset);
                        pthread_mutex_unlock(&engine->trie_lock);

                        w_idx = 0; // Reset for the next word
                    }
                }
            }

            // Final word on the page
            if (w_idx > 0) {
                word[w_idx] = '\0'; // Terminate the string

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

// The "Wrapper" that pthreads requires
void *thread_chunk_worker(void *arg) {
    thread_chunk_t *chunk = (thread_chunk_t *)arg;

    for (int i = chunk->start_index; i < chunk->end_index; i++) {
        char *path = chunk->engine->document_map[i];
        // Print from the backgroudn thread!
        printf("[Thread %lu] Indexing: %s\n", (unsigned long)pthread_self(),
               path);
        // The thread calls indexing function for its assigned range
        index_pdf_content(chunk->engine, i, path);
    }

    free(chunk); // Clean up the task envelope
    return NULL;
}

char *get_snippet(const char *filepath, int page_num, long byte_offset) {
    GError *error = NULL;
    gchar *uri = g_filename_to_uri(filepath, NULL, &error);

    if (uri) {
        PopplerDocument *doc =
            poppler_document_new_from_file(uri, NULL, &error);
        if (doc == NULL) {
            g_warning("Failed to open document: %s", error->message);
            g_error_free(error);
        } else {
            PopplerPage *page = poppler_document_get_page(doc, page_num);
            char *page_text = poppler_page_get_text(page);
            if (page_text) {
                size_t page_len = strlen(page_text);
                long start = (byte_offset > 30) ? (byte_offset - 30) : 0;
                long end = (byte_offset + 30 < (long)page_len)
                               ? (byte_offset + 30)
                               : (long)page_len;

                // Snap START backward to the nearest space
                while (start > 0 && page_text[start] != ' ' &&
                       page_text[start] != '\n') {
                    start--;
                }

                // Snap END forward to the nearest space
                while (end < (long)page_len && page_text[end] != ' ' &&
                       page_text[end] != '\n') {
                    end++;
                }

                size_t length_to_copy = end - start;
                char *result = malloc(length_to_copy + 1);

                // Prevent from going before the beginning of text (index 0)
                strncpy(result, page_text + start, length_to_copy);
                result[length_to_copy] = '\0';
                g_free(page_text);
                g_object_unref(page);
                g_object_unref(doc);
                g_free(uri);

                return result;
            }
        }
    }
    return NULL;
}

void free_snippet(char *snippet) { free(snippet); }
