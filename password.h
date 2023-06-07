#include <stdbool.h>

#include "common.h"

typedef enum PwdResult {
  ERR_IDENTIFIER_INVALID,
  ERR_PASSWD_GENERATION,
  PASSWD_OK
} PwdResult;

// again not anticipating passwords longer than 50 characters;
// makes memory management easier
#define PASSWD_MAX_LENGTH 50

// should not clash with base64 characters
#define IDENT_PASSWD_DELIMITER "|"

void obtain_master_password(char *master_pwd, bool confirm);
PwdResult generate_random_password(char *password, int byte_count);
PwdResult check_password_identifier(char *identifier);
int find_password_entry(Lines entries, int num_entries, char *identifier);
void password_from_entry(Line password, Line entry);
void handle_password_result(PwdResult result);
