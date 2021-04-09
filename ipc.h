#include <stdbool.h>

#define MASTER_PWD_MAX_LENGTH 100

typedef struct master_pwd_cache {
  bool daemon_running;
  bool password_available;
  char master_password[MASTER_PWD_MAX_LENGTH];
} master_pwd_cache;

master_pwd_cache *get_shared_memory(char *filename);
void run_master_password_daemon();
void detach_shared_memory(master_pwd_cache *cache);
