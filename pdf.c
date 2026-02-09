#include "pdf.h"
#include <stdlib.h>

// Initialization functions

PDFMetadata *pdf_metadata_create(void) {
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

void pdf_free(PDF *pdf) {
  if (pdf == NULL)
    return;

  free(pdf->filepath);
  free(pdf->filename);
  free(pdf->version);
  free(pdf->owner_password);
  free(pdf->user_password);
  free(pdf->pdf_handle);

  for (int i = 0; i < pdf->num_pages; i++) {
    free(pdf->pages[i].content);
    free(pdf->pages[i].raw_data);
  }
  free(pdf->pages);
  pdf_metadata_free(pdf->metadata);

  free(pdf);
}
