#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include "jiffy.h"

/* 
 * static void
 * dump_parser(
 *   const jiffy_parser_t * const p
 * ) {
 *   fprintf(stderr, "D: state (%zu):", p->pos);
 *   for (size_t i = 0; i <= p->pos; i++) {
 *     fprintf(stderr, " %s", jiffy_state_to_s(p->stack.ptr[i]));
 *   }
 *   fputs("\n", stderr);
 * }
 */ 

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

static void on_object_key_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: object_key start\n");
}

static void on_object_key_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: object_key end\n");
}

static void on_object_value_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: object_value start\n");
}

static void on_object_value_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: object_value end\n");
}

static void on_array_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: array start\n");
}

static void on_array_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: array end\n");
}

static void on_array_element_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: array_element start\n");
}

static void on_array_element_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: array_element end\n");
}

static void on_string_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: string start\n");
}

static void on_string_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: string end\n");
}

static void on_string_byte(
  const jiffy_parser_t * const p,
  const uint8_t byte
) {
  (void) p;
  fprintf(stderr, "D: string byte = %02x\n", byte);
}

static void on_number_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: number start\n");
}

static void on_number_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: number end\n");
}

static void on_number_byte(
  const jiffy_parser_t * const p,
  const uint8_t byte
) {
  (void) p;
  fprintf(stderr, "D: number byte = %02x\n", byte);
}

static void on_true(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: true\n");
}

static void on_false(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: false\n");
}

static void on_null(
  const jiffy_parser_t * const p
) {
  (void) p;
  fprintf(stderr, "D: null\n");
}

static const jiffy_parser_cbs_t CBS = {
  .on_error               = on_error,
  .on_object_start        = on_object_start,
  .on_object_end          = on_object_end,
  .on_object_key_start    = on_object_key_start,
  .on_object_key_end      = on_object_key_end,
  .on_object_value_start  = on_object_value_start,
  .on_object_value_end    = on_object_value_end,
  .on_array_start         = on_array_start,
  .on_array_end           = on_array_end,
  .on_array_element_start = on_array_element_start,
  .on_array_element_end   = on_array_element_end,
  .on_string_start        = on_string_start,
  .on_string_end          = on_string_end,
  .on_string_byte         = on_string_byte,
  .on_number_start        = on_number_start,
  .on_number_end          = on_number_end,
  .on_number_byte         = on_number_byte,
  .on_true                = on_true,
  .on_false               = on_false,
  .on_null                = on_null,
};

#define STACK_LEN 128
static uint32_t stack_mem[STACK_LEN];

int main(int argc, char *argv[]) {
  (void) argc;
  (void) argv;

  char buf[1024];
  while (fgets(buf, sizeof(buf), stdin)) {
    const size_t len = strlen(buf);
    if (len < 1) {
      continue;
    }

    // strip newline
    buf[len - 1] = '\0';

    fprintf(stderr, "D: parsing: %s\n", buf);

    // parse line
    if (!jiffy_parse(&CBS, stack_mem, STACK_LEN, buf, len - 1, NULL)) {
      exit(EXIT_FAILURE);
    }

    fprintf(stderr, "D: parsing done\n");
  }

  return EXIT_SUCCESS;
}
