#include <stdbool.h>

typedef enum Command {
  CMD_ADD_PASSWD,
  CMD_COPY_PASSWD,
  CMD_DEL_PASSWD,
  CMD_NONE,
  CMD_LIST_PASSWD,
  CMD_PUT_PASSWD,
} Command;

typedef struct InputArgs {
  Command command;
  char *identifier;
} InputArgs;

InputArgs parse_command_line(int argc, char **argv);
void print_help();

bool clipboard_copy(char *string);
