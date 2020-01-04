#ifndef JIFFY_H
#define JIFFY_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#define JIFFY_ERROR_LIST \
  E(OK, "ok"), \
  E(BAD_BYTE, "bad byte"), \
  E(BAD_STATE, "bad state"), \
  E(BAD_ESCAPE, "bad escape"), \
  E(BAD_UNICODE_ESCAPE, "bad unicode escape"), \
  E(STACK_UNDERFLOW, "stack underflow"), \
  E(STACK_OVERFLOW, "stack overflow"), \
  E(EXPECTED_ARRAY_ELEMENT, "expected array element"), \
  E(EXPECTED_COMMA_OR_ARRAY_END, "expected comma or array end"), \
  E(EXPECTED_STRING_OR_OBJECT_END, "expected string or object end"), \
  E(EXPECTED_COMMA_OR_OBJECT_END, "expected comma or object end"), \
  E(EXPECTED_OBJECT_KEY, "expected object key"), \
  E(EXPECTED_COLON, "expected colon"), \
  E(NOT_DONE, "not done"), \
  E(LAST, "unknown error"),

typedef enum {
#define E(a, b) JIFFY_ERR_##a
JIFFY_ERROR_LIST
#undef E
} jiffy_err_t;

const char *jiffy_err_to_s(const jiffy_err_t);

const char *jiffy_state_to_s(const uint32_t);

typedef struct jiffy_parser_t_ jiffy_parser_t;

typedef void (*jiffy_event_cb_t)(const jiffy_parser_t *);
typedef void (*jiffy_byte_cb_t)(const jiffy_parser_t *, const uint8_t);

typedef struct {
  const jiffy_event_cb_t on_null,
                         on_true,
                         on_false,
                         on_array_start,
                         on_array_end,
                         on_array_element_start,
                         on_array_element_end,
                         on_object_start,
                         on_object_end,
                         on_object_key_start,
                         on_object_key_end,
                         on_object_value_start,
                         on_object_value_end,
                         on_string_start,
                         on_string_end,
                         on_number_start,
                         on_number_end;

  const jiffy_byte_cb_t on_string_byte,
                        on_number_byte;

  void (*on_error)(
    const jiffy_parser_t *,
    const jiffy_err_t
  );
} jiffy_parser_cbs_t;

typedef struct {
  uint32_t *ptr;
  size_t len;
} jiffy_stack_t;

struct jiffy_parser_t_ {
  const jiffy_parser_cbs_t *cbs;
  jiffy_stack_t stack;
  size_t pos, num_bytes;
  uint8_t hex;
  
  void *user_data;
};

void
jiffy_parser_init(
  jiffy_parser_t * const,
  const jiffy_parser_cbs_t * const,
  jiffy_stack_t * const,
  void * const
);

void *
jiffy_parser_get_user_data(
  const jiffy_parser_t * const
);

_Bool
jiffy_parser_push(
  jiffy_parser_t * const,
  const void * const,
  const size_t
);

_Bool
jiffy_parser_fini(
  jiffy_parser_t * const
);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // JIFFY_H
