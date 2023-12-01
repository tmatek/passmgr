#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#endif

#include "error.h"
#include "password.h"

// should not clash with base64 characters or user provided passwords
#define IDENT_PASSWD_DELIMITER "|"

#ifdef _WIN32
char *getpass(const char *prompt) {
  printf(prompt);

  static char password[PASSWD_MAX_LENGTH];
  memset(password, '\0', sizeof(password));

  int count = 0;
  char in;
  while ((in = _getch()) != '\r' && in != EOF) {
    // Ctrl-C
    if (in == '\x3') {
      GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
    }

    // Backspace
    if (in == '\b' && count > 0) {
      printf("\b \b");
      count--;
    }

    if (in != '\b') {
      printf("*");
      password[count++] = in;
    }

    if (count >= PASSWD_MAX_LENGTH - 1) {
      // cannot store any more, stop here
      break;
    }
  }

  password[count++] = '\0';
  printf("\n");

  return password;
}
#endif

void obtain_master_password(char *master_pwd, bool confirm) {
  char *pass = getpass("Master password: ");
  strcpy(master_pwd, pass);

  // wait for matching password
  while (confirm && strcmp(master_pwd, getpass("Repeat password: ")) != 0)
    ;
}

bool obtain_user_password(char *password) {
  // ensure user provided password does not clash with delimiter
  char *pass = getpass("Secret: ");
  strcpy(password, pass);

  if (strstr(password, IDENT_PASSWD_DELIMITER) != NULL) {
    last_error = ERR_PASSWD_INVALID;
    return false;
  }

  // wait for matching password
  while (strcmp(password, getpass("Repeat secret: ")) != 0)
    ;

  return true;
}

bool generate_random_password(char *password, int byte_count) {
  char command[50];
  sprintf(command, "openssl rand -base64 %d", byte_count);

  FILE *gen = popen(command, "r");
  if (!gen) {
    last_error = ERR_PASSWD_GENERATION;
    return false;
  }

  // read until new line is consumed and then remove the new line
  char *res = fgets(password, PASSWD_MAX_LENGTH, gen);
  strtok(password, "\n");
  pclose(gen);

  if (!res) {
    last_error = ERR_PASSWD_GENERATION;
    return false;
  }

  return true;
}

bool check_password_identifier(char *identifier) {
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

    last_error = ERR_IDENTIFIER_INVALID;
    return false;
  }

  return true;
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

void entries_to_identifiers(Lines identifiers, Lines entries, int num_entries) {
  for (int i = 0; i < num_entries; i++) {
    Line temp;
    memcpy(temp, entries[i], sizeof(Line));

    char *identifier = strtok(temp, IDENT_PASSWD_DELIMITER);
    memcpy(identifiers[i], identifier, sizeof(Line));
  }
}

void create_entry(Line entry, char *identifier, char *password) {
  sprintf(entry, "%s%s%s", identifier, IDENT_PASSWD_DELIMITER, password);
}
