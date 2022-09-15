#include "database.h"
#include <limits.h>
#include <stdio.h>
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
  char db_path[PATH_MAX];
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

DatabaseStatus create_database(char *master_pwd) {
  char db_path[PATH_MAX];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return ERR_DB_HOME_DIR;
  }

  // no database, use OpenSSL to create initial database
  char *format =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s";
  char command[PATH_MAX + sizeof(master_pwd) + sizeof(format)];
  sprintf(command, format, db_path, master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    return ERR_DB_OPEN_FAILED;
  }

  pclose(db);
  return DB_OK;
}

DatabaseStatus read_database(char *master_pwd, Lines lines, int *lines_read) {
  char db_path[PATH_MAX];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return ERR_DB_HOME_DIR;
  }

  char *format =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -d -in %s -pass pass:%s";
  char command[PATH_MAX + sizeof(master_pwd) + sizeof(format)];
  sprintf(command, format, db_path, master_pwd);

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

DatabaseStatus save_database(char *master_pwd, Lines lines, int num_lines) {
  char db_path[PATH_MAX];
  bool path_ok = get_db_path(db_path);

  if (!path_ok) {
    return ERR_DB_HOME_DIR;
  }

  char *format =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s";
  char command[PATH_MAX + sizeof(master_pwd) + sizeof(format)];
  sprintf(command, format, db_path, master_pwd);

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
