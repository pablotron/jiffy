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

/**
 * Error codes.  These are passed to the on_error callback when the
 * jiffy_parser_*() functions encounter an error.
 *
 * You can use jiffy_err_to_s() to get a text description of the error
 * code.
 */
typedef enum {
#define E(a, b) JIFFY_ERR_##a
JIFFY_ERROR_LIST
#undef E
} jiffy_err_t;

/**
 * Convert an error code to human-readable text.
 *
 * Note: The string returned by this method is read-only.
 */
const char *jiffy_err_to_s(
  // error code
  const jiffy_err_t
);

/**
 * Convert a internal parser state to human-readable text.
 *
 * This method is mainly used for debugging.
 *
 * Note: The string returned by this method is read-only.
 */
const char *jiffy_state_to_s(
  // parser state
  const uint32_t
);

// forward declaration
typedef struct jiffy_parser_t_ jiffy_parser_t;

/**
 * Parser event callback.
 */
typedef void (*jiffy_parser_cb_t)(
  // pointer to parser callback structure
  const jiffy_parser_t *
);

/**
 * Parser byte callback.
 *
 * Used for parser callbacks which pass a byte of data (on_string_byte,
 * on_number_byte, etc).
 */
typedef void (*jiffy_parser_byte_cb_t)(
  // pointer to parser callback structure
  const jiffy_parser_t *,

  // byte that was parsed
  const uint8_t
);

/**
 * Parser callbacks.
 *
 * Used for parser callbacks which pass a byte of data (on_string_byte,
 * on_number_byte, etc).
 *
 * Note: Any or all of these callback pointers may be NULL.
 */
typedef struct {
  // literal null value.
  const jiffy_parser_cb_t on_null;

  // literal true value.
  const jiffy_parser_cb_t on_true;

  // literal false value.
  const jiffy_parser_cb_t on_false;

  // start of an array.
  const jiffy_parser_cb_t on_array_start;

  // end of an array.
  const jiffy_parser_cb_t on_array_end;

  // start of array element.
  const jiffy_parser_cb_t on_array_element_start;

  // end of array element.
  const jiffy_parser_cb_t on_array_element_end;

  // start of an object.
  const jiffy_parser_cb_t on_object_start;

  // end of an object.
  const jiffy_parser_cb_t on_object_end;

  // start of a key in an object.
  const jiffy_parser_cb_t on_object_key_start;

  // end of a key in an object.
  const jiffy_parser_cb_t on_object_key_end;

  // start of a value in an object.
  const jiffy_parser_cb_t on_object_value_start;

  // end of a value in an object.
  const jiffy_parser_cb_t on_object_value_end;

  // start of a string value.
  const jiffy_parser_cb_t on_string_start;

  // single byte of string value.
  const jiffy_parser_byte_cb_t on_string_byte;

  // end of a string value.
  const jiffy_parser_cb_t on_string_end;

  // start of a number value.
  const jiffy_parser_cb_t on_number_start;

  // single byte of number value.
  const jiffy_parser_byte_cb_t on_number_byte;

  // end of a number value.
  const jiffy_parser_cb_t on_number_end;

  void (*on_error)(
    // pointer to parser context.
    const jiffy_parser_t *,

    // error code
    const jiffy_err_t
  );
} jiffy_parser_cbs_t;

/**
 * Parser context.
 *
 * Note: You should not access the fields of this structure directly.
 * Use the jiffy_parser_get_*() functions instead.
 */
struct jiffy_parser_t_ {
  // pointer to parser callback structure. Provided by user via a
  // jiffy_parser_init() parameter.
  const jiffy_parser_cbs_t *cbs;

  // pointer to memory for state stack.  Provided by user via a
  // jiffy_parser_init() parameter.
  uint32_t *stack_ptr;

  // number of entries in stack memory.  Provided by user via a
  // jiffy_parser_init() parameter.
  size_t stack_len;

  // stack position
  size_t stack_pos;

  // number of bytes parsed so far.  Accessible via the
  // jiffy_parser_et_num_bytes() function.
  size_t num_bytes;

  // parsed hex value.  used to decode unicode escape sequences.
  uint8_t hex;

  // opaque pointer to user data.  Provided by user via a
  // jiffy_parser_init() parameter.  Accessible via the
  // jiffy_parser_get_user_data() function.
  void *user_data;
};

/**
 * Initialize a parser.
 *
 * Returns false if any of the following errors occur:
 *
 * - the parser context is NULL
 * - the stack memory pointer is NULL
 * - the number of stack memory elements is less than 2
 */
_Bool
jiffy_parser_init(
  // pointer to parser context (required)
  jiffy_parser_t * const,

  // pointer to parser callback structure (optional, may be NULL)
  const jiffy_parser_cbs_t * const,

  // memory for state stack (required)
  uint32_t * const,

  // number of entries in state stack (required, must be non-zero)
  const size_t,

  // opaque pointer to user data (optional)
  void * const
);

/**
 * Return user data associated with parser.
 */
void *
jiffy_parser_get_user_data(
  // pointer to parser context (required)
  const jiffy_parser_t * const
);

/**
 * Return number of bytes parsed by parser.
 */
size_t
jiffy_parser_get_num_bytes(
  // pointer to parser context (required)
  const jiffy_parser_t * const
);

/**
 * Parse buffer of data.
 *
 * Returns true on success.  If an error occurs, the on_error callback
 * is called with an error code, and this function return false.
 */
_Bool
jiffy_parser_push(
  // pointer to parser context (required)
  jiffy_parser_t * const,

  // pointer to input buffer (required)
  const void * const,

  // size of input buffer, in bytes
  const size_t
);

/**
 * Finalize a parser.  Call this function when you have reached the end
 * of the data to parse.
 *
 * This function flushes any outstanding parsed data and finalizes the
 * parser.
 *
 * Returns true on success.  If an error occurs, the on_error callback
 * is called with an error code, and this function return false.
 */
_Bool
jiffy_parser_fini(
  // pointer to parser context (required)
  jiffy_parser_t * const
);

/**
 * Convenience function to parse an input buffer in a single call.
 *
 * Returns true on success.  If an error occurs, then the on_error
 * callback is called with an error code, and this function return
 * false.
 */
_Bool
jiffy_parse(
  // pointer to parser callback structure (optional, may be NULL)
  const jiffy_parser_cbs_t * const,

  // memory for state stack (required)
  uint32_t * const,

  // number of entries in state stack (required, must be non-zero)
  const size_t,

  // pointer to input buffer (required)
  const void * const,

  // size of input buffer, in bytes
  const size_t,

  // opaque pointer to user data (optional)
  void * const
);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // JIFFY_H
