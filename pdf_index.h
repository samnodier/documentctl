// Forward declare engine
typedef struct SearchEngine search_engine_t;

void index_pdf_content(search_engine_t *engine, int doc_id,
                       const char *filepath);
