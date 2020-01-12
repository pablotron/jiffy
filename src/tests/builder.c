#include <stdbool.h> // bool
#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include <err.h> // err(), errx(), warn()
#include "../jiffy.h"
#include "../test-set.h"

#define BUILDER_STACK_LEN 128
static jiffy_builder_state_t builder_stack[BUILDER_STACK_LEN];

#define PARSER_STACK_LEN 128
static jiffy_parser_state_t parser_stack[PARSER_STACK_LEN];

typedef struct {
  uint8_t *buf;
  size_t len;
} builder_data_t;

static void on_builder_write(
  const jiffy_builder_t * const builder,
  const void * const ptr,
  const size_t len
) {
  builder_data_t * const data = jiffy_builder_get_user_data(builder);
  memcpy(data->buf + data->len, ptr, len);
  data->len += len;
}

static void on_builder_error(
  const jiffy_builder_t * const builder,
  const jiffy_err_t err
) {
  (void) builder;
  warnx("builder error: %s", jiffy_err_to_s(err));
}

static const jiffy_builder_cbs_t BUILDER_CBS = {
  .on_write = on_builder_write,
  .on_error = on_builder_error,
};

static void
dump_builder(
  const jiffy_builder_t * const b
) {
  size_t len;
  const jiffy_builder_state_t * const stack = jiffy_builder_get_stack(b, &len);

  fprintf(stderr, "stack = ");
  for (size_t i = 0; i < len; i++) {
    const char * const delim = (i > 0) ? "," : "";
    const char * const state = jiffy_builder_state_to_s(stack[i]);
    fprintf(stderr, "%s%s", delim, state);

  }
  fprintf(stderr, "\n");
}

static void
on_parser_error(
  const jiffy_parser_t * const p,
  const jiffy_err_t err
) {
  (void) p;
  warnx("builder error: %s", jiffy_err_to_s(err));
}
static void on_parser_object_start(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: object start");
  if (!jiffy_builder_object_start(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_object_end(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: object end");
  dump_builder(builder);
  if (!jiffy_builder_object_end(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_array_start(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: array start");
  if (!jiffy_builder_array_start(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_array_end(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: array end");
  if (!jiffy_builder_array_end(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_string_start(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: string start");
  if (!jiffy_builder_string_start(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_string_end(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: string end");
  if (!jiffy_builder_string_end(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_string_byte(
  const jiffy_parser_t * const p,
  const uint8_t byte
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: string data: %02x", byte);
  if (!jiffy_builder_string_data(builder, &byte, 1)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_number_start(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: number start");
  if (!jiffy_builder_number_start(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_number_end(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: number end");
  if (!jiffy_builder_number_end(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_number_byte(
  const jiffy_parser_t * const p,
  const uint8_t byte
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: number data: %02x", byte);
  if (!jiffy_builder_number_data(builder, &byte, 1)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_true(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: true");
  if (!jiffy_builder_true(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_false(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: false");
  if (!jiffy_builder_false(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static void on_parser_null(
  const jiffy_parser_t * const p
) {
  jiffy_builder_t * const builder = jiffy_parser_get_user_data(p);

  warnx("build: null");
  if (!jiffy_builder_null(builder)) {
    dump_builder(builder);
    exit(EXIT_FAILURE);
  }
}

static const jiffy_parser_cbs_t PARSER_CBS = {
  .on_error         = on_parser_error,
  .on_object_start  = on_parser_object_start,
  .on_object_end    = on_parser_object_end,
  .on_array_start   = on_parser_array_start,
  .on_array_end     = on_parser_array_end,
  .on_string_start  = on_parser_string_start,
  .on_string_end    = on_parser_string_end,
  .on_string_byte   = on_parser_string_byte,
  .on_number_start  = on_parser_number_start,
  .on_number_end    = on_parser_number_end,
  .on_number_byte   = on_parser_number_byte,
  .on_true          = on_parser_true,
  .on_false         = on_parser_false,
  .on_null          = on_parser_null,
};

void test_builder(int argc, char *argv[]) {
  char src_buf[1024], dst_buf[1024];

  // populate builder data
  builder_data_t builder_data = {
    .buf = (uint8_t*) dst_buf,
    .len = 0,
  };

  // init test set
  test_set_t set;
  if (!test_set_init(&set, argc, argv)) {
    return;
  }

  size_t len;
  bool expect;
  while (test_set_next(&set, src_buf, sizeof(src_buf), &expect, &len)) {
    warnx("I: src = %s", src_buf);

    // clear builder data
    builder_data.len = 0;

    // init builder, check for error
    jiffy_builder_t builder;
    if (!jiffy_builder_init(&builder, &BUILDER_CBS, builder_stack, BUILDER_STACK_LEN, &builder_data)) {
      errx(EXIT_FAILURE, "jiffy_builder_init() failed");
    }

    // parse line, populate builder
    if (jiffy_parse(&PARSER_CBS, parser_stack, PARSER_STACK_LEN, src_buf, len, &builder) != expect) {
      errx(EXIT_FAILURE, "jiffy_parse() failed");
    }

    // finalize builder, check for error
    if (!jiffy_builder_fini(&builder)) {
      errx(EXIT_FAILURE, "jiffy_builder_fini() failed");
    }

    dst_buf[builder_data.len] = '\0';
    warnx("I: dst = %s", dst_buf);
  }
}
