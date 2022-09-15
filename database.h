#include <stdbool.h>

typedef enum DatabaseStatus {
  ERR_DB_HOME_DIR,
  ERR_DB_OPEN_FAILED,
  ERR_DB_MASTER_PWD,
  DB_OK
} DatabaseStatus;

// maximum 200 characters per line
typedef char Line[200];

// maximum 1000 lines per database
typedef Line Lines[1000];

bool database_exists();
DatabaseStatus create_database(char *master_pwd);
DatabaseStatus read_database(char *master_pwd, Lines lines, int *lines_read);
DatabaseStatus save_database(char *master_pwd, Lines lines, int num_lines);
