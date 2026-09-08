#include "crawler.h"
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

int crawl_directory(const char *path, search_engine_t *engine,
                    void (*callback)(const char *)) {
  DIR *dir = opendir(path);
  if (dir == NULL) {
    perror("Could not open the directory");
    return -1;
  }

  struct dirent *de;
  while ((de = readdir(dir)) != NULL) {
    if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0)
      continue;

    char full_path[PATH_MAX];
    int n = snprintf(full_path, sizeof(full_path), "%s%s%s", path,
                     (path[0] != '\0' && path[strlen(path) - 1] == '/') ? ""
                                                                        : "/",
                     de->d_name);
    if (n < 0 || (size_t)n >= sizeof(full_path)) {
      fprintf(stderr, "Path too long, skipping: %s/%s\n", path, de->d_name);
      continue;
    }

    int is_dir = 0;
    int is_reg = 0;
    if (de->d_type == DT_DIR) {
      is_dir = 1;
    } else if (de->d_type == DT_REG) {
      is_reg = 1;
    } else {
      /* DT_UNKNOWN (common on some filesystems) and symlinks need stat(). */
      struct stat st;
      if (stat(full_path, &st) != 0)
        continue;
      is_dir = S_ISDIR(st.st_mode);
      is_reg = S_ISREG(st.st_mode);
    }

    if (is_dir) {
      if (crawl_directory(full_path, engine, callback) != 0) {
        closedir(dir);
        return -1;
      }
    } else if (is_reg) {
      char *dot_pos = strrchr(de->d_name, '.');
      if (dot_pos == NULL || dot_pos == de->d_name)
        continue;

      if (strcasecmp(dot_pos + 1, "pdf") != 0)
        continue;

      if (engine->doc_count >= engine->doc_capacity) {
        if (engine_grow_capacity(engine) != 0) {
          closedir(dir);
          return -1;
        }
      }

      engine->document_map[engine->doc_count] = strdup(full_path);
      if (engine->document_map[engine->doc_count] == NULL) {
        perror("strdup failed");
        closedir(dir);
        return -1;
      }
      engine->doc_count++;
      if (callback)
        callback(full_path);
    }
  }

  closedir(dir);
  return 0;
}
