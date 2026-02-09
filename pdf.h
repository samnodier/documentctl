#ifndef PDF_H
#define PDF_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// PDF page structure
typedef struct {
  int page_number;
  float width;
  float height;
  char *content; // text content extracted from the page
  size_t content_length;
  void *raw_data;
  size_t raw_data_size;
} PDFPage;

// PDF metadata structure
typedef struct {
  char *title;
  char *author;
  char *subject;
  char *creator;
  char *producer;
  char *creation_date;
  char *modification_date;
  char *keywords;
} PDFMetadata;

// Main PDF structure
typedef struct {
  // File information
  char *filepath;
  char *filename;
  size_t filesize;

  char *version;

  int num_pages;
  PDFPage *pages;

  PDFMetadata *metadata;

  // Security
  bool is_encrypted;
  bool is_locked;
  char *owner_password;
  char *user_password;

  // Permissions
  bool allow_printing;
  bool allow_copying;
  bool allow_modification;
  bool allow_annotation;

  // Content tracking
  bool has_forms;
  bool has_javascript;
  int num_images;
  int num_fonts;

  // Internal data
  void *pdf_handle;    // Pointer to library-specific PDF object
  uint8_t *raw_buffer; // Raw PDF file content
  size_t buffer_size;

  // Error handling
  int error_code;
  char *error_message;
} PDF;

// PDF Initialization and cleanup
PDF *pdf_create(void);
void pdf_free(PDF *pdf);
void pdf_page_free(PDFPage *page);

// Loading and saving
int pdf_load_from_file(PDF *pdf, const char *filepath);
int pdf_load_from_memory(PDF *pdf, const uint8_t *data, size_t size);
int pdf_save(PDF *pdf, const char *output_path);

// Page operations
PDFPage *pdf_get_page(PDF *pdf, int page_number);
char *pdf_extract_text(PDF *pdf, int page_number);
int pdf_get_page_count(PDF *pdf);

// Metadata operations
PDFMetadata *pdf_metadata_create(void);
void pdf_metadata_free(PDFMetadata *metadata);
int pdf_set_metadata(PDF *pdf, PDFMetadata *metadata);
PDFMetadata *pdf_get_metadata(PDF *pdf);

// Security operations
int pdf_unlock(PDF *pdf, const char *password);
bool pdf_is_encrypted(PDF *pdf);

#endif // !PDF_H
