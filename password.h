#include <stdbool.h>

typedef enum PWDResult {
  ERR_IDENTIFIER_INVALID,
  ERR_PASSWD_GENERATION,
  ERR_CLIPBOARD_COPY,
  PASSWD_OK
} PWDResult;

// again not anticipating passwords longer than 50 characters;
// makes memory management easier
#define PASSWD_MAX_LENGTH 50

// should not clash with base64 characters
#define IDENT_PASSWD_DELIMITER "|"

void obtain_master_password(char *master_pwd, bool confirm);
PWDResult generate_random_password(char *password, int byte_count);
PWDResult check_password_identifier(char *identifier);
PWDResult clipboard_copy(char *password);
void handle_password_result(PWDResult result);
