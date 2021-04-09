#include "ipc.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MASTER_PWD_MAX_LENGTH 100
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
  printf("Usage: pass [options] [identifier]\n");
  printf("  The default action is to copy the password associated with "
         "identifier to your clipboard.\n");
  printf("  If the database has not yet been created, you will be asked for a "
         "master password to create the database.\n");
  printf("  options:\n");
  printf("    -l  list all entries in the database.\n");
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

    entry.to_remove = false;
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

bool create_database(char *db_path, char *master_pwd) {
  char *pass = getpass("Master password: ");
  strcpy(master_pwd, pass);

  // wait for matching password
  while (strcmp(master_pwd, getpass("Repeat password: ")) != 0)
    ;

  // no database, use OpenSSL to create initial database
  char *format =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s";
  char command[PATH_MAX + MAX_PWD_LENGTH + sizeof(format)];
  sprintf(command, format, db_path, master_pwd);

  FILE *db = popen(command, "w");
  if (!db) {
    fprintf(stderr, "Unable to create initial database file.\n");
    return false;
  }

  pclose(db);
  return true;
}

bool read_database(char *db_path, char *master_pwd, pwd_entry *entries,
                   int *num_entries) {
  char *format =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -d -in %s -pass pass:%s";
  char command[PATH_MAX + MAX_PWD_LENGTH + sizeof(format)];
  sprintf(command, format, db_path, master_pwd);
  FILE *db = popen(command, "r");
  if (!db) {
    return false;
  }

  *num_entries = read_passwords(db, entries);

  // master password checks out?
  int res = pclose(db);
  if (res > 0) {
    return false;
  }

  return true;
}

bool save_database(char *db_path, char *master_pwd, pwd_entry *entries,
                   int num_entries) {
  char *format =
      "openssl enc -aes-256-cbc -pbkdf2 -iter 100000 -out %s -pass pass:%s";
  char command[PATH_MAX + MAX_PWD_LENGTH + sizeof(format)];
  sprintf(command, format, db_path, master_pwd);
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
#ifdef _WIN32
  FILE *cpy = popen("clip", "w");
#else
  FILE *cpy = popen("pbcopy", "w");
#endif

  if (!cpy) {
    return false;
  }

  fprintf(cpy, "%s", password);
  pclose(cpy);
  return true;
}

/**
 * Sets users home directory for release builds and the current working
 * directory for debug builds.
 */
bool get_db_path(char *db_path) {
#ifndef NDEBUG
  sprintf(db_path, "./passdb");
#else
  char *homedir = getenv("HOME");
  if (!homedir) {
    return false;
  }

  sprintf(db_path, "%s/passdb", homedir);
#endif

  return true;
}

int main(int argc, char **argv) {
  char db_path[PATH_MAX];
  bool path_ok = get_db_path(db_path);
  if (!path_ok) {
    fprintf(stderr, "Unable to find user's home directory.\n");
    return EXIT_FAILURE;
  }

  /**
   * Check if database has been created yet.
   */
  bool created_db = false;
  char init_master_pwd[MASTER_PWD_MAX_LENGTH] = {'\0'};
  FILE *db = fopen(db_path, "r");
  if (!db) {
    printf("No database found, creating one now.\n");
    created_db = create_database(db_path, init_master_pwd);
    if (!created_db) {
      fprintf(stderr, "Unable to create initial database file.\n");
      return EXIT_FAILURE;
    }
  } else {
    fclose(db);
  }

  /**
   * Setup shared memory and run the daemon if shared memory hasn't been created
   * yet.
   */
  master_pwd_cache *cache = get_shared_memory(db_path);
  if (!cache) {
    fprintf(stderr, "Error in process communication.\n");
    return EXIT_FAILURE;
  }

#ifndef _WIN32
  if (!cache->daemon_running) {
    cache->daemon_running = true;
    pid_t pid = fork();
    if (pid == 0) {
      run_master_password_daemon();
      return EXIT_SUCCESS;
    }
  }
#endif

  /**
   * Read the password from shared memory if it is there, otherwise ask for it
   * and cache it.
   */
  if (created_db) {
    strcpy(cache->master_password, init_master_pwd);
  } else if (!cache->password_available) {
    char *pass = getpass("Master password: ");
    strcpy(cache->master_password, pass);
  }

  /**
   * The database is there, so decrypt it first. OpenSSL force quits upon failed
   * decryption due to an invalid passphrase, so we set the password_available
   * property only after successful decoding.
   */
  pwd_entry entries[MAX_PWD_ENTRIES];
  int num_entries;

  bool read_ok =
      read_database(db_path, cache->master_password, entries, &num_entries);
  if (!read_ok) {
    fprintf(stderr, "Unable to read database file.\n");

    // read was not ok, clear the master password to try again
    cache->password_available = false;
    detach_shared_memory(cache);

    return EXIT_FAILURE;
  }

  // starts the cache-clearing timer
  cache->password_available = true;

  // read options and process
  char opt;
  while ((opt = getopt(argc, argv, "c:d:l")) != -1) {
    switch (opt) {
    case 'c':
    case 'd': {
      if (!valid_identifier(optarg)) {
        fprintf(stderr, "Identifier can only be alphanumeric, with underscore "
                        "and/or a dash.\n");
        detach_shared_memory(cache);
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
              detach_shared_memory(cache);
              return EXIT_FAILURE;
            }
          } else {
            detach_shared_memory(cache);
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
          detach_shared_memory(cache);
          return EXIT_FAILURE;
        }

        entries[num_entries++] = new_entry;
        entry = &new_entry;
      }

      // save the database by re-writing all entries
      bool save_ok =
          save_database(db_path, cache->master_password, entries, num_entries);
      if (!save_ok) {
        fprintf(stderr, "Unable to save database file.\n");
        detach_shared_memory(cache);
        return EXIT_FAILURE;
      }

      if (opt == 'c') {
        bool copy_ok = clipboard_copy(entry->password);
        if (!copy_ok) {
          fprintf(stderr, "Unable to copy password to your clipboard.\n");
          detach_shared_memory(cache);
          return EXIT_FAILURE;
        }

        printf("Password copied to clipboard.\n");
      }

      if (opt == 'd') {
        if (entry) {
          printf("Password removed from database.\n");
        } else {
          printf("No entry found for key \"%s\".\n", optarg);
        }
      }

      detach_shared_memory(cache);
      return EXIT_SUCCESS;
    }

    case 'l':
      for (int i = 0; i < num_entries; i++) {
        printf("%s\n", entries[i].key);
      }
      detach_shared_memory(cache);
      return EXIT_SUCCESS;

    case ':':
    case '?':
      print_help();
      detach_shared_memory(cache);
      return EXIT_FAILURE;
    }
  }

  if (argc < 2) {
    print_help();
    detach_shared_memory(cache);
    return EXIT_FAILURE;
  }

  // get single entry
  for (int i = 0; i < num_entries; i++) {
    if (strcmp(argv[1], entries[i].key) == 0) {
      bool copy_ok = clipboard_copy(entries[i].password);
      if (!copy_ok) {
        fprintf(stderr, "Unable to copy password to your clipboard.\n");
        detach_shared_memory(cache);
        return EXIT_FAILURE;
      }

      printf("Password copied to clipboard.\n");
      detach_shared_memory(cache);
      return EXIT_SUCCESS;
    }
  }

  // no entries found?
  printf("No entry found for key \"%s\".\n", argv[1]);
  detach_shared_memory(cache);
  return EXIT_SUCCESS;
}
