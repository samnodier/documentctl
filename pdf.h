#ifndef PDF_H
#define PDF_H

// PDF Initialization and cleanup
PDF* pdf_create(void);
void pdf_free(PDF *pdf);
void pdf_page_free(PDFPage *page);

// Loading and saving
int pdf_load_from_file(PDF *pdf, const char *filepath);
int pdf_load_from_memory(PDF *pdf, const uint8_t *data, size_t size);
int pdf_save(PDF *pdf, const char *output_path);

// Page operations
PDFPage* pdf_get_page(PDF *pdf, int page_number);
char *pdf_extract_text(PDF *pdf, int page_number);
int pdf_get_page_count(PDF *pdf);

// Metadata operations
PDFMetadata* pdf_metadata_create(void);
void pdf_metadata_free(PDFMetadata *metadata);
int pdf_set_metadata(PDF *pdf, PDFMetadata *metadata);
PDFMetadata* pdf_get_metadata(PDF *pdf);

// Security operations
int pdf_unlock(PDF *pdf, const char *password);
bool pdf_is_encrypted(PDF *pdf);

#endif // !PDF_H
