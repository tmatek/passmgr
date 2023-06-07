#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "database.h"
#include "inout.h"
#include "ipc.h"
#include "password.h"

void copy_password_to_clipboard(Line entry);

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
  if (args.identifier) {
    PwdResult check_res = check_password_identifier(args.identifier);
    handle_password_result(check_res);
  }

  bool first_time = !database_exists();

  // prepare master password cache store
  master_pwd_cache *cache = get_shared_memory(argv[0]);
  if (!cache) {
    fprintf(stderr, "Error in process communication.\n");
    return EXIT_FAILURE;
  }

  // always ask for master password, if not cached
  if (!cache->password_available) {
    obtain_master_password(cache->master_password, first_time);
  }

  // create a new database
  if (first_time) {
    DBResult res = create_database(cache->master_password);
    handle_database_result(res);
    printf("Database created\n");
  }

  // read the current database
  Lines entries;
  int num_entries;
  DBResult read_res =
      read_database(cache->master_password, entries, &num_entries);
  handle_database_result(read_res);

  switch (args.command) {

  case CMD_ADD_PASSWD: {
    // check for existing entry
    int entry_idx = find_password_entry(entries, num_entries, args.identifier);
    if (entry_idx >= 0 && !ask_override_entry()) {
      return EXIT_SUCCESS;
    }

    char new_password[PASSWD_MAX_LENGTH]; // final password is > 15 chars
    PwdResult gen_res = generate_random_password(new_password, 15);
    handle_password_result(gen_res);

    int new_entry_idx = entry_idx >= 0 ? entry_idx : num_entries++;
    create_entry(entries[new_entry_idx], args.identifier, new_password);
    copy_password_to_clipboard(entries[new_entry_idx]);
    break;

  case CMD_DEL_PASSWD: {
    // check for existing entry
    int entry_idx = find_password_entry(entries, num_entries, args.identifier);
    if (entry_idx < 0) {
      fprintf(stderr, "No entry found for key \"%s\".\n", args.identifier);
      return EXIT_SUCCESS;
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
      fprintf(stderr, "No entry found for key \"%s\".\n", args.identifier);
      break;
    }

    copy_password_to_clipboard(entries[entry_idx]);
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
    DBResult save_res =
        save_database(cache->master_password, entries, num_entries);
    handle_database_result(save_res);
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

void copy_password_to_clipboard(Line entry) {
  Line password;
  password_from_entry(password, entry);

  if (!clipboard_copy(password)) {
    fprintf(stderr, "Unable to copy password to your clipboard.\n");
    exit(EXIT_FAILURE);
  }

  printf("Password copied to clipboard.\n");
}
