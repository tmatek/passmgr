#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "database.h"
#include "error.h"

/**
 * Sets users home directory for release builds and the current working
 * directory for debug builds.
 */
bool get_db_path(char *db_path) {
#ifndef NDEBUG
  sprintf(db_path, "./passdb");
#else
  char *homedir = getenv("HOME");
  if (!homedir) {
    return false;
  }

  sprintf(db_path, "%s/passdb", homedir);
#endif

  return true;
}

bool database_exists() {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return false;
  }

  FILE *db = fopen(db_path, "r");
  if (!db) {
    return false;
  }

  fclose(db);
  return true;
}

bool create_database(char *master_pwd) {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    last_error = ERR_DB_HOME_DIR;
    return false;
  }

  // no database, use OpenSSL to create initial database
  char command[FS_MAX_PATH_LENGTH];
  sprintf(command,
          "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s",
          db_path, master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    last_error = ERR_DB_OPEN_FAILED;
    return false;
  }

  pclose(db);
  return true;
}

bool read_database(char *master_pwd, Lines lines, int *lines_read) {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    last_error = ERR_DB_HOME_DIR;
    return false;
  }

  char command[FS_MAX_PATH_LENGTH];
  sprintf(
      command,
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -d -in %s -pass pass:%s",
      db_path, master_pwd);

  FILE *db = popen(command, "r");
  if (!db) {
    last_error = ERR_DB_OPEN_FAILED;
    return false;
  }

  Line line;
  int count = 0;

  while (fgets(line, sizeof(line), db) != NULL) {
    strtok(line, "\n"); // remove trailing new line
    memcpy(lines[count++], line, sizeof(line));
  }

  *lines_read = count;

  // master password checks out?
  int res = pclose(db);
  if (res > 0) {
    last_error = ERR_DB_MASTER_PWD;
    return false;
  }

  return true;
}

bool save_database(char *master_pwd, Lines lines, int num_lines) {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    last_error = ERR_DB_HOME_DIR;
    return false;
  }

  char command[FS_MAX_PATH_LENGTH];
  sprintf(command,
          "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s",
          db_path, master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    last_error = ERR_DB_OPEN_FAILED;
    return false;
  }

  for (int i = 0; i < num_lines; i++) {
    fprintf(db, "%s\n", lines[i]);
  }

  pclose(db);
  return true;
}
