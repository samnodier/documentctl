#ifndef CRAWLER_H
#define CRAWLER_H
#include "toolkit_core.h"

int crawl_directory(const char *path, search_engine_t *engine,
                    void (*callback)(const char *));

#endif // !CRAWLER_H
