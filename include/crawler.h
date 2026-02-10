#ifndef CRAWLER_H
#define CRAWLER_H
#include "engine.h"

int crawl_directory(const char *path, search_engine_t *engine,
                    void (*callback)(const char *));

#endif // !CRAWLER_H
