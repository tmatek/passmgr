#include <stdio.h>

#include "error.h"

PassError last_error;

void print_error() {
  switch (last_error) {
  case ERR_CLIPBOARD_COPY:
    fprintf(stderr, "Unable to copy password to your clipboard.\n");
    break;

  case ERR_IDENTIFIER_INVALID:
    fprintf(stderr, "Identifier can only be alphanumeric, with underscore "
                    "and/or a dash.\n");
    break;

  case ERR_PASSWD_GENERATION:
    fprintf(stderr, "Unable to generate a new password.\n");
    break;

  case ERR_DB_HOME_DIR:
    fprintf(stderr, "Unable to access your home directory.\n");
    break;

  case ERR_DB_OPEN_FAILED:
    fprintf(stderr, "Unable to open database file.\n");
    break;

  case ERR_DB_MASTER_PWD:
    fprintf(stderr, "Unable to decrypt database file.\n");
    break;

  case ERR_SHARED_MEM:
    fprintf(stderr, "Error allocating shared memory.\n");
    break;

  case ERR_OPENSSL_INVALID:
    fprintf(stderr, "Invalid version of OpenSSL detected. Please use at least "
                    "version 3.0.\n");
    break;
  }
}
