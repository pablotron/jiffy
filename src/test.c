#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include "jiffy.h"

static void
dump_parser(
  const jiffy_parser_t * const p
) {
  fprintf(stderr, "D: state (%zu):", p->pos);
  for (size_t i = 0; i <= p->pos; i++) {
    fprintf(stderr, " %s", jiffy_state_to_s(p->stack.ptr[i]));
  }
  fputs("\n", stderr);
}

static void on_error(
  const jiffy_parser_t * const p,
  const jiffy_err_t err
) {
  (void) p;
  fprintf(stderr, "E: error: %s\n", jiffy_err_to_s(err));
}

static void on_object_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: object start\n");
}

static void on_object_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: object end\n");
}

static const jiffy_parser_cbs_t CBS = {
  .on_error         = on_error,
  .on_object_start  = on_object_start,
  .on_object_end    = on_object_end,
};

static uint32_t stack_mem[128];

int main(int argc, char *argv[]) {
  jiffy_stack_t stack = {
    .ptr = stack_mem,
    .len = sizeof(stack_mem) / sizeof(uint32_t),
  };

  for (int i = 1; i < argc; i++) {
    // init parser
    jiffy_parser_t parser;
    jiffy_parser_init(&parser, &CBS, &stack, NULL);

    fprintf(stderr, "D: parsing \"%s\"\n", argv[i]);

    // parse argument
    if (!jiffy_parser_push(&parser, argv[i], strlen(argv[i]))) {
      dump_parser(&parser);
      exit(EXIT_FAILURE);
    }

    // finalize parser
    if (!jiffy_parser_fini(&parser)) {
      dump_parser(&parser);
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "D: parsing done\n");
  }

  return EXIT_SUCCESS;
}
