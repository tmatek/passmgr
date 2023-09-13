#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "database.h"
#include "error.h"
#include "inout.h"
#include "ipc.h"
#include "password.h"

void ensure_initial_database(master_pwd_cache *cache);
void ensure_master_password(master_pwd_cache *cache);
void add_new_password(master_pwd_cache *cache, char *identifier);
void delete_password(master_pwd_cache *cache, char *identifier);
void retrieve_password(master_pwd_cache *cache, char *identifier);
void list_passwords(master_pwd_cache *cache);

int main(int argc, char **argv) {
  InputArgs args = parse_command_line(argc, argv);

  // handle invalid command-line inputs
  if (args.command == CMD_NONE) {
    print_help();
    return EXIT_FAILURE;
  }

  if (args.command == CMD_ADD_PASSWD && !args.identifier) {
    print_help();
    return EXIT_FAILURE;
  }

  if (args.command == CMD_COPY_PASSWD && !args.identifier) {
    print_help();
    return EXIT_FAILURE;
  }

  if (args.command == CMD_DEL_PASSWD && !args.identifier) {
    print_help();
    return EXIT_FAILURE;
  }

  // check if identifier is in correct format
  if (args.identifier && !check_password_identifier(args.identifier)) {
    print_error();
    return EXIT_FAILURE;
  }

  // prepare master password cache store
  master_pwd_cache *cache = get_shared_memory();
  if (!cache) {
    print_error();
    return EXIT_FAILURE;
  }

  ensure_initial_database(cache);
  ensure_master_password(cache);

  switch (args.command) {
  case CMD_ADD_PASSWD:
    add_new_password(cache, args.identifier);
    break;

  case CMD_DEL_PASSWD:
    delete_password(cache, args.identifier);
    break;

  case CMD_COPY_PASSWD:
    retrieve_password(cache, args.identifier);
    break;

  case CMD_LIST_PASSWD:
    list_passwords(cache);
    break;

  default: {}
  }

  // cache the master password for a while; run a daemon which will bust the
  // cache after some time
  if (!cache->password_available) {
    cache->password_available = true;
    run_master_password_daemon(cache);
  }

  detach_shared_memory(cache);
  return EXIT_SUCCESS;
}

void exit_with_cleanup(master_pwd_cache *cache) {
  print_error();
  detach_shared_memory(cache);
  exit(EXIT_FAILURE);
}

void ensure_initial_database(master_pwd_cache *cache) {
  if (database_exists()) {
    return;
  }

  char init_master_pwd[PASSWD_MAX_LENGTH];
  obtain_master_password(init_master_pwd, true);

  if (!create_database(init_master_pwd)) {
    exit_with_cleanup(cache);
  }
  printf("Database created\n");

  // copy initial password to cache
  memcpy(cache->master_password, init_master_pwd, PASSWD_MAX_LENGTH);
  memset(init_master_pwd, 0, PASSWD_MAX_LENGTH);
}

void ensure_master_password(master_pwd_cache *cache) {
  if (!cache->password_available) {
    obtain_master_password(cache->master_password, false);
  }
}

bool copy_password_to_clipboard(Line entry) {
  Line password;
  password_from_entry(password, entry);

  if (!clipboard_copy(password)) {
    return false;
  }

  printf("Password copied to clipboard.\n");
  return true;
}

void add_new_password(master_pwd_cache *cache, char *identifier) {
  Lines entries;
  int num_entries;
  if (!read_database(cache->master_password, entries, &num_entries)) {
    exit_with_cleanup(cache);
  }

  // check for existing entry and ask to override it
  int entry_idx = find_password_entry(entries, num_entries, identifier);
  bool entry_found = entry_idx >= 0;

  if (entry_found && !ask_override_entry()) {
    // can't do much if user does not confirm
    return;
  }

  char new_password[PASSWD_MAX_LENGTH]; // final password is > 15 chars
  if (!generate_random_password(new_password, 15)) {
    exit_with_cleanup(cache);
  }

  int new_entry_idx = entry_idx >= 0 ? entry_idx : num_entries++;
  create_entry(entries[new_entry_idx], identifier, new_password);

  // save updated database
  if (!save_database(cache->master_password, entries, num_entries)) {
    exit_with_cleanup(cache);
  }

  // copy new password to clipboard
  if (!copy_password_to_clipboard(entries[new_entry_idx])) {
    exit_with_cleanup(cache);
  };
}

void delete_password(master_pwd_cache *cache, char *identifier) {
  Lines entries;
  int num_entries;
  if (!read_database(cache->master_password, entries, &num_entries)) {
    exit_with_cleanup(cache);
  }

  // check for existing entry
  int entry_idx = find_password_entry(entries, num_entries, identifier);
  if (entry_idx < 0) {
    printf("No entry found for key \"%s\".\n", identifier);
    return;
  }

  // swap the deleted entry with the last one in the list and reduce number
  // of entries
  memcpy(entries[entry_idx], entries[num_entries - 1], sizeof(Line));
  num_entries--;

  // save updated database
  if (!save_database(cache->master_password, entries, num_entries)) {
    exit_with_cleanup(cache);
  }

  printf("Password removed from database.\n");
}

void retrieve_password(master_pwd_cache *cache, char *identifier) {
  Lines entries;
  int num_entries;
  if (!read_database(cache->master_password, entries, &num_entries)) {
    exit_with_cleanup(cache);
  }

  // find existing entry
  int entry_idx = find_password_entry(entries, num_entries, identifier);
  if (entry_idx < 0) {
    printf("No entry found for key \"%s\".\n", identifier);
    return;
  }

  if (!copy_password_to_clipboard(entries[entry_idx])) {
    exit_with_cleanup(cache);
  };
}

void list_passwords(master_pwd_cache *cache) {
  Lines entries;
  int num_entries;
  if (!read_database(cache->master_password, entries, &num_entries)) {
    exit_with_cleanup(cache);
  }

  Lines identifiers;
  entries_to_identifiers(identifiers, entries, num_entries);
  print_columns(identifiers, num_entries);
}
