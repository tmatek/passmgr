#include <stdbool.h>

typedef enum DBResult {
  ERR_DB_HOME_DIR,
  ERR_DB_OPEN_FAILED,
  ERR_DB_MASTER_PWD,
  DB_OK
} DBResult;

// I picked these limits as it makes memory management easier;
// I do not anticipate more than 1000 passwords per database
// or passwords + identifiers so long, that they would exceed
// the 200 character line limit.

// maximum 200 characters per line
typedef char Line[200];

// maximum 1000 lines per database
typedef Line Lines[1000];

bool database_exists();
DBResult create_database(char *master_pwd);
DBResult read_database(char *master_pwd, Lines lines, int *lines_read);
DBResult save_database(char *master_pwd, Lines lines, int num_lines);
void handle_database_result(DBResult result);
