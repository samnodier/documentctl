#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


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
  void *pdf_handle;     // Pointer to library-specific PDF object
  uint8_t *raw_buffer;  // Raw PDF file content
  size_t buffer_size;

  // Error handling
  int error_code;
  char *error_message;
} PDF;




// Initialization functions

PDFMetadata* pdf_metadata_create(void) {
  PDFMetadata *metadata = malloc(sizeof(PDFMetadata));
  if (metadata == NULL) {
    return NULL;
  }

  // Initialize all pointers to NULL;
  metadata->title = NULL;
  metadata->author = NULL;
  metadata->subject = NULL;
  metadata->creator = NULL;
  metadata->producer = NULL;
  metadata->creation_date = NULL;
  metadata->modification_date = NULL;
  metadata->keywords = NULL;

  return metadata;
}

// Cleanup function

void pdf_metadata_free(PDFMetadata *metadata) {
  if (metadata == NULL) {
    return;
  }


  free(metadata->title);
  free(metadata->author);
  free(metadata->subject);
  free(metadata->creator);
  free(metadata->producer);
  free(metadata->creation_date);
  free(metadata->modification_date);
  free(metadata->keywords); 

  free(metadata);
}

void pdf_free(PDF *pdf){
  if (pdf == NULL) return;

  free(pdf->filepath);
  free(pdf->filename);
  free(pdf->version);
  free(pdf->owner_password);
  free(pdf->user_password);
  free(pdf->pdf_handle);

  for (size_t i = 0; i<pdf->num_pages; i++) {
    free(pdf->pages[i].content);
    free(pdf->pages[i].raw_data);
  }
  free(pdf->pages);
  pdf_metadata_free(pdf->metadata);

  free(pdf);
}
