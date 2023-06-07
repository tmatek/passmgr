#include <stdbool.h>

#include "common.h"

typedef enum PwdResult {
  ERR_IDENTIFIER_INVALID,
  ERR_PASSWD_GENERATION,
  PASSWD_OK
} PwdResult;

void obtain_master_password(char *master_pwd, bool confirm);
PwdResult generate_random_password(char *password, int byte_count);
PwdResult check_password_identifier(char *identifier);
int find_password_entry(Lines entries, int num_entries, char *identifier);
void password_from_entry(Line password, Line entry);
void entries_to_identifiers(Lines identifiers, Lines entries, int num_entries);
void create_entry(Line entry, char *identifier, char *password);
void handle_password_result(PwdResult result);
