#include <stdbool.h>

#include "common.h"

typedef enum DBResult {
  ERR_DB_HOME_DIR,
  ERR_DB_OPEN_FAILED,
  ERR_DB_MASTER_PWD,
  DB_OK
} DBResult;

bool database_exists();
DBResult create_database(char *master_pwd);
DBResult read_database(char *master_pwd, Lines lines, int *lines_read);
DBResult save_database(char *master_pwd, Lines lines, int num_lines);
void handle_database_result(DBResult result);
