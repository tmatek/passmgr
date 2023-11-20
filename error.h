typedef enum PassError {
  ERR_CLIPBOARD_COPY,
  ERR_DB_HOME_DIR,
  ERR_DB_OPEN_FAILED,
  ERR_DB_MASTER_PWD,
  ERR_IDENTIFIER_INVALID,
  ERR_PASSWD_GENERATION,
  ERR_SHARED_MEM,
  ERR_OPENSSL_INVALID,
} PassError;

extern PassError last_error;

void print_error();
