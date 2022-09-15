#include "cmdline.h"
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

  return EXIT_SUCCESS;
}
