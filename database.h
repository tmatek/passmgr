#include <stdbool.h>

#include "common.h"

bool database_exists();
bool create_database(char *master_pwd);
bool read_database(char *master_pwd, Lines lines, int *lines_read);
bool save_database(char *master_pwd, Lines lines, int num_lines);
