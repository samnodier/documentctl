#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>   // for PATH_MAX

int crawl_directory(const char *path, void (*callback)(const char *)) {
  DIR *dir;
  // Open the given directory
  struct dirent *de;    // Pointer for directory entry

  char full_path[PATH_MAX];

  dir = opendir(path);

  if (dir == NULL) {
    perror("Could not open the directory");
    return -1;
  }

  // Read entries sequentially until readdir returns NULL
  while ((de = readdir(dir)) != NULL) {
    // Find the last occurence of '.'
    char *dot_pos = strrchr(de->d_name, '.');

    // If the dot is not found or the first char (hidden file)
    // If changed to search for any files, ignore "." and ".."
    if (dot_pos == NULL || dot_pos == de->d_name) continue;

    if (strcasecmp(dot_pos + 1, "pdf") == 0) {
      int result = snprintf(full_path, sizeof(full_path), "%s/%s", path, de->d_name);
      // If output was truncated
      if (result >= sizeof(full_path)) {
        printf("Output truncated (%d chars)", result);
      }
      callback(full_path);     // Call the python function
    }
  }

  // Close directory stream
  closedir(dir);

  return 0;
}
