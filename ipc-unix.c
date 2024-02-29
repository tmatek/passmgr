#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>

#include "database.h"
#include "error.h"
#include "inout.h"
#include "ipc.h"

#ifndef NDEBUG
#define CLEAR_CACHED_MASTER_PWD_INTERVAL 30
#else
#define CLEAR_CACHED_MASTER_PWD_INTERVAL 180
#endif

#define CLEAR_CLIPBOARD_AFTER_SECONDS 30

master_pwd_cache *get_shared_memory() {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return NULL;
  }

  key_t key = ftok(db_path, 0);
  if (key < 0) {
    last_error = ERR_SHARED_MEM;
    return NULL;
  }

  int shared_block_id = shmget(key, sizeof(master_pwd_cache), 0644 | IPC_CREAT);
  if (shared_block_id < 0) {
    last_error = ERR_SHARED_MEM;
    return NULL;
  }

  master_pwd_cache *result = shmat(shared_block_id, NULL, 0);
  if (result < 0) {
    last_error = ERR_SHARED_MEM;
    return NULL;
  }

  return result;
}

/*+
 * The master password daemon runs as a standalone process and caches the
 * correctly entered master password for up three minutes, before clearing it.
 */
void run_master_password_daemon(master_pwd_cache *cache) {
  // parent
  pid_t pid = fork();
  if (pid > 0) {
    return;
  }

  // child from here on
  pid_t sid = setsid();
  if (sid < 0) {
    detach_shared_memory(cache);
    exit(EXIT_FAILURE);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  sleep(CLEAR_CACHED_MASTER_PWD_INTERVAL);
  memset(cache->master_password, 0, PASSWD_MAX_LENGTH);
  cache->password_available = false;

  detach_shared_memory(cache);
  exit(EXIT_SUCCESS);
}

/**
 * The clipboard cleaner daemon runs as a standalone process and clears the
 * copied password from the clipboard after short time.
 */
void run_clipboard_cleaner_daemon() {
  // parent
  pid_t pid = fork();
  if (pid > 0) {
    return;
  }

  // child from here on
  pid_t sid = setsid();
  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  sleep(CLEAR_CLIPBOARD_AFTER_SECONDS);

  if (clipboard_clear()) {
    exit(EXIT_SUCCESS);
  }

  exit(EXIT_FAILURE);
}

void detach_shared_memory(master_pwd_cache *cache) { shmdt(cache); }
