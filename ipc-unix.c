#include "ipc.h"
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define CLEAR_CACHED_MASTER_PWD_INTERVAL 60
#define IPC_RESULT_ERROR (-1)

master_pwd_cache *get_shared_memory(char *filename) {
  key_t key = ftok(filename, 0);
  if (key == IPC_RESULT_ERROR) {
    return NULL;
  }

  int shared_block_id = shmget(key, sizeof(master_pwd_cache), 0644 | IPC_CREAT);
  if (shared_block_id == IPC_RESULT_ERROR) {
    return NULL;
  }

  master_pwd_cache *result = shmat(shared_block_id, NULL, 0);
  if (result == (void *)-1) {
    return false;
  }

  return result;
}

/*+
 * The master password daemon runs as a standalone process and caches the
 * correctly entered master password for up to a minute, before clearing it.
 */
void run_master_password_daemon(char *db_path) {
  pid_t sid = setsid();
  if (sid < 0) {
    exit(EXIT_FAILURE);
  }

  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);

  master_pwd_cache *cache = get_shared_memory(db_path);
  if (!cache) {
    exit(EXIT_FAILURE);
  }

  bool prev_available = cache->password_available;
  int start = prev_available ? (int)time(NULL) : 0;

  while (true) {
    sleep(1);

    // password was set? start the timer
    if (cache->password_available && !prev_available) {
      start = (int)time(NULL);
    }

    int now = (int)time(NULL);
    if (start > 0 && now - start >= CLEAR_CACHED_MASTER_PWD_INTERVAL) {
      start = 0;
      cache->password_available = false;
      memset(cache->master_password, 0, MASTER_PWD_MAX_LENGTH);
    }

    prev_available = cache->password_available;
  }

  shmdt(cache);
}

void detach_shared_memory(master_pwd_cache *cache) { shmdt(cache); }
