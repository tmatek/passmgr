#include <stdbool.h>

#include "common.h"

bool openssl_valid();
bool get_db_path(char *db_path);
bool database_exists();
bool create_database(char *master_pwd);
bool read_database(char *master_pwd, Lines lines, int *lines_read);
bool save_database(char *master_pwd, Lines lines, int num_lines);
