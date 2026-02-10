#include "pdf_processor.h"
#include "engine.h"
#include "glib-object.h"
#include "poppler-document.h"
#include "poppler-page.h"
#include <ctype.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <poppler.h>
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

void index_pdf_content(search_engine_t *engine, int doc_id,
                       const char *filepath) {
  GError *error = NULL;
  gchar *uri = g_filename_to_uri(filepath, NULL, &error);

  if (uri) {
    PopplerDocument *doc = poppler_document_new_from_file(uri, NULL, &error);
    if (doc == NULL) {
      g_warning("Failed to open document: %s", error->message);
      g_error_free(error);
    } else {
      int num_pages = poppler_document_get_n_pages(doc);
      for (int i = 0; i < num_pages; i++) {
        PopplerPage *page = poppler_document_get_page(doc, i);
        char *page_text = poppler_page_get_text(page);
        if (page_text) {
          char word[100];
          int w_idx = 0; // Separate index for our small word buffer
          for (size_t j = 0; j < strlen(page_text); j++) {
            char c = page_text[j];
            if (isalnum(c)) {
              if (w_idx < 99) {
                word[w_idx++] = tolower(c);
              }
            } else {
              // We hit a space or punctuation
              if (w_idx > 0) {      // We have letters
                word[w_idx] = '\0'; // Terminate the string
                trie_insert(engine->index_root, word, doc_id, i);
                w_idx = 0; // Reset for the next word
              }
            }
          }
          if (w_idx > 0) {
            word[w_idx] = '\0'; // Terminate the string
            trie_insert(engine->index_root, word, doc_id, i);
          }
          g_free(page_text);
        }
        g_object_unref(page);
      }
      g_object_unref(doc);
    }
  }
}
