#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "inout.h"

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

bool clipboard_copy(char *string) {
#ifdef _WIN32
  FILE *cpy = popen("clip", "w");
#else
  FILE *cpy = popen("pbcopy", "w");
#endif

  if (!cpy) {
    last_error = ERR_CLIPBOARD_COPY;
    return false;
  }

  fprintf(cpy, "%s", string);
  pclose(cpy);
  return true;
}

void print_columns(Lines strings, int num_strings) {
  int col_size = num_strings < 10 ? 1 : num_strings < 20 ? 2 : 3;
  for (int i = 0; i < num_strings; i++) {
    if (col_size > 1) {
      printf("%20s ", strings[i]);
    } else {
      printf("%s", strings[i]);
    }

    if ((i + 1) % col_size == 0) {
      printf("\n");
    }
  }

  if (num_strings % col_size != 0) {
    printf("\n");
  }
}

void print_help() {
  printf("usage: pass <command> [identifier]\n\n");
  printf("where command is one of the following:\n");
  printf("%8s\t%s\n", "add",
         "Create a new, random password entry in the database with identifier");
  printf("%8s\t%s\n", "del", "Remove an existing entry from the database");
  printf("%8s\t%s\n", "put",
         "Store your own password entry under the identifier");
  printf("%8s\t%s\n", "list", "List all entries in the database");
  printf("\n");
  printf("If no command is given, the password associated with identifier will "
         "be copied to your clipboard.\n");
  printf("\n");
}
