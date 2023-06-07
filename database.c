#include "database.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

DBResult create_database(char *master_pwd) {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return ERR_DB_HOME_DIR;
  }

  // no database, use OpenSSL to create initial database
  char command[FS_MAX_PATH_LENGTH];
  sprintf(command,
          "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s",
          db_path, master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    return ERR_DB_OPEN_FAILED;
  }

  pclose(db);
  return DB_OK;
}

DBResult read_database(char *master_pwd, Lines lines, int *lines_read) {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return ERR_DB_HOME_DIR;
  }

  char command[FS_MAX_PATH_LENGTH];
  sprintf(
      command,
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -d -in %s -pass pass:%s",
      db_path, master_pwd);

  FILE *db = popen(command, "r");
  if (!db) {
    return ERR_DB_OPEN_FAILED;
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
    return ERR_DB_MASTER_PWD;
  }

  return DB_OK;
}

DBResult save_database(char *master_pwd, Lines lines, int num_lines) {
  char db_path[FS_MAX_PATH_LENGTH];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return ERR_DB_HOME_DIR;
  }

  char command[FS_MAX_PATH_LENGTH];
  sprintf(command,
          "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s",
          db_path, master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    return ERR_DB_OPEN_FAILED;
  }

  for (int i = 0; i < num_lines; i++) {
    fprintf(db, "%s\n", lines[i]);
  }

  pclose(db);
  return DB_OK;
}

void handle_database_result(DBResult result) {
  switch (result) {
  case ERR_DB_HOME_DIR:
    fprintf(stderr, "Unable to access your home directory.\n");
    exit(EXIT_FAILURE);

  case ERR_DB_OPEN_FAILED:
    fprintf(stderr, "Unable to open database file.\n");
    exit(EXIT_FAILURE);

  case ERR_DB_MASTER_PWD:
    fprintf(stderr, "Unable to decrypt database file.\n");
    exit(EXIT_FAILURE);

  case DB_OK:
    return;
  }
}
