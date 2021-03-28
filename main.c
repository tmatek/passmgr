#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_PWD_ENTRIES 1000
#define MAX_KEY_LENGTH 100
#define MAX_PWD_LENGTH 20

// should not clash with base64 characters
#define KEY_PWD_DELIMITER "|"

// max length of key + length of password + delimiter sign
#define MAX_LINE_LENGTH (MAX_KEY_LENGTH + MAX_PWD_LENGTH + 1)

typedef struct pwd_entry {
  char key[MAX_KEY_LENGTH];
  char password[MAX_PWD_LENGTH];
  bool to_remove;
} pwd_entry;

void print_help() {
  printf("Offline password manager 1.0 (March 28th, 2021).\n\n");
  printf("Store passwords securely in a local file, encrypted with a master "
         "password.\n");
  printf("Remember one password and forget about the rest.\n\n");
  printf("Usage: pass [options] identifier\n");
  printf("  The default action is to copy the password associated with "
         "identifier to your clipboard.\n");
  printf("  If the database has not yet been created, you will be asked for a "
         "master password to create the database.\n");
  printf("  options:\n");
  printf("    -c  create a new password entry in the database, overwriting "
         "existing entry.\n");
  printf("    -d  remove an existing entry from the database.\n");
}

int read_passwords(FILE *db, pwd_entry *entries) {
  char line[MAX_LINE_LENGTH];
  int count = 0;

  while (fgets(line, 1024, db) != NULL) {
    strtok(line, "\n"); // remove trailing new line
    char *identifier = strtok(line, KEY_PWD_DELIMITER);

    pwd_entry entry;
    memcpy(entry.key, identifier, MAX_KEY_LENGTH);

    char *pwd = strtok(NULL, KEY_PWD_DELIMITER);
    if (!pwd) {
      // broken line
      continue;
    }

    memcpy(entry.password, pwd, MAX_PWD_LENGTH);

    entries[count++] = entry;
  }

  return count;
}

bool generate_random_password(pwd_entry *entry) {
  FILE *gen = popen("openssl rand -base64 15", "r");
  if (!gen) {
    return false;
  }

  // read until new line is consumed and then remove the new line
  char *res = fgets(entry->password, 1024, gen);
  strtok(entry->password, "\n");
  pclose(gen);

  return !!res;
}

bool valid_identifier(char *identifier) {
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

    return false;
  }

  return true;
}

int main(int argc, char **argv) {
  char opt;
  bool create_update = false;
  bool delete = false;

  while ((opt = getopt(argc, argv, ":cd")) != -1) {
    switch (opt) {
    case 'c':
      create_update = true;
      break;

    case 'd':
      delete = true;
      break;
    }
  }

  if (optind >= argc || (create_update && delete)) {
    print_help();
    return EXIT_FAILURE;
  }

  char *identifier = argv[optind];
  if (!valid_identifier(identifier)) {
    fprintf(stderr, "Identifier can only be alphanumeric, with underscore "
                    "and/or a dash.\n");
    return EXIT_FAILURE;
  }

  char master_pwd[256] = {'\0'};

  /**
   * Check if database has been created.
   */
  FILE *db = fopen("./passdb", "r");
  if (!db) {
    printf("No database found, creating one now.\n");

    char *pass = getpass("Master password: ");
    strcpy(master_pwd, pass);

    // wait for matching password
    while (strcmp(master_pwd, getpass("Repeat password: ")) != 0)
      ;

    // no database, use OpenSSL to create initial database
    char command[350];
    sprintf(command, "openssl enc -aes-256-cbc -out passdb -pass pass:%s",
            master_pwd);

    db = popen(command, "w");
    if (!db) {
      fprintf(stderr, "Unable to run OpenSSL.\n");
      return EXIT_FAILURE;
    }

    pclose(db);

    printf("Database created, you may now generate passwords.\n");
    return EXIT_SUCCESS;
  } else {
    fclose(db);

    char *pass = getpass("Master password: ");
    strcpy(master_pwd, pass);
  }

  /**
   * The database is there, so decrypt it first.
   */
  char command[350];
  sprintf(command, "openssl enc -aes-256-cbc -d -in passdb -pass pass:%s",
          master_pwd);
  db = popen(command, "r");
  if (!db) {
    fprintf(stderr, "Unable to read database file.\n");
    return EXIT_FAILURE;
  }

  pwd_entry entries[MAX_PWD_ENTRIES];
  pwd_entry *matching_entry = NULL;
  int num_entries = read_passwords(db, entries);
  pclose(db);

  bool pwd_updated = false;
  for (int i = 0; i < num_entries; i++) {
    if (strcmp(identifier, entries[i].key) != 0) {
      continue;
    }

    matching_entry = &entries[i];

    if (create_update) {
      char ok;
      printf("Overwrite existing password in the database? (Y/N): ");
      scanf("%c", &ok);

      if (ok == 'Y' || ok == 'y') {
        pwd_updated = generate_random_password(&entries[i]);
        if (!pwd_updated) {
          fprintf(stderr, "Unable to generate a new password.\n");
          return EXIT_FAILURE;
        }
      } else {
        return EXIT_SUCCESS;
      }
    }
  }

  // no entries found?
  if (!create_update && !matching_entry) {
    printf("No entry found for key \"%s\".\n", identifier);
    return EXIT_SUCCESS;
  }

  // create a brand new entry?
  if (create_update && !pwd_updated) {
    pwd_entry new_entry;
    memcpy(new_entry.key, identifier, MAX_KEY_LENGTH);

    if (!generate_random_password(&new_entry)) {
      fprintf(stderr, "Unable to generate a new password.\n");
      return EXIT_FAILURE;
    }

    matching_entry = &new_entry;
    entries[num_entries++] = new_entry;
  }

  // save the database by re-writing all entries
  sprintf(command, "openssl enc -aes-256-cbc -out passdb -pass pass:%s",
          master_pwd);
  db = popen(command, "w");
  for (int i = 0; i < num_entries; i++) {
    // skip entry?
    if (delete &&(&entries[i] == matching_entry)) {
      continue;
    }

    fprintf(db, "%s%s%s\n", entries[i].key, KEY_PWD_DELIMITER,
            entries[i].password);
  }

  pclose(db);

  // copy password to clipboard
  if (!delete) {
    FILE *cpy = popen("pbcopy", "w");
    if (!cpy) {
      fprintf(stderr, "Unable to copy password to your clipboard.\n");
      return EXIT_FAILURE;
    }

    fprintf(cpy, "%s", matching_entry->password);
    pclose(cpy);

    printf("Password copied to clipboard.\n");
  } else {
    printf("Password removed from database.\n");
  }

  return EXIT_SUCCESS;
}
