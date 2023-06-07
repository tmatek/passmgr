#include "ipc.h"
#include <stdlib.h>

master_pwd_cache *get_shared_memory(char *filename) {
  master_pwd_cache *cache =
      (master_pwd_cache *)malloc(sizeof(master_pwd_cache));

  cache->password_available = false;
  return cache;
}

void run_master_password_daemon(master_pwd_cache *cache) { return; }

void detach_shared_memory(master_pwd_cache *cache) { free(cache); }
