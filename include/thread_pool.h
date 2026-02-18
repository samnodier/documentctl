#include <limits.h>
#include <pthread_t.h>

typedef struct QueueNode {
  char path[PATH_MAX];
    queue_node_t next;
} queue_node_t;

typedef struct ThreadPool {
  pthread_t workers;
  int worker_count;

  queue_node_t *head;
  queue_node_t *tail;
  int queue_size;

  pthread_mutex_t lock
} thread_pool_t;


