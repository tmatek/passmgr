#include "common.h"
#include <stdbool.h>

typedef struct master_pwd_cache {
  bool daemon_running;
  bool password_available;
  char master_password[PASSWD_MAX_LENGTH];
} master_pwd_cache;

master_pwd_cache *get_shared_memory(char *filename);
void run_master_password_daemon(char *filename);
void detach_shared_memory(master_pwd_cache *cache);
