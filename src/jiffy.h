#ifndef JIFFY_H
#define JIFFY_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stdint.h> // uint8_t
#include <stddef.h> // size_t

/**
 * Jiffy errors.
 */
#define JIFFY_ERROR_LIST \
  JIFFY_DEF_ERR(OK, "ok"), \
  JIFFY_DEF_ERR(BAD_BYTE, "bad byte"), \
  JIFFY_DEF_ERR(BAD_STATE, "bad state"), \
  JIFFY_DEF_ERR(BAD_ESCAPE, "bad escape"), \
  JIFFY_DEF_ERR(BAD_UTF16_BOM, "bad UTF-16 byte order mark"), \
  JIFFY_DEF_ERR(BAD_UTF8_BOM, "bad UTF-8 byte order mark"), \
  JIFFY_DEF_ERR(BAD_UNICODE_ESCAPE, "bad unicode escape"), \
  JIFFY_DEF_ERR(BAD_UNICODE_CODEPOINT, "bad unicode code point"), \
  JIFFY_DEF_ERR(STACK_UNDERFLOW, "stack underflow"), \
  JIFFY_DEF_ERR(STACK_OVERFLOW, "stack overflow"), \
  JIFFY_DEF_ERR(EXPECTED_ARRAY_ELEMENT, "expected array element"), \
  JIFFY_DEF_ERR(EXPECTED_COMMA_OR_ARRAY_END, "expected comma or array end"), \
  JIFFY_DEF_ERR(EXPECTED_STRING_OR_OBJECT_END, "expected string or object end"), \
  JIFFY_DEF_ERR(EXPECTED_COMMA_OR_OBJECT_END, "expected comma or object end"), \
  JIFFY_DEF_ERR(EXPECTED_OBJECT_KEY, "expected object key"), \
  JIFFY_DEF_ERR(EXPECTED_COLON, "expected colon"), \
  JIFFY_DEF_ERR(NOT_DONE, "not done"), \
  JIFFY_DEF_ERR(TREE_STACK_SCAN_FAILED, "tree stack scan failed"), \
  JIFFY_DEF_ERR(TREE_STACK_MALLOC_FAILED, "tree stack malloc() failed"), \
  JIFFY_DEF_ERR(TREE_OUTPUT_MALLOC_FAILED, "tree output malloc() failed"), \
  JIFFY_DEF_ERR(TREE_PARSE_MALLOC_FAILED, "tree parse malloc() failed"), \
  JIFFY_DEF_ERR(LAST, "unknown error"),

/**
 * Error codes.  These are passed to the on_error callback when the
 * jiffy_parser_*() functions encounter an error.
 *
 * You can use jiffy_err_to_s() to get a text description of the error
 * code.
 */
typedef enum {
#define JIFFY_DEF_ERR(a, b) JIFFY_ERR_##a
JIFFY_ERROR_LIST
#undef JIFFY_DEF_ERR
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

#define JIFFY_WARNING_LIST \
  JIFFY_DEF_WARNING(UTF16_BOM, "encountered UTF-16 byte order mark"), \
  JIFFY_DEF_WARNING(UTF8_BOM, "encountered UTF-8 byte order mark"), \
  JIFFY_DEF_WARNING(LAST, "unknown warning"),

/**
 * Warning codes.  A warning indicates a non-fatal problem with the
 * input that you may wish to be aware of, such as the presence of a
 * byte order mark.
 *
 * These are passed to the on_warning callback when the jiffy_parser_*()
 * functions encounter a warning.
 *
 * You can use jiffy_warning_to_s() to get a text description of the
 * warning code.
 */
typedef enum {
#define JIFFY_DEF_WARNING(a, b) JIFFY_WARNING_##a
JIFFY_WARNING_LIST
#undef JIFFY_DEF_WARNING
} jiffy_warning_t;

/**
 * Convert an warning code to human-readable text.
 *
 * Note: The string returned by this method is read-only.
 */
const char *jiffy_warning_to_s(
  // warning code
  const jiffy_warning_t
);

/**
 * Parser state.
 *
 * Note: In memory-constraint environments you can switch this from a
 * uint32_t to a uint8_t to save on space.  You may pay a speed penalty
 * on architectures which do not allow unaligned access (e.g., ARM).
 */
typedef uint32_t jiffy_parser_state_t;

typedef enum {
  // number contains a decimal component
  JIFFY_NUMBER_FLAG_FRAC = 0x01,

  // number contains an exponent
  JIFFY_NUMBER_FLAG_EXP  = 0x02,
} jiffy_number_flag_t;

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

  // fired just before the end of a number value; contains a
  // union of the jiffy_number_flag_t flags.
  void (*on_number_flags)(
    // pointer to parser context.
    const jiffy_parser_t *,

    // number flags
    const uint32_t
  );

  // fired when the parser encounters a non-fatal condition, such as the
  // presence of a byte order mark
  void (*on_warning)(
    // pointer to parser context.
    const jiffy_parser_t *,

    // warning code
    const jiffy_warning_t
  );

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

  union {
    struct {
      // parsed hex value.  used to decode unicode escape sequences.
      uint8_t hex;
    } v_str;

    struct {
      uint32_t flags;
    } v_num;
  };

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

#define JIFFY_TYPE_LIST \
  JIFFY_DEF_TYPE(NULL, "null"), \
  JIFFY_DEF_TYPE(TRUE, "true"), \
  JIFFY_DEF_TYPE(FALSE, "false"), \
  JIFFY_DEF_TYPE(NUMBER, "number"), \
  JIFFY_DEF_TYPE(STRING, "string"), \
  JIFFY_DEF_TYPE(ARRAY, "array"), \
  JIFFY_DEF_TYPE(OBJECT, "object"), \
  JIFFY_DEF_TYPE(LAST, "invalid"),

typedef enum {
#define JIFFY_DEF_TYPE(a, b) JIFFY_TYPE_##a
JIFFY_TYPE_LIST
#undef JIFFY_DEF_TYPE
} jiffy_type_t;

/**
 * Convert value type to human-readable text.
 *
 * Note: The string returned by this method is read-only.
 */
const char *jiffy_type_to_s(
  const jiffy_type_t
);

// forward declaration
typedef struct jiffy_value_t_ jiffy_value_t;

struct jiffy_value_t_ {
  jiffy_type_t type;

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
jiffy_type_t jiffy_value_get_type(
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
  const void * const src,
  const size_t len,
  void * const user_data
);

/**
 * TODO: document this
 */
_Bool jiffy_tree_new_ex(
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

/**
 * Writer state.
 *
 * Note: In memory-constraint environments you can switch this from a
 * uint32_t to a uint8_t to save on space.  You may pay a speed penalty
 * on architectures which do not allow unaligned access (e.g., ARM).
 */
typedef uint32_t jiffy_writer_state_t;

// forward declaration
typedef struct jiffy_writer_t_ jiffy_writer_t;

typedef struct {
  void (*on_write)(const jiffy_writer_t *, const void *, const size_t);
  void (*on_fini)(const jiffy_writer_t *);
  void (*on_error)(const jiffy_writer_t *, const jiffy_err_t);
} jiffy_writer_cbs_t;

struct jiffy_writer_t_ {
  const jiffy_writer_cbs_t * cbs;

  jiffy_writer_state_t *stack_ptr;
  size_t stack_len;
  size_t stack_pos;

  void * user_data;
};

_Bool jiffy_writer_init(
  jiffy_writer_t * const,
  const jiffy_writer_cbs_t *,
  jiffy_writer_state_t * const,
  const size_t,
  void *
);

_Bool jiffy_writer_write_null(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_true(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_false(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_object_start(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_object_end(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_array_start(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_array_end(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_number_start(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_number_data(
  jiffy_writer_t * const w,
  const void * const,
  const size_t
);

_Bool jiffy_writer_write_number_end(
  jiffy_writer_t * const w
);

_Bool jiffy_writer_write_number_from_string(
  jiffy_writer_t * const w,
  const void * const,
  const size_t
);

#ifdef __cplusplus
};
#endif // __cplusplus

#endif // JIFFY_H
