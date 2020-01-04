#include <stdbool.h> // bool
#include "jiffy.h"

// whitespace characters (/[ \t\v\r\n]/)
#define CASE_WHITESPACE \
  case ' ': \
  case '\t': \
  case '\v': \
  case '\n': \
  case '\r':

// non-zero number characters (/[1-9]/)
#define CASE_NONZERO_NUMBER \
  case '1': \
  case '2': \
  case '3': \
  case '4': \
  case '5': \
  case '6': \
  case '7': \
  case '8': \
  case '9':

// number characters (/[0-9]/)
#define CASE_NUMBER \
  case '0': \
  case '1': \
  case '2': \
  case '3': \
  case '4': \
  case '5': \
  case '6': \
  case '7': \
  case '8': \
  case '9':

// lowercase alpha hex digits (/[a-f]/)
#define CASE_HEX_AF_LO \
  case 'a': \
  case 'b': \
  case 'c': \
  case 'd': \
  case 'e': \
  case 'f':

// uppercase alpha hex digits (/[A-F]/)
#define CASE_HEX_AF_HI \
  case 'A': \
  case 'B': \
  case 'C': \
  case 'D': \
  case 'E': \
  case 'F':

// hex digits (/[0-9a-fA-F]/)
#define CASE_HEX \
  CASE_NUMBER \
  CASE_HEX_AF_LO \
  CASE_HEX_AF_HI

/**
 * Decode a hex nibble and return it's value.
 */
static inline uint8_t
nibble(
  const uint8_t byte
) {
  switch (byte) {
  CASE_NUMBER
    return byte - '0';
  CASE_HEX_AF_LO
    return byte - 'a' + 10;
  CASE_HEX_AF_HI
    return byte - 'A' + 10;
  default:
    // FIXME
    return 0;
  }
}

/**
 * Error strings.  Used by jiffy_err_to_s().
 */
static const char *
JIFFY_ERRORS[] = {
#define JIFFY_ERR(a, b) b
JIFFY_ERROR_LIST
#undef JIFFY_ERR
};

const char *
jiffy_err_to_s(
  const jiffy_err_t err
) {
  const size_t ofs = err < JIFFY_ERR_LAST ? err : JIFFY_ERR_LAST;
  return JIFFY_ERRORS[ofs];
}

/**
 * Parser states.
 */
#define JIFFY_STATE_LIST \
  JIFFY_STATE(INIT), \
  JIFFY_STATE(DONE), \
  JIFFY_STATE(FAIL), \
  JIFFY_STATE(VALUE), \
  JIFFY_STATE(LIT_N), \
  JIFFY_STATE(LIT_NU), \
  JIFFY_STATE(LIT_NUL), \
  JIFFY_STATE(LIT_T), \
  JIFFY_STATE(LIT_TR), \
  JIFFY_STATE(LIT_TRU), \
  JIFFY_STATE(LIT_F), \
  JIFFY_STATE(LIT_FA), \
  JIFFY_STATE(LIT_FAL), \
  JIFFY_STATE(LIT_FALS), \
  JIFFY_STATE(NUMBER_AFTER_SIGN), \
  JIFFY_STATE(NUMBER_AFTER_LEADING_ZERO), \
  JIFFY_STATE(NUMBER_INT), \
  JIFFY_STATE(NUMBER_AFTER_DOT), \
  JIFFY_STATE(NUMBER_FRAC), \
  JIFFY_STATE(NUMBER_AFTER_EXP), \
  JIFFY_STATE(NUMBER_AFTER_EXP_SIGN), \
  JIFFY_STATE(NUMBER_EXP_NUM), \
  JIFFY_STATE(STRING), \
  JIFFY_STATE(STRING_ESC), \
  JIFFY_STATE(STRING_UNICODE), \
  JIFFY_STATE(STRING_UNICODE_X), \
  JIFFY_STATE(STRING_UNICODE_XX), \
  JIFFY_STATE(STRING_UNICODE_XXX), \
  JIFFY_STATE(OBJECT_START), \
  JIFFY_STATE(ARRAY_START), \
  JIFFY_STATE(ARRAY_ELEMENT), \
  JIFFY_STATE(OBJECT_KEY), \
  JIFFY_STATE(AFTER_OBJECT_KEY), \
  JIFFY_STATE(BEFORE_OBJECT_KEY), \
  JIFFY_STATE(AFTER_OBJECT_VALUE), \
  JIFFY_STATE(LAST),

/**
 * Parser states.
 */
enum jiffy_parser_states {
#define JIFFY_STATE(a) STATE_##a
JIFFY_STATE_LIST
#undef JIFFY_STATE
};

/**
 * Parser state strings.  Used by jiffy_parser_state_to_s().
 */
static const char *
JIFFY_STATES[] = {
#define JIFFY_STATE(a) "STATE_" # a
JIFFY_STATE_LIST
#undef JIFFY_STATE
};

const char *
jiffy_parser_state_to_s(
  const jiffy_parser_state_t state
) {
  const size_t ofs = (state < STATE_LAST) ? state : STATE_LAST;
  return JIFFY_STATES[ofs];
}

// get the current state
#define GET_STATE(p) ((p)->stack_ptr[(p)->stack_pos])

// set the current state
#define SWAP(p, state) (p)->stack_ptr[(p)->stack_pos] = (state)

// invoke on_error callback with error code, set the state to
// STATE_FAIL, and then return false.
#define FAIL(p, err) do { \
  if ((p)->cbs && (p)->cbs->on_error) { \
    (p)->cbs->on_error((p), err); \
  } \
  SWAP((p), STATE_FAIL); \
  return false; \
} while (0)

// call given callback, if it is non-NULL
#define FIRE(p, cb_name) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p); \
  } \
} while (0)

// call given callback with byte value, if it is non-NULL
#define EMIT(p, cb_name, byte) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p, (byte)); \
  } \
} while (0)

bool
jiffy_parser_init(
  jiffy_parser_t * const p,
  const jiffy_parser_cbs_t * const cbs,
  jiffy_parser_state_t * const stack_ptr,
  const size_t stack_len,
  void * const user_data
) {
  // verify that the following conditions are true:
  // * the parser context is non-null
  // * the stack memory pointer is non-null
  // * the number of stack memory elements is greater than 1
  if (!p || !stack_ptr || stack_len < 2) {
    // return failure
    return false;
  }

  // init callbacks
  p->cbs = cbs;

  // init stack
  p->stack_ptr = stack_ptr;
  p->stack_len = stack_len;
  p->stack_pos = 0;

  // init state
  SWAP(p, STATE_INIT);

  // clear number of bytes read
  p->num_bytes = 0;

  // save user data pointer
  p->user_data = user_data;

  // return success
  return true;
}

void *
jiffy_parser_get_user_data(
  const jiffy_parser_t * const p
) {
  return p->user_data;
}

size_t
jiffy_parser_get_num_bytes(
  const jiffy_parser_t * const p
) {
  return p->num_bytes;
}

/**
 * Push parser state.  Returns false on stack overflow.
 *
 * Note: this is defined as an inline function rather than a macro to
 * give the compiler more flexibility in terms of inlining.
 */
static inline bool
jiffy_parser_push_state(
  jiffy_parser_t * const p,
  const jiffy_parser_state_t state
) {
  if (p->stack_pos < p->stack_len - 1) {
    p->stack_ptr[++p->stack_pos] = state;
    return true;
  } else {
    FAIL(p, JIFFY_ERR_STACK_OVERFLOW);
  }
}

/**
 * Pop parser state.  Returns false on stack underflow.
 *
 * Note: this is defined as an inline function rather than a macro to
 * give the compiler more flexibility in terms of inlining.
 */
static inline bool
jiffy_parser_pop_state(
  jiffy_parser_t * const p
) {
  // check for underflow
  if (!p->stack_pos) {
    // got underflow, return error
    FAIL(p, JIFFY_ERR_STACK_UNDERFLOW);
  }

  // decriment position
  p->stack_pos--;

  // check for done
  if (!p->stack_pos && GET_STATE(p) == STATE_INIT) {
    SWAP(p, STATE_DONE);
  }

  // return success
  return true;
}

// push parser state and return false if an error occurred.
#define PUSH(p, state) do { \
  if (!jiffy_parser_push_state((p), (state))) { \
    return false; \
  } \
} while (0)

// pop parser state and return false if an error occurred.
#define POP(p) do { \
  if (!jiffy_parser_pop_state(p)) { \
    return false; \
  } \
} while (0)

/**
 * Emit given unicode code point as UTF-8.  Returns false if the given
 * code point is outside of the valid range of Unicode code points
 * (e.g., (0, 0x110000), exclusive).
 */
static bool
jiffy_parser_emit_utf8(
  jiffy_parser_t * const p,
  const uint32_t code
) {
  if (!code) {
    return false;
  } else if (code < 0x80) {
    EMIT(p, on_string_byte, code);
    return true;
  } else if (code < 0x0800) {
    EMIT(p, on_string_byte, (0x03 << 6) | ((code >> 6) & 0x1f));
    EMIT(p, on_string_byte, (0x01 << 7) | (code & 0x3f));
    return true;
  } else if (code < 0x10000) {
    EMIT(p, on_string_byte, (0x0e << 4) | ((code >> 12) & 0x0f));
    EMIT(p, on_string_byte, (0x01 << 7) | ((code >> 6) & 0x3f));
    EMIT(p, on_string_byte, (0x01 << 7) | (code & 0x3f));
    return true;
  } else if (code < 0x110000) {
    EMIT(p, on_string_byte, (0x0f << 4) | ((code >> 18) & 0x07));
    EMIT(p, on_string_byte, (0x01 << 7) | ((code >> 12) & 0x3f));
    EMIT(p, on_string_byte, (0x01 << 7) | ((code >> 6) & 0x3f));
    EMIT(p, on_string_byte, (0x01 << 7) | (code & 0x3f));
    return true;
  } else {
    // this should never be reached, but just in case
    return false;
  }
}

static bool
jiffy_parser_push_byte(
  jiffy_parser_t * const p,
  const uint8_t byte
) {
retry:
  switch (GET_STATE(p)) {
  case STATE_FAIL:
    return false;
  case STATE_DONE:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_INIT:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    default:
      PUSH(p, STATE_VALUE);
      goto retry;
    }

    break;
  case STATE_VALUE:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case 'n':
      SWAP(p, STATE_LIT_N);
      break;
    case 't':
      SWAP(p, STATE_LIT_T);
      break;
    case 'f':
      SWAP(p, STATE_LIT_F);
      break;
    case '+':
    case '-':
      SWAP(p, STATE_NUMBER_AFTER_SIGN);
      FIRE(p, on_number_start);
      EMIT(p, on_number_byte, byte);
      break;
    case '0':
      SWAP(p, STATE_NUMBER_AFTER_LEADING_ZERO);
      FIRE(p, on_number_start);
      EMIT(p, on_number_byte, byte);
      break;
    CASE_NONZERO_NUMBER
      SWAP(p, STATE_NUMBER_INT);
      FIRE(p, on_number_start);
      EMIT(p, on_number_byte, byte);
      break;
    case '{':
      SWAP(p, STATE_OBJECT_START);
      FIRE(p, on_object_start);
      break;
    case '[':
      SWAP(p, STATE_ARRAY_START);
      FIRE(p, on_array_start);
      break;
    case '"':
      SWAP(p, STATE_STRING);
      FIRE(p, on_string_start);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_N:
    if (byte == 'u') {
      SWAP(p, STATE_LIT_NU);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_NU:
    if (byte == 'l') {
      SWAP(p, STATE_LIT_NUL);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_NUL:
    if (byte == 'l') {
      FIRE(p, on_null);
      POP(p);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_T:
    if (byte == 'r') {
      SWAP(p, STATE_LIT_TR);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_TR:
    if (byte == 'u') {
      SWAP(p, STATE_LIT_TRU);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_TRU:
    if (byte == 'e') {
      FIRE(p, on_true);
      POP(p);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_F:
    if (byte == 'a') {
      SWAP(p, STATE_LIT_FA);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_FA:
    if (byte == 'l') {
      SWAP(p, STATE_LIT_FAL);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_FAL:
    if (byte == 's') {
      SWAP(p, STATE_LIT_FALS);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_LIT_FALS:
    if (byte == 'e') {
      FIRE(p, on_false);
      POP(p);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_AFTER_SIGN:
    switch (byte) {
    case '0':
      SWAP(p, STATE_NUMBER_AFTER_LEADING_ZERO);
      EMIT(p, on_number_byte, byte);
      break;
    CASE_NONZERO_NUMBER
      SWAP(p, STATE_NUMBER_INT);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_AFTER_LEADING_ZERO:
    if (byte == '.') {
      SWAP(p, STATE_NUMBER_AFTER_DOT);
      EMIT(p, on_number_byte, byte);
    } else if (byte == 'e' || byte == 'E') {
      SWAP(p, STATE_NUMBER_AFTER_EXP);
      EMIT(p, on_number_byte, byte);
    } else {
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_NUMBER_INT:
    switch (byte) {
    CASE_NUMBER
      EMIT(p, on_number_byte, byte);
      break;
    case '.':
      SWAP(p, STATE_NUMBER_AFTER_DOT);
      EMIT(p, on_number_byte, byte);
      break;
    case 'e':
    case 'E':
      SWAP(p, STATE_NUMBER_AFTER_EXP);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_NUMBER_AFTER_DOT:
    switch (byte) {
    CASE_NUMBER
      SWAP(p, STATE_NUMBER_FRAC);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_FRAC:
    switch (byte) {
    CASE_NUMBER
      EMIT(p, on_number_byte, byte);
      break;
    case 'e':
    case 'E':
      SWAP(p, STATE_NUMBER_AFTER_EXP);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_NUMBER_AFTER_EXP:
    switch (byte) {
    case '+':
    case '-':
      SWAP(p, STATE_NUMBER_AFTER_EXP_SIGN);
      EMIT(p, on_number_byte, byte);
      break;
    CASE_NUMBER
      SWAP(p, STATE_NUMBER_EXP_NUM);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_AFTER_EXP_SIGN:
    switch (byte) {
    CASE_NUMBER
      SWAP(p, STATE_NUMBER_EXP_NUM);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_EXP_NUM:
    switch (byte) {
    CASE_NUMBER
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_STRING:
    switch (byte) {
    case '"':
      FIRE(p, on_string_end);
      POP(p);
      break;
    case '\\':
      PUSH(p, STATE_STRING_ESC);
      break;
    case 0:
    case '\r':
    case '\n':
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    default:
      EMIT(p, on_string_byte, byte);
    }

    break;
  case STATE_STRING_ESC:
    switch (byte) {
    case '\\':
      EMIT(p, on_string_byte, '\\');
      POP(p);
      break;
    case '/':
      EMIT(p, on_string_byte, '/');
      POP(p);
      break;
    case '\"':
      EMIT(p, on_string_byte, '\"');
      POP(p);
      break;
    case 'n':
      EMIT(p, on_string_byte, '\n');
      POP(p);
      break;
    case 'r':
      EMIT(p, on_string_byte, '\r');
      POP(p);
      break;
    case 't':
      EMIT(p, on_string_byte, '\t');
      POP(p);
      break;
    case 'v':
      EMIT(p, on_string_byte, '\v');
      POP(p);
      break;
    case 'f':
      EMIT(p, on_string_byte, '\f');
      POP(p);
      break;
    case 'b':
      EMIT(p, on_string_byte, '\b');
      POP(p);
      break;
    case 'u':
      SWAP(p, STATE_STRING_UNICODE);
      p->hex = 0;
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_ESCAPE);
    }

    break;
  case STATE_STRING_UNICODE:
    switch (byte) {
    CASE_HEX
      p->hex = (p->hex << 4) + nibble(byte);
      SWAP(p, STATE_STRING_UNICODE_X);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case STATE_STRING_UNICODE_X:
    switch (byte) {
    CASE_HEX
      p->hex = (p->hex << 4) + nibble(byte);
      SWAP(p, STATE_STRING_UNICODE_XX);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case STATE_STRING_UNICODE_XX:
    switch (byte) {
    CASE_HEX
      p->hex = (p->hex << 4) + nibble(byte);
      SWAP(p, STATE_STRING_UNICODE_XXX);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case STATE_STRING_UNICODE_XXX:
    switch (byte) {
    CASE_HEX
      p->hex = (p->hex << 4) + nibble(byte);
      if (!jiffy_parser_emit_utf8(p, p->hex)) {
        FAIL(p, JIFFY_ERR_BAD_UNICODE_CODEPOINT);
      }
      POP(p);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case STATE_ARRAY_START:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ']':
      FIRE(p, on_array_end);
      POP(p);
      break;
    case ',':
      FAIL(p, JIFFY_ERR_EXPECTED_ARRAY_ELEMENT);
    default:
      PUSH(p, STATE_ARRAY_ELEMENT);
      PUSH(p, STATE_VALUE);
      FIRE(p, on_array_element_start);
      goto retry;
    }

    break;
  case STATE_ARRAY_ELEMENT:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ']':
      // end element
      FIRE(p, on_array_element_end);
      POP(p);

      // end array
      FIRE(p, on_array_end);
      POP(p);

      break;
    case ',':
      // end element
      FIRE(p, on_array_element_end);

      // start element
      PUSH(p, STATE_VALUE);
      FIRE(p, on_array_element_start);

      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_COMMA_OR_ARRAY_END);
    }

    break;
  case STATE_OBJECT_START:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case '}':
      // end object
      FIRE(p, on_object_end);
      POP(p);

      break;
    case '"':
      PUSH(p, STATE_OBJECT_KEY);
      PUSH(p, STATE_STRING);
      FIRE(p, on_object_key_start);
      FIRE(p, on_string_start);

      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_STRING_OR_OBJECT_END);
    }

    break;
  case STATE_OBJECT_KEY:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ':':
      FIRE(p, on_object_key_end);
      SWAP(p, STATE_AFTER_OBJECT_KEY);
      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_COLON);
    }

    break;
  case STATE_AFTER_OBJECT_KEY:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    default:
      SWAP(p, STATE_AFTER_OBJECT_VALUE);
      PUSH(p, STATE_VALUE);
      FIRE(p, on_object_value_start);
      goto retry;
    }

    break;
  case STATE_AFTER_OBJECT_VALUE:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ',':
      FIRE(p, on_object_value_end);
      SWAP(p, STATE_BEFORE_OBJECT_KEY);
      break;
    case '}':
      FIRE(p, on_object_value_end);
      POP(p);
      FIRE(p, on_object_end);
      POP(p);
      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_COMMA_OR_OBJECT_END);
    }

    break;
  case STATE_BEFORE_OBJECT_KEY:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case '"':
      SWAP(p, STATE_OBJECT_KEY);
      PUSH(p, STATE_STRING);
      FIRE(p, on_object_key_start);
      FIRE(p, on_string_start);
      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_OBJECT_KEY);
    }

    break;
  default:
    FAIL(p, JIFFY_ERR_BAD_STATE);
  }

  // increment byte count
  p->num_bytes++;

  // return success
  return true;
}

bool
jiffy_parser_push(
  jiffy_parser_t * const p,
  const void * const ptr,
  const size_t len
) {
  const uint8_t * const buf = ptr;

  for (size_t i = 0; i < len; i++) {
    // parse byte, check for error
    if (!jiffy_parser_push_byte(p, buf[i])) {
      // return failure
      return false;
    }
  }

  // return success
  return true;
}

bool
jiffy_parser_fini(
  jiffy_parser_t * const p
) {
  // push a single space; this will flush any pending numbers
  if (!jiffy_parser_push_byte(p, ' ')) {
    return false;
  }

  // check to see if parsing is done
  if (p->stack_pos || GET_STATE(p) != STATE_DONE) {
    FAIL(p, JIFFY_ERR_NOT_DONE);
  }

  // return success
  return true;
}

bool
jiffy_parse(
  const jiffy_parser_cbs_t * const cbs,
  jiffy_parser_state_t * const stack_ptr,
  const size_t stack_len,
  const void * const buf,
  const size_t len,
  void * const user_data
) {
  jiffy_parser_t p;

  // init parser, check for error
  if (!jiffy_parser_init(&p, cbs, stack_ptr, stack_len, user_data)) {
    // return failure
    return false;
  }

  // write data, check for error
  if (!jiffy_parser_push(&p, buf, len)) {
    // return failure
    return false;
  }

  // finalize parser, check for error
  if (!jiffy_parser_fini(&p)) {
    // return failure
    return false;
  }

  // return success
  return true;
}