#include "inout.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

InputArgs parse_command_line(int argc, char **argv) {
  InputArgs args = {CMD_NONE, NULL};

  if (argc < 2) {
    return args;
  }

  if (strcmp(argv[1], "add") == 0) {
    args.command = CMD_ADD_PASSWD;
  } else if (strcmp(argv[1], "put") == 0) {
    args.command = CMD_PUT_PASSWD;
  } else if (strcmp(argv[1], "del") == 0) {
    args.command = CMD_DEL_PASSWD;
  } else if (strcmp(argv[1], "list") == 0) {
    args.command = CMD_LIST_PASSWD;
  } else {
    args.command = CMD_COPY_PASSWD;
    args.identifier = argv[1];
  }

  if (argc > 2) {
    args.identifier = argv[2];
  }

  return args;
}

bool ask_override_entry() {
  char ok;
  printf("Overwrite existing password in the database? (Y/N): ");
  scanf("%c", &ok);

  return ok == 'Y' || ok == 'y';
}

void print_help() {
  printf("usage: pass <command> [identifier]\n\n");
  printf("where command is one of the following:\n");
  printf("%8s\t%s\n", "add",
         "Creates a new password entry in the database with identifier");
  printf("%8s\t%s\n", "del", "Remove an existing entry from the database");
  printf("%8s\t%s\n", "list", "List all entries in the database");
  printf("%8s\t%s\n", "put",
         "Stores your own password entry under the identifier");
  printf("\n");
  printf("If no command is given, the password associated with identifier will "
         "be copied to your clipboard.\n");
  printf("\n");
}
