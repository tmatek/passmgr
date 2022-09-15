#include "cmdline.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  InputArgs args = parse_command_line(argc, argv);

  // handle invalid command-line inputs
  if (args.command == CMD_NONE) {
    print_help();
    return EXIT_FAILURE;
  }

  if (!args.identifier && args.command != CMD_COPY_PASSWD &&
      args.command != CMD_LIST_PASSWD) {
    print_help();
    return EXIT_FAILURE;
  }

  if (!database_exists()) {
    // prompt for master password
    // create_database()
  }

  // create_database("test1234");

  // Lines lines;
  //
  // int lines_read;
  // read_database("test1234", lines, &lines_read);
  //
  // for (int i = 0; i < lines_read; i++) {
  //   printf("%s\n", lines[i]);
  // }

  return EXIT_SUCCESS;
}
