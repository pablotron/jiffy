#ifndef JIFFY_H
#define JIFFY_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

#define JIFFY_ERROR_LIST \
  JIFFY_ERR(OK, "ok"), \
  JIFFY_ERR(BAD_BYTE, "bad byte"), \
  JIFFY_ERR(BAD_STATE, "bad state"), \
  JIFFY_ERR(BAD_ESCAPE, "bad escape"), \
  JIFFY_ERR(BAD_UNICODE_ESCAPE, "bad unicode escape"), \
  JIFFY_ERR(BAD_UNICODE_CODEPOINT, "bad unicode code point"), \
  JIFFY_ERR(STACK_UNDERFLOW, "stack underflow"), \
  JIFFY_ERR(STACK_OVERFLOW, "stack overflow"), \
  JIFFY_ERR(EXPECTED_ARRAY_ELEMENT, "expected array element"), \
  JIFFY_ERR(EXPECTED_COMMA_OR_ARRAY_END, "expected comma or array end"), \
  JIFFY_ERR(EXPECTED_STRING_OR_OBJECT_END, "expected string or object end"), \
  JIFFY_ERR(EXPECTED_COMMA_OR_OBJECT_END, "expected comma or object end"), \
  JIFFY_ERR(EXPECTED_OBJECT_KEY, "expected object key"), \
  JIFFY_ERR(EXPECTED_COLON, "expected colon"), \
  JIFFY_ERR(NOT_DONE, "not done"), \
  JIFFY_ERR(TREE_OUTPUT_MALLOC_FAILED, "tree output malloc() failed"), \
  JIFFY_ERR(TREE_PARSE_MALLOC_FAILED, "tree parse malloc() failed"), \
  JIFFY_ERR(LAST, "unknown error"),

/**
 * Error codes.  These are passed to the on_error callback when the
 * jiffy_parser_*() functions encounter an error.
 *
 * You can use jiffy_err_to_s() to get a text description of the error
 * code.
 */
typedef enum {
#define JIFFY_ERR(a, b) JIFFY_ERR_##a
JIFFY_ERROR_LIST
#undef JIFFY_ERR
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
 * Parser state.
 *
 * Note: In memory-constraint environments you can switch this from a
 * uint32_t to a uint8_t to save on space.  You may pay a speed penalty
 * on architectures which do not allow unaligned access (e.g., ARM).
 */
typedef uint32_t jiffy_parser_state_t;

/**
 * Convert a internal parser state to human-readable text.
 *
 * This method is mainly used for debugging.
 *
 * Note: The string returned by this method is read-only.
 */
const char *jiffy_parser_state_to_s(
  // parser state
  const jiffy_parser_state_t
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
  jiffy_parser_state_t *stack_ptr;

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
  jiffy_parser_state_t * const,

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
  jiffy_parser_state_t * const,

  // number of entries in state stack (required, must be non-zero)
  const size_t,

  // pointer to input buffer (required)
  const void * const,

  // size of input buffer, in bytes
  const size_t,

  // opaque pointer to user data (optional)
  void * const
);

#define JIFFY_VALUE_TYPE_LIST \
  JIFFY_VALUE_TYPE(NULL, "null"), \
  JIFFY_VALUE_TYPE(TRUE, "true"), \
  JIFFY_VALUE_TYPE(FALSE, "false"), \
  JIFFY_VALUE_TYPE(NUMBER, "number"), \
  JIFFY_VALUE_TYPE(STRING, "string"), \
  JIFFY_VALUE_TYPE(ARRAY, "array"), \
  JIFFY_VALUE_TYPE(OBJECT, "object"), \
  JIFFY_VALUE_TYPE(LAST, "invalid"),

typedef enum {
#define JIFFY_VALUE_TYPE(a, b) JIFFY_VALUE_TYPE_##a
JIFFY_VALUE_TYPE_LIST
#undef JIFFY_VALUE_TYPE
} jiffy_value_type_t;

/**
 * Convert value type to human-readable text.
 *
 * Note: The string returned by this method is read-only.
 */
const char *jiffy_value_type_to_s(
  const jiffy_value_type_t
);

// forward declaration
typedef struct jiffy_value_t_ jiffy_value_t;

struct jiffy_value_t_ {
  jiffy_value_type_t type;

  union {
    struct {
      uint8_t *ptr;
      size_t len;
    } v_num;

    struct {
      uint8_t *ptr;
      size_t len;
    } v_str;

    struct {
      jiffy_value_t **vals;
      size_t len;
    } v_obj;

    struct {
      jiffy_value_t **vals;
      size_t len;
    } v_ary;
  };
};

/**
 * Get the type of the given value.
 */
jiffy_value_type_t jiffy_value_get_type(
  const jiffy_value_t * const
);

/**
 * Get a pointer to bytes and the number of bytes of the given number
 * value.
 *
 * Returns NULL if the given value is not a number.
 */
const uint8_t *jiffy_number_get_bytes(
  // value
  const jiffy_value_t * const,

  // pointer to store byte length
  size_t * const
);

/**
 * Get a pointer to bytes and the number of bytes of the given string
 * value.
 *
 * Returns NULL if the given value is not a string.
 */
const uint8_t *jiffy_string_get_bytes(
  // string value
  const jiffy_value_t * const,

  // pointer to returned byte count
  size_t * const
);

/**
 * Get the number of elements in the given array value.
 *
 * Note: Results are undefined if the given value is not an array.
 */
size_t jiffy_array_get_size(
  // array value
  const jiffy_value_t * const
);

/**
 * Get the Nth value of the given array.
 *
 * Returns NULL if the given value is not an array or the given offset
 * is out of bounds.
 */
const jiffy_value_t *jiffy_array_get_nth(
  // array value
  const jiffy_value_t * const,

  // offset
  const size_t
);

/**
 * Get the number of elements in the given object value.
 *
 * Note: Results are undefined if the given value is not an object.
 */
size_t jiffy_object_get_size(
  // object value
  const jiffy_value_t * const
);

/**
 * Get the Nth key of the given object.
 *
 * Returns NULL if the given value is not an object or the given offset
 * is out of bounds.
 */
const jiffy_value_t *jiffy_object_get_nth_key(
  // object value
  const jiffy_value_t * const,

  // offset
  const size_t
);

/**
 * Get the Nth value of the given object.
 *
 * Returns NULL if the given value is not an object or the given offset
 * is out of bounds.
 */
const jiffy_value_t *jiffy_object_get_nth_value(
  // object value
  const jiffy_value_t * const,

  // offset
  const size_t ofs
);

typedef struct jiffy_tree_t_ jiffy_tree_t;

typedef struct {
  void *(*malloc)(const size_t, void * const);
  void (*free)(void * const, void * const);

  void (*on_error)(const jiffy_tree_t *, const jiffy_err_t);
} jiffy_tree_cbs_t;

struct jiffy_tree_t_ {
  const jiffy_tree_cbs_t *cbs;
  void *user_data;

  // all allocated data, ordered like so:
  // * vals
  // * array rows
  // * object rows
  // * bytes (byte data for numbers and strings)
  uint8_t *data;

  // list of values (pointer into data)
  jiffy_value_t *vals;
  size_t num_vals;
};

/**
 * TODO: document this
 */
_Bool jiffy_tree_new(
  jiffy_tree_t * const tree,
  const jiffy_tree_cbs_t * const cbs,
  jiffy_parser_state_t * const stack,
  const size_t stack_len,
  const void * const src,
  const size_t len,
  void * const user_data
);

/**
 * Get user data associated with given tree.
 */
void *jiffy_tree_get_user_data(
  const jiffy_tree_t * const tree
);

/**
 * Get the root value for the given tree.
 *
 * Returns NULL if the given tree is empty.
 */
const jiffy_value_t *jiffy_tree_get_root_value(
  const jiffy_tree_t * const
);

/**
 * Free memory associated with tree.
 */
void jiffy_tree_free(
  jiffy_tree_t * const
);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // JIFFY_H
