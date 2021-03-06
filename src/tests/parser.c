#include <stdbool.h> // bool
#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include <err.h> // err(), warn()
#include "../jiffy.h"
#include "test-set.h"

static void on_error(
  const jiffy_parser_t * const p,
  const jiffy_err_t err
) {
  (void) p;
  warnx("error: %s", jiffy_err_to_s(err));
}

static void on_object_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: object start");
}

static void on_object_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: object end");
}

static void on_object_key_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: object_key start");
}

static void on_object_key_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: object_key end");
}

static void on_object_value_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: object_value start");
}

static void on_object_value_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: object_value end");
}

static void on_array_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: array start");
}

static void on_array_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: array end");
}

static void on_array_element_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: array_element start");
}

static void on_array_element_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: array_element end");
}

static void on_string_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: string start");
}

static void on_string_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: string end");
}

static void on_string_byte(
  const jiffy_parser_t * const p,
  const uint8_t byte
) {
  (void) p;
  warnx("D: string byte = %02x", byte);
}

static void on_number_start(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: number start");
}

static void on_number_end(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: number end");
}

static void on_number_byte(
  const jiffy_parser_t * const p,
  const uint8_t byte
) {
  (void) p;
  warnx("D: number byte = %02x", byte);
}

static void on_true(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: true");
}

static void on_false(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: false");
}

static void on_null(
  const jiffy_parser_t * const p
) {
  (void) p;
  warnx("D: null");
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

void test_parser(int argc, char *argv[]) {
  char buf[1024];

  test_set_t set;
  if (!test_set_init(&set, argc, argv)) {
    return;
  }

  bool expect;
  size_t len;
  while (test_set_next(&set, buf, sizeof(buf), &expect, &len)) {
    warnx("D: parsing: \"%s\"", buf);

    // parse line
    if (jiffy_parse(&CBS, stack_mem, STACK_LEN, buf, len, NULL) != expect) {
      errx(EXIT_FAILURE, "jiffy_parse() test failed.");
    }

    warnx("D: parsing done");
  }
}
