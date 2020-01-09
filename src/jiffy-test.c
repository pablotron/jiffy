#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include <err.h> // errx()
#include "jiffy.h"

extern void test_parser(int, char **);
extern void test_tree(int, char **);
static void help(int, char **);

static const struct {
  const char * const name;
  const char * const text;
  void (* const fn)(int, char **);
} CMDS[] = {{
  .name = "help",
  .text = "print help",
  .fn   = help,
}, {
  .name = "parser",
  .text = "test jiffy_parser_init()",
  .fn   = test_parser,
}, {
  .name = "tree",
  .text = "test jiffy_tree_new()",
  .fn   = test_tree,
}, {
//   .name = "builder",
//   .text = "test jiffy_builder_*()",
//   .fn   = test_builder,
// }, {
  .name = NULL,
}};

static int find_command(const char * const name) {
  for (size_t i = 0; CMDS[i].name; i++) {
    if (!strncmp(name, CMDS[i].name, strlen(CMDS[i].name) + 1)) {
      return i;
    }
  }

  errx(EXIT_FAILURE, "Unknown command: %s", name);
}

static void help(int argc, char *argv[]) {
  if (argc > 0) {
    const int ofs = find_command(argv[0]);
    printf("%s: %s\n", CMDS[ofs].name, CMDS[ofs].text);
  } else {
    for (size_t i = 0; CMDS[i].name; i++) {
      printf("%s: %s\n", CMDS[i].name, CMDS[i].text);
    }
  }
}

int main(int argc, char *argv[]) {
  // get command
  const char * cmd = (argc > 1) ? argv[1] : "help";

  // find command, invoke it
  CMDS[find_command(cmd)].fn(argc - 2, argv + 2);

  // return success
  return EXIT_SUCCESS;
}
