#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "password.h"

void obtain_master_password(char *master_pwd, bool confirm) {
  char *pass = getpass("Master password: ");
  strcpy(master_pwd, pass);

  // wait for matching password
  while (confirm && strcmp(master_pwd, getpass("Repeat password: ")) != 0)
    ;
}

PwdResult generate_random_password(char *password, int byte_count) {
  char command[50];
  sprintf(command, "openssl rand -base64 %d", byte_count);

  FILE *gen = popen(command, "r");
  if (!gen) {
    return ERR_PASSWD_GENERATION;
  }

  // read until new line is consumed and then remove the new line
  char *res = fgets(password, PASSWD_MAX_LENGTH, gen);
  strtok(password, "\n");
  pclose(gen);

  if (!res) {
    return ERR_PASSWD_GENERATION;
  }

  return PASSWD_OK;
}

PwdResult check_password_identifier(char *identifier) {
  for (char *ptr = identifier; *ptr != '\0'; ptr++) {
    if (*ptr >= 48 && *ptr <= 57)
      continue; // digits
    if (*ptr >= 65 && *ptr <= 90)
      continue; // large letters
    if (*ptr >= 97 && *ptr <= 122)
      continue; // small letters
    if (*ptr == 95)
      continue; // underscore
    if (*ptr == 45)
      continue; // dash

    return ERR_IDENTIFIER_INVALID;
  }

  return PASSWD_OK;
}

int find_password_entry(Lines entries, int num_entries, char *identifier) {
  for (int i = 0; i < num_entries; i++) {
    Line temp;
    memcpy(temp, entries[i], sizeof(Line));

    char *current_identifier = strtok(temp, IDENT_PASSWD_DELIMITER);
    if (strcmp(current_identifier, identifier) == 0) {
      return i;
    }
  }

  return -1;
}

void password_from_entry(Line password, Line entry) {
  memcpy(password, entry, sizeof(Line));
  strtok(password, IDENT_PASSWD_DELIMITER);
  char *second = strtok(NULL, IDENT_PASSWD_DELIMITER);
  memcpy(password, second, PASSWD_MAX_LENGTH);
}

void handle_password_result(PwdResult result) {
  switch (result) {
  case ERR_IDENTIFIER_INVALID:
    fprintf(stderr, "Identifier can only be alphanumeric, with underscore "
                    "and/or a dash.\n");
    exit(EXIT_FAILURE);

  case ERR_PASSWD_GENERATION:
    fprintf(stderr, "Unable to generate a new password.\n");
    exit(EXIT_FAILURE);

  case PASSWD_OK:
    return;
  }
}
