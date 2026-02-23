#ifndef PDF_PROCESSOR_H
#define PDF_PROCESSOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Forward declare engine
typedef struct SearchEngine search_engine_t;

typedef struct {
    search_engine_t *engine;
    int start_index;
    int end_index;
} thread_chunk_t;

// Essential functions
void index_pdf_content(search_engine_t *engine, int doc_id,
                       const char *filepath);
void *thread_chunk_worker(void *arg);

// Snippet logic (for the search results)
char *get_snippet(const char *filepath, int page_num, long byte_offset);
void free_snippet(char *snippet);

#endif // !PDF_PROCESSOR_H
