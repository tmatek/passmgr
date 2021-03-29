#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_PWD_ENTRIES 1000
#define MAX_KEY_LENGTH 100
#define MAX_PWD_LENGTH 30

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
  printf("    -l  list all entires in the database.\n");
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

bool create_database(char *master_pwd) {
  char *pass = getpass("Master password: ");
  strcpy(master_pwd, pass);

  // wait for matching password
  while (strcmp(master_pwd, getpass("Repeat password: ")) != 0)
    ;

  // no database, use OpenSSL to create initial database
  char command[350];
  sprintf(command, "openssl enc -aes-256-cbc -out passdb -pass pass:%s",
          master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    fprintf(stderr, "Unable to create initial database file.\n");
    return false;
  }

  pclose(db);
  return true;
}

bool read_database(char *master_pwd, pwd_entry *entries, int *num_entries) {
  char command[350];
  sprintf(command, "openssl enc -aes-256-cbc -d -in passdb -pass pass:%s",
          master_pwd);
  FILE *db = popen(command, "r");
  if (!db) {
    return false;
  }

  *num_entries = read_passwords(db, entries);

  // master password checks out?
  int res = pclose(db);
  if (res) {
    return false;
  }

  return true;
}

bool save_database(char *master_pwd, pwd_entry *entries, int num_entries) {
  char command[350];
  sprintf(command, "openssl enc -aes-256-cbc -out passdb -pass pass:%s",
          master_pwd);
  FILE *db = popen(command, "w");
  if (!db) {
    return false;
  }

  for (int i = 0; i < num_entries; i++) {
    // skip entry?
    if (entries[i].to_remove) {
      continue;
    }

    fprintf(db, "%s%s%s\n", entries[i].key, KEY_PWD_DELIMITER,
            entries[i].password);
  }

  pclose(db);
  return true;
}

bool clipboard_copy(char *password) {
  FILE *cpy = popen("pbcopy", "w");
  if (!cpy) {
    return false;
  }

  fprintf(cpy, "%s", password);
  pclose(cpy);
  return true;
}

int main(int argc, char **argv) {
  char master_pwd[256] = {'\0'};

  /**
   * Check if database has been created.
   */
  FILE *db = fopen("./passdb", "r");
  if (!db) {
    printf("No database found, creating one now.\n");
    bool db_ok = create_database(master_pwd);
    if (!db_ok) {
      fprintf(stderr, "Unable to create initial database file.\n");
      return EXIT_FAILURE;
    }
  } else {
    fclose(db);

    char *pass = getpass("Master password: ");
    strcpy(master_pwd, pass);
  }

  /**
   * The database is there, so decrypt it first.
   */
  pwd_entry entries[MAX_PWD_ENTRIES];
  int num_entries;

  bool read_ok = read_database(master_pwd, entries, &num_entries);
  if (!read_ok) {
    fprintf(stderr, "Unable to read database file.\n");
    return EXIT_FAILURE;
  }

  // read options and process
  char opt;
  while ((opt = getopt(argc, argv, "c:d:l")) != -1) {
    switch (opt) {
    case 'c':
    case 'd': {
      if (!valid_identifier(optarg)) {
        fprintf(stderr, "Identifier can only be alphanumeric, with underscore "
                        "and/or a dash.\n");
        return EXIT_FAILURE;
      }

      bool pwd_updated = false;
      pwd_entry *entry = NULL;

      for (int i = 0; i < num_entries; i++) {
        if (strcmp(optarg, entries[i].key) != 0) {
          continue;
        }

        entry = &entries[i];

        if (opt == 'd') {
          entries[i].to_remove = true;
        }

        if (opt == 'c') {
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

      // create a brand new entry?
      if (opt == 'c' && !pwd_updated) {
        pwd_entry new_entry;
        memcpy(new_entry.key, optarg, MAX_KEY_LENGTH);

        if (!generate_random_password(&new_entry)) {
          fprintf(stderr, "Unable to generate a new password.\n");
          return EXIT_FAILURE;
        }

        entries[num_entries++] = new_entry;
        entry = &new_entry;
      }

      // save the database by re-writing all entries
      bool save_ok = save_database(master_pwd, entries, num_entries);
      if (!save_ok) {
        fprintf(stderr, "Unable to save database file.\n");
        return EXIT_FAILURE;
      }

      if (opt == 'c') {
        bool copy_ok = clipboard_copy(entry->password);
        if (!copy_ok) {
          fprintf(stderr, "Unable to copy password to your clipboard.\n");
          return EXIT_FAILURE;
        }

        printf("Password copied to clipboard.\n");
      }

      if (opt == 'd') {
        printf("Password removed from database.\n");
      }

      return EXIT_SUCCESS;
    }

    case 'l':
      for (int i = 0; i < num_entries; i++) {
        printf("%s\n", entries[i].key);
      }
      return EXIT_SUCCESS;

    case ':':
    case '?':
      print_help();
      return EXIT_FAILURE;
    }
  }

  if (argc < 2) {
    print_help();
    return EXIT_FAILURE;
  }

  // get single entry
  for (int i = 0; i < num_entries; i++) {
    if (strcmp(argv[1], entries[i].key) == 0) {
      bool copy_ok = clipboard_copy(entries[i].password);
      if (!copy_ok) {
        fprintf(stderr, "Unable to copy password to your clipboard.\n");
        return EXIT_FAILURE;
      }

      printf("Password copied to clipboard.\n");
      return EXIT_SUCCESS;
    }
  }

  // no entries found?
  printf("No entry found for key \"%s\".\n", argv[1]);
  return EXIT_SUCCESS;
}
