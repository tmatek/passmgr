#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "database.h"
#include "error.h"
#include "inout.h"
#include "ipc.h"
#include "password.h"

void exit_with_cleanup(master_pwd_cache *cache);
bool copy_password_to_clipboard(Line entry);

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

  bool first_time = !database_exists();

  // prepare master password cache store
  master_pwd_cache *cache = get_shared_memory(argv[0]);
  if (!cache) {
    print_error();
    return EXIT_FAILURE;
  }

  // always ask for master password, if not cached
  if (!cache->password_available) {
    obtain_master_password(cache->master_password, first_time);
  }

  // create a new database
  if (first_time) {
    if (!create_database(cache->master_password)) {
      exit_with_cleanup(cache);
    }
    printf("Database created\n");
  }

  // read the current database
  Lines entries;
  int num_entries;
  if (!read_database(cache->master_password, entries, &num_entries)) {
    exit_with_cleanup(cache);
  }

  switch (args.command) {

  case CMD_ADD_PASSWD: {
    // check for existing entry
    int entry_idx = find_password_entry(entries, num_entries, args.identifier);
    if (entry_idx >= 0 && !ask_override_entry()) {
      break;
    }

    char new_password[PASSWD_MAX_LENGTH]; // final password is > 15 chars
    if (!generate_random_password(new_password, 15)) {
      exit_with_cleanup(cache);
    }

    int new_entry_idx = entry_idx >= 0 ? entry_idx : num_entries++;
    create_entry(entries[new_entry_idx], args.identifier, new_password);

    if (!copy_password_to_clipboard(entries[new_entry_idx])) {
      exit_with_cleanup(cache);
    };
    break;

  case CMD_DEL_PASSWD: {
    // check for existing entry
    int entry_idx = find_password_entry(entries, num_entries, args.identifier);
    if (entry_idx < 0) {
      printf("No entry found for key \"%s\".\n", args.identifier);
      break;
    }

    // swap the deleted entry with the last one in the list and reduce number
    // of entries
    memcpy(entries[entry_idx], entries[num_entries - 1], sizeof(Line));
    num_entries--;

    printf("Password removed from database.\n");
    break;
  }

  case CMD_COPY_PASSWD: {
    // find existing entry
    int entry_idx = find_password_entry(entries, num_entries, args.identifier);
    if (entry_idx < 0) {
      printf("No entry found for key \"%s\".\n", args.identifier);
      break;
    }

    if (!copy_password_to_clipboard(entries[entry_idx])) {
      exit_with_cleanup(cache);
    };
    break;
  }

  case CMD_LIST_PASSWD: {
    Lines identifiers;
    entries_to_identifiers(identifiers, entries, num_entries);
    print_columns(identifiers, num_entries);
    break;
  }
  }

  default:;
  }

  // finally, save updated database
  if (args.command == CMD_ADD_PASSWD || args.command == CMD_DEL_PASSWD) {
    if (!save_database(cache->master_password, entries, num_entries)) {
      exit_with_cleanup(cache);
    }
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

bool copy_password_to_clipboard(Line entry) {
  Line password;
  password_from_entry(password, entry);

  if (!clipboard_copy(password)) {
    return false;
  }

  printf("Password copied to clipboard.\n");
  return true;
}
