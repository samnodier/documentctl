#include "crawler.h"
#include <dirent.h>
#include <limits.h> // for PATH_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

int crawl_directory(const char *path, search_engine_t *engine,
                    void (*callback)(const char *)) {

  DIR *dir;
  // Open the given directory
  struct dirent *de; // Pointer for directory entry

  char full_path[PATH_MAX];

  dir = opendir(path);

  if (dir == NULL) {
    perror("Could not open the directory");
    return -1;
  }

  // Read entries sequentially until readdir returns NULL
  while ((de = readdir(dir)) != NULL) {
    // If changed to search for any files, ignore "." and ".."
    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
      continue;

    // If de->d_type is a directory
    if (de->d_type == DT_DIR) {
      char sub_path[PATH_MAX];

      int result = snprintf(
          sub_path, sizeof(sub_path), "%s%s%s", path,
          (strlen(path) > 0 && path[strlen(path) - 1] == '/') ? "" : "/",
          de->d_name);
      if (result < 0) {
        perror("Encoding/error");
      } else if ((size_t)result >= sizeof(sub_path)) {
        printf("Directory truncated (%d chars)", result);
      }
      crawl_directory(sub_path, engine, callback);
    } else if (de->d_type == DT_REG) { // This is a file

      // Find the last occurence of '.'
      char *dot_pos = strrchr(de->d_name, '.');

      // If the dot is not found or the first char (hidden file)
      if (dot_pos == NULL || dot_pos == de->d_name)
        continue;

      // If the file is a PDF
      if (strcasecmp(dot_pos + 1, "pdf") == 0) {
        int result =
            snprintf(full_path, sizeof(full_path), "%s/%s", path, de->d_name);
        // If output was truncated
        if (result < 0) {
          perror("Encoding/error");
        } else if ((size_t)result >= sizeof(full_path)) {
          printf("Output truncated (%d chars)", result);
        }
        if (engine->doc_count >= engine->doc_capacity) {
          engine->doc_capacity *= 2;
          char **temp_map = realloc(engine->document_map,
                                    sizeof(char *) * engine->doc_capacity);
          if (temp_map != NULL) {
            engine->document_map = temp_map;
          } else {
            // Handle error: memory realloc failed
            perror("realloc failed");
            return -1;
          }
        }
        engine->document_map[engine->doc_count] = strdup(full_path);
        // int doc_id = engine->doc_count;
        engine->doc_count++;
        callback(full_path); // Call the python function
      }
    }
  }

  // Close directory stream
  closedir(dir);

  return 0;
}
