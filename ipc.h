#include <stdbool.h>

#include "common.h"

typedef struct master_pwd_cache {
  bool password_available;
  char master_password[PASSWD_MAX_LENGTH];
} master_pwd_cache;

master_pwd_cache *get_shared_memory();
void run_master_password_daemon(master_pwd_cache *cache);
void detach_shared_memory(master_pwd_cache *cache);
