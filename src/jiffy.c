#include <stdbool.h> // bool
#include <stdlib.h> // malloc(), free()
#include "jiffy.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

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
#define JIFFY_DEF_ERR(a, b) b
JIFFY_ERROR_LIST
#undef JIFFY_DEF_ERR
};

const char *
jiffy_err_to_s(
  const jiffy_err_t err
) {
  const size_t ofs = err < JIFFY_ERR_LAST ? err : JIFFY_ERR_LAST;
  return JIFFY_ERRORS[ofs];
}

/**
 * Warning strings.  Used by jiffy_warning_to_s().
 */
static const char *
JIFFY_WARNINGS[] = {
#define JIFFY_DEF_WARNING(a, b) b
JIFFY_WARNING_LIST
#undef JIFFY_DEF_WARNING
};

const char *
jiffy_warning_to_s(
  const jiffy_warning_t code
) {
  const size_t ofs = (code < JIFFY_WARNING_LAST) ? code : JIFFY_WARNING_LAST;
  return JIFFY_WARNINGS[ofs];
}

/**
 * Parser states.
 */
#define JIFFY_PARSER_STATE_LIST \
  JIFFY_DEF_PARSER_STATE(INIT), \
  JIFFY_DEF_PARSER_STATE(DONE), \
  JIFFY_DEF_PARSER_STATE(FAIL), \
  JIFFY_DEF_PARSER_STATE(BOM_UTF16_X), \
  JIFFY_DEF_PARSER_STATE(BOM_UTF8_X), \
  JIFFY_DEF_PARSER_STATE(BOM_UTF8_XX), \
  JIFFY_DEF_PARSER_STATE(VALUE), \
  JIFFY_DEF_PARSER_STATE(LIT_N), \
  JIFFY_DEF_PARSER_STATE(LIT_NU), \
  JIFFY_DEF_PARSER_STATE(LIT_NUL), \
  JIFFY_DEF_PARSER_STATE(LIT_T), \
  JIFFY_DEF_PARSER_STATE(LIT_TR), \
  JIFFY_DEF_PARSER_STATE(LIT_TRU), \
  JIFFY_DEF_PARSER_STATE(LIT_F), \
  JIFFY_DEF_PARSER_STATE(LIT_FA), \
  JIFFY_DEF_PARSER_STATE(LIT_FAL), \
  JIFFY_DEF_PARSER_STATE(LIT_FALS), \
  JIFFY_DEF_PARSER_STATE(NUMBER_AFTER_SIGN), \
  JIFFY_DEF_PARSER_STATE(NUMBER_AFTER_LEADING_ZERO), \
  JIFFY_DEF_PARSER_STATE(NUMBER_INT), \
  JIFFY_DEF_PARSER_STATE(NUMBER_AFTER_DOT), \
  JIFFY_DEF_PARSER_STATE(NUMBER_FRAC), \
  JIFFY_DEF_PARSER_STATE(NUMBER_AFTER_EXP), \
  JIFFY_DEF_PARSER_STATE(NUMBER_AFTER_EXP_SIGN), \
  JIFFY_DEF_PARSER_STATE(NUMBER_EXP_NUM), \
  JIFFY_DEF_PARSER_STATE(STRING), \
  JIFFY_DEF_PARSER_STATE(STRING_ESC), \
  JIFFY_DEF_PARSER_STATE(STRING_UNICODE), \
  JIFFY_DEF_PARSER_STATE(STRING_UNICODE_X), \
  JIFFY_DEF_PARSER_STATE(STRING_UNICODE_XX), \
  JIFFY_DEF_PARSER_STATE(STRING_UNICODE_XXX), \
  JIFFY_DEF_PARSER_STATE(OBJECT_START), \
  JIFFY_DEF_PARSER_STATE(ARRAY_START), \
  JIFFY_DEF_PARSER_STATE(ARRAY_ELEMENT), \
  JIFFY_DEF_PARSER_STATE(OBJECT_KEY), \
  JIFFY_DEF_PARSER_STATE(AFTER_OBJECT_KEY), \
  JIFFY_DEF_PARSER_STATE(BEFORE_OBJECT_KEY), \
  JIFFY_DEF_PARSER_STATE(AFTER_OBJECT_VALUE), \
  JIFFY_DEF_PARSER_STATE(LAST),

/**
 * Parser states.
 */
enum jiffy_parser_states {
#define JIFFY_DEF_PARSER_STATE(a) PARSER_STATE_##a
JIFFY_PARSER_STATE_LIST
#undef JIFFY_DEF_PARSER_STATE
};

/**
 * Parser state strings.  Used by jiffy_parser_state_to_s().
 */
static const char *
JIFFY_PARSER_STATES[] = {
#define JIFFY_DEF_PARSER_STATE(a) "PARSER_STATE_" # a
JIFFY_PARSER_STATE_LIST
#undef JIFFY_DEF_PARSER_STATE
};

const char *
jiffy_parser_state_to_s(
  const jiffy_parser_state_t state
) {
  const size_t ofs = (state < PARSER_STATE_LAST) ? state : PARSER_STATE_LAST;
  return JIFFY_PARSER_STATES[ofs];
}

// get the current state
#define GET_STATE(p) ((p)->stack_ptr[(p)->stack_pos])

// set the current state
#define SWAP(p, state) (p)->stack_ptr[(p)->stack_pos] = (state)

// invoke on_error callback with error code, set the state to
// PARSER_STATE_FAIL, and then return false.
#define FAIL(p, err) do { \
  if ((p)->cbs && (p)->cbs->on_error) { \
    (p)->cbs->on_error((p), err); \
  } \
  SWAP((p), PARSER_STATE_FAIL); \
  return false; \
} while (0)

// call given callback, if it is non-NULL
#define FIRE(p, cb_name) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p); \
  } \
} while (0)

// call given callback with value, if it is non-NULL
#define EMIT(p, cb_name, val) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p, (val)); \
  } \
} while (0)

// call given callback with byte value, if it is non-NULL
#define EMIT_FLAGS(p, cb_name, flags) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p, (flags)); \
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
  SWAP(p, PARSER_STATE_INIT);

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
  if (!p->stack_pos && GET_STATE(p) == PARSER_STATE_INIT) {
    SWAP(p, PARSER_STATE_DONE);
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
  case PARSER_STATE_FAIL:
    return false;
  case PARSER_STATE_DONE:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_INIT:
    switch (byte) {
    case 0xFE:
      PUSH(p, PARSER_STATE_BOM_UTF16_X);
      break;
    case 0xEF:
      PUSH(p, PARSER_STATE_BOM_UTF8_X);
      break;
    default:
      PUSH(p, PARSER_STATE_VALUE);
      goto retry;
    }

    break;
  case PARSER_STATE_BOM_UTF16_X:
    switch (byte) {
    case 0xFF:
      EMIT(p, on_warning, JIFFY_WARNING_UTF16_BOM);
      SWAP(p, PARSER_STATE_VALUE);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UTF16_BOM);
    }

    break;
  case PARSER_STATE_BOM_UTF8_X:
    switch (byte) {
    case 0xBB:
      SWAP(p, PARSER_STATE_BOM_UTF8_XX);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UTF8_BOM);
    }

    break;
  case PARSER_STATE_BOM_UTF8_XX:
    switch (byte) {
    case 0xBF:
      EMIT(p, on_warning, JIFFY_WARNING_UTF8_BOM);
      POP(p);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UTF8_BOM);
    }

    break;
  case PARSER_STATE_VALUE:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case 'n':
      SWAP(p, PARSER_STATE_LIT_N);
      break;
    case 't':
      SWAP(p, PARSER_STATE_LIT_T);
      break;
    case 'f':
      SWAP(p, PARSER_STATE_LIT_F);
      break;
    case '+':
    case '-':
      // clear number flags
      p->v_num.flags = 0;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_SIGN);
      FIRE(p, on_number_start);
      EMIT(p, on_number_byte, byte);

      break;
    case '0':
      // clear number flags
      p->v_num.flags = 0;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_LEADING_ZERO);
      FIRE(p, on_number_start);
      EMIT(p, on_number_byte, byte);

      break;
    CASE_NONZERO_NUMBER
      // clear number flags
      p->v_num.flags = 0;

      SWAP(p, PARSER_STATE_NUMBER_INT);
      FIRE(p, on_number_start);
      EMIT(p, on_number_byte, byte);

      break;
    case '{':
      SWAP(p, PARSER_STATE_OBJECT_START);
      FIRE(p, on_object_start);
      break;
    case '[':
      SWAP(p, PARSER_STATE_ARRAY_START);
      FIRE(p, on_array_start);
      break;
    case '"':
      SWAP(p, PARSER_STATE_STRING);
      FIRE(p, on_string_start);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_N:
    if (byte == 'u') {
      SWAP(p, PARSER_STATE_LIT_NU);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_NU:
    if (byte == 'l') {
      SWAP(p, PARSER_STATE_LIT_NUL);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_NUL:
    if (byte == 'l') {
      FIRE(p, on_null);
      POP(p);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_T:
    if (byte == 'r') {
      SWAP(p, PARSER_STATE_LIT_TR);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_TR:
    if (byte == 'u') {
      SWAP(p, PARSER_STATE_LIT_TRU);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_TRU:
    if (byte == 'e') {
      FIRE(p, on_true);
      POP(p);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_F:
    if (byte == 'a') {
      SWAP(p, PARSER_STATE_LIT_FA);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_FA:
    if (byte == 'l') {
      SWAP(p, PARSER_STATE_LIT_FAL);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_FAL:
    if (byte == 's') {
      SWAP(p, PARSER_STATE_LIT_FALS);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_LIT_FALS:
    if (byte == 'e') {
      FIRE(p, on_false);
      POP(p);
    } else {
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_NUMBER_AFTER_SIGN:
    switch (byte) {
    case '0':
      SWAP(p, PARSER_STATE_NUMBER_AFTER_LEADING_ZERO);
      EMIT(p, on_number_byte, byte);
      break;
    CASE_NONZERO_NUMBER
      SWAP(p, PARSER_STATE_NUMBER_INT);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_NUMBER_AFTER_LEADING_ZERO:
    if (byte == '.') {
      // set fraction flag
      p->v_num.flags |= JIFFY_NUMBER_FLAG_FRAC;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_DOT);
      EMIT(p, on_number_byte, byte);
    } else if (byte == 'e' || byte == 'E') {
      // set exponent flag
      p->v_num.flags |= JIFFY_NUMBER_FLAG_EXP;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_EXP);
      EMIT(p, on_number_byte, byte);
    } else {
      EMIT_FLAGS(p, on_number_flags, p->v_num.flags);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case PARSER_STATE_NUMBER_INT:
    switch (byte) {
    CASE_NUMBER
      EMIT(p, on_number_byte, byte);
      break;
    case '.':
      // set fraction flag
      p->v_num.flags |= JIFFY_NUMBER_FLAG_FRAC;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_DOT);
      EMIT(p, on_number_byte, byte);
      break;
    case 'e':
    case 'E':
      // set exponent flag
      p->v_num.flags |= JIFFY_NUMBER_FLAG_EXP;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_EXP);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      EMIT_FLAGS(p, on_number_flags, p->v_num.flags);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case PARSER_STATE_NUMBER_AFTER_DOT:
    switch (byte) {
    CASE_NUMBER
      // set fraction flag
      p->v_num.flags |= JIFFY_NUMBER_FLAG_FRAC;

      SWAP(p, PARSER_STATE_NUMBER_FRAC);
      EMIT(p, on_number_byte, byte);

      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_NUMBER_FRAC:
    switch (byte) {
    CASE_NUMBER
      EMIT(p, on_number_byte, byte);
      break;
    case 'e':
    case 'E':
      // set exponent flag
      p->v_num.flags |= JIFFY_NUMBER_FLAG_EXP;

      SWAP(p, PARSER_STATE_NUMBER_AFTER_EXP);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      EMIT_FLAGS(p, on_number_flags, p->v_num.flags);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case PARSER_STATE_NUMBER_AFTER_EXP:
    switch (byte) {
    case '+':
    case '-':
      SWAP(p, PARSER_STATE_NUMBER_AFTER_EXP_SIGN);
      EMIT(p, on_number_byte, byte);
      break;
    CASE_NUMBER
      SWAP(p, PARSER_STATE_NUMBER_EXP_NUM);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_NUMBER_AFTER_EXP_SIGN:
    switch (byte) {
    CASE_NUMBER
      SWAP(p, PARSER_STATE_NUMBER_EXP_NUM);
      EMIT(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case PARSER_STATE_NUMBER_EXP_NUM:
    switch (byte) {
    CASE_NUMBER
      EMIT(p, on_number_byte, byte);
      break;
    default:
      EMIT_FLAGS(p, on_number_flags, p->v_num.flags);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case PARSER_STATE_STRING:
    switch (byte) {
    case '"':
      FIRE(p, on_string_end);
      POP(p);
      break;
    case '\\':
      PUSH(p, PARSER_STATE_STRING_ESC);
      break;
    case 0:
    case '\r':
    case '\n':
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    default:
      EMIT(p, on_string_byte, byte);
    }

    break;
  case PARSER_STATE_STRING_ESC:
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
      p->v_str.hex = 0;
      SWAP(p, PARSER_STATE_STRING_UNICODE);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_ESCAPE);
    }

    break;
  case PARSER_STATE_STRING_UNICODE:
    switch (byte) {
    CASE_HEX
      p->v_str.hex = (p->v_str.hex << 4) + nibble(byte);
      SWAP(p, PARSER_STATE_STRING_UNICODE_X);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case PARSER_STATE_STRING_UNICODE_X:
    switch (byte) {
    CASE_HEX
      p->v_str.hex = (p->v_str.hex << 4) + nibble(byte);
      SWAP(p, PARSER_STATE_STRING_UNICODE_XX);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case PARSER_STATE_STRING_UNICODE_XX:
    switch (byte) {
    CASE_HEX
      p->v_str.hex = (p->v_str.hex << 4) + nibble(byte);
      SWAP(p, PARSER_STATE_STRING_UNICODE_XXX);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case PARSER_STATE_STRING_UNICODE_XXX:
    switch (byte) {
    CASE_HEX
      p->v_str.hex = (p->v_str.hex << 4) + nibble(byte);
      if (!jiffy_parser_emit_utf8(p, p->v_str.hex)) {
        FAIL(p, JIFFY_ERR_BAD_UNICODE_CODEPOINT);
      }
      POP(p);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_UNICODE_ESCAPE);
    }

    break;
  case PARSER_STATE_ARRAY_START:
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
      PUSH(p, PARSER_STATE_ARRAY_ELEMENT);
      PUSH(p, PARSER_STATE_VALUE);
      FIRE(p, on_array_element_start);
      goto retry;
    }

    break;
  case PARSER_STATE_ARRAY_ELEMENT:
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
      PUSH(p, PARSER_STATE_VALUE);
      FIRE(p, on_array_element_start);

      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_COMMA_OR_ARRAY_END);
    }

    break;
  case PARSER_STATE_OBJECT_START:
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
      PUSH(p, PARSER_STATE_OBJECT_KEY);
      PUSH(p, PARSER_STATE_STRING);
      FIRE(p, on_object_key_start);
      FIRE(p, on_string_start);

      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_STRING_OR_OBJECT_END);
    }

    break;
  case PARSER_STATE_OBJECT_KEY:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ':':
      FIRE(p, on_object_key_end);
      SWAP(p, PARSER_STATE_AFTER_OBJECT_KEY);
      break;
    default:
      FAIL(p, JIFFY_ERR_EXPECTED_COLON);
    }

    break;
  case PARSER_STATE_AFTER_OBJECT_KEY:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    default:
      SWAP(p, PARSER_STATE_AFTER_OBJECT_VALUE);
      PUSH(p, PARSER_STATE_VALUE);
      FIRE(p, on_object_value_start);
      goto retry;
    }

    break;
  case PARSER_STATE_AFTER_OBJECT_VALUE:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ',':
      FIRE(p, on_object_value_end);
      SWAP(p, PARSER_STATE_BEFORE_OBJECT_KEY);
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
  case PARSER_STATE_BEFORE_OBJECT_KEY:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case '"':
      SWAP(p, PARSER_STATE_OBJECT_KEY);
      PUSH(p, PARSER_STATE_STRING);
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
  if (p->stack_pos || GET_STATE(p) != PARSER_STATE_DONE) {
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

static const char *
JIFFY_TYPES[] = {
#define JIFFY_DEF_TYPE(a, b) b
JIFFY_TYPE_LIST
#undef JIFFY_DEF_TYPE
};

const char *
jiffy_type_to_s(
  const jiffy_type_t type
) {
  const size_t ofs = (type < JIFFY_TYPE_LAST) ? type : JIFFY_TYPE_LAST;
  return JIFFY_TYPES[ofs];
}

jiffy_type_t
jiffy_value_get_type(
  const jiffy_value_t * const val
) {
  return val->type;
}

const uint8_t *
jiffy_number_get_bytes(
  const jiffy_value_t * const val,
  size_t * const r_len
) {
  // value is a number
  const bool is_valid = jiffy_value_get_type(val) == JIFFY_TYPE_NUMBER;

  if (is_valid && r_len) {
    *r_len = val->v_num.len;
  }

  return is_valid ? val->v_num.ptr : NULL;
}

const uint8_t *
jiffy_string_get_bytes(
  const jiffy_value_t * const val,
  size_t * const r_len
) {
  // value is a string
  const bool is_valid = jiffy_value_get_type(val) == JIFFY_TYPE_STRING;

  if (is_valid && r_len) {
    *r_len = val->v_str.len;
  }

  return is_valid ? val->v_str.ptr : NULL;
}

size_t
jiffy_array_get_size(
  const jiffy_value_t * const val
) {
  return val->v_ary.len;
}

const jiffy_value_t *
jiffy_array_get_nth(
  const jiffy_value_t * const val,
  const size_t ofs
) {
  const bool is_valid = (
    // value is an array
    (jiffy_value_get_type(val) == JIFFY_TYPE_ARRAY) &&

    // offset is in bounds
    (ofs < val->v_ary.len)
  );

  return is_valid ? val->v_ary.vals[ofs] : NULL;
}

size_t
jiffy_object_get_size(
  const jiffy_value_t * const val
) {
  return val->v_obj.len;
}

const jiffy_value_t *
jiffy_object_get_nth_key(
  const jiffy_value_t * const val,
  const size_t ofs
) {
  const bool is_valid = (
    // value is an object
    (jiffy_value_get_type(val) == JIFFY_TYPE_OBJECT) &&

    // offset is in bounds
    (ofs < val->v_obj.len)
  );

  return is_valid ? val->v_obj.vals[2 * ofs] : NULL;
}

const jiffy_value_t *
jiffy_object_get_nth_value(
  const jiffy_value_t * const val,
  const size_t ofs
) {
  const bool is_valid = (
    // value is an object
    (jiffy_value_get_type(val) == JIFFY_TYPE_OBJECT) &&

    // offset is in bounds
    (ofs < val->v_obj.len)
  );

  return is_valid ? val->v_obj.vals[2 * ofs + 1] : NULL;
}

typedef struct {
  // number of bytes in numbers and strings
  size_t num_bytes;

  // total number of values (including objects)
  size_t num_vals;

  // total number of objects
  size_t num_objs;

  // total number of object key/value pairs
  size_t num_obj_rows;

  // total number of array values across all arrays
  size_t num_ary_rows;

  // current and maximum depth
  size_t curr_depth,
         max_depth;

  // error encountered during scan
  jiffy_err_t err;
} jiffy_tree_scan_data_t;

static void
on_tree_scan_byte(
 const jiffy_parser_t * const p,
 const uint8_t byte
) {
  (void) byte;
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->num_bytes++;
}

static void
on_tree_scan_val(
 const jiffy_parser_t * const p
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->num_vals++;
}

static void
on_tree_scan_container_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->curr_depth++;
  scan_data->max_depth = MAX(scan_data->curr_depth, scan_data->max_depth);
  on_tree_scan_val(p);
}

static void
on_tree_scan_container_end(
 const jiffy_parser_t * const p
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->curr_depth--;
}

static void
on_tree_scan_array_start(
 const jiffy_parser_t * const p
) {
  on_tree_scan_container_start(p);
}

static void
on_tree_scan_array_element_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->num_ary_rows++;
}

static void
on_tree_scan_object_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  on_tree_scan_container_start(p);
  scan_data->num_objs++;
}

static void
on_tree_scan_object_key_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->num_obj_rows++;
}

static void
on_tree_scan_error(
 const jiffy_parser_t * const p,
 const jiffy_err_t err
) {
  jiffy_tree_scan_data_t * const scan_data = jiffy_parser_get_user_data(p);
  scan_data->err = err;
}

static const jiffy_parser_cbs_t
TREE_SCAN_CBS = {
  .on_null                = on_tree_scan_val,
  .on_true                = on_tree_scan_val,
  .on_false               = on_tree_scan_val,
  .on_number_start        = on_tree_scan_val,
  .on_string_start        = on_tree_scan_val,

  .on_array_start         = on_tree_scan_array_start,
  .on_array_end           = on_tree_scan_container_end,
  .on_array_element_start = on_tree_scan_array_element_start,

  .on_object_start        = on_tree_scan_object_start,
  .on_object_end          = on_tree_scan_container_end,
  .on_object_key_start    = on_tree_scan_object_key_start,

  .on_number_byte         = on_tree_scan_byte,
  .on_string_byte         = on_tree_scan_byte,

  .on_error               = on_tree_scan_error,
};

static bool
jiffy_tree_scan(
  jiffy_tree_scan_data_t * const scan_data,
  jiffy_parser_state_t * const stack,
  const size_t stack_len,
  const void * const src,
  const size_t len
) {
  scan_data->num_bytes = 0;
  scan_data->num_vals = 0;
  scan_data->num_objs = 0;
  scan_data->num_ary_rows = 0;
  scan_data->num_obj_rows = 0;
  scan_data->curr_depth = 0;
  scan_data->max_depth = 0;
  scan_data->err = JIFFY_ERR_OK;

  // populate scan data
  return jiffy_parse(&TREE_SCAN_CBS, stack, stack_len, src, len, scan_data);
}

typedef struct {
  jiffy_value_t *ary;
  jiffy_value_t *val;
} jiffy_tree_parse_ary_row_t;

typedef struct {
  jiffy_value_t *obj;
  jiffy_value_t *key;
  jiffy_value_t *val;
} jiffy_tree_parse_obj_row_t;

typedef struct {
  // scan data
  const jiffy_tree_scan_data_t *scan_data;

  // output tree
  jiffy_tree_t *tree;

  // allocated data for parsing
  uint8_t *data;

  // number of values
  size_t num_vals;

  // container stack (pointer into data)
  jiffy_value_t **stack;
  size_t stack_pos;

  // array/value pairs (pointer into data)
  jiffy_tree_parse_ary_row_t *ary_rows;
  size_t num_ary_rows;

  // object/key/value tuples (pointer into data)
  jiffy_tree_parse_obj_row_t *obj_rows;
  size_t num_obj_rows;

  // pointer to output byte data
  uint8_t *bytes;
  size_t num_bytes;

  jiffy_err_t err;
} jiffy_tree_parse_data_t;

static void
on_tree_parse_null(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->tree->vals[parse_data->num_vals++].type = JIFFY_TYPE_NULL;
}

static void
on_tree_parse_true(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->tree->vals[parse_data->num_vals++].type = JIFFY_TYPE_TRUE;
}

static void
on_tree_parse_false(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->tree->vals[parse_data->num_vals++].type = JIFFY_TYPE_FALSE;
}

static void
on_tree_parse_number_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  jiffy_value_t * const val = parse_data->tree->vals + parse_data->num_vals;
  parse_data->num_vals++;

  val->type = JIFFY_TYPE_NUMBER;
  val->v_num.ptr = parse_data->bytes + parse_data->num_bytes;
  val->v_num.len = 0;
}

static void
on_tree_parse_number_end(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  jiffy_value_t * const val = parse_data->tree->vals + parse_data->num_vals - 1;
  val->v_num.len = parse_data->bytes + parse_data->num_bytes - val->v_num.ptr;
}

static void
on_tree_parse_number_byte(
 const jiffy_parser_t * const p,
 const uint8_t byte
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->bytes[parse_data->num_bytes++] = byte;
}

static void
on_tree_parse_string_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  jiffy_value_t * const val = parse_data->tree->vals + parse_data->num_vals;
  parse_data->num_vals++;

  val->type = JIFFY_TYPE_STRING;
  val->v_str.ptr = parse_data->bytes + parse_data->num_bytes;
  val->v_str.len = 0;
}

static void
on_tree_parse_string_end(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  jiffy_value_t * const val = parse_data->tree->vals + parse_data->num_vals - 1;

  val->v_str.len = parse_data->bytes + parse_data->num_bytes - val->v_str.ptr;
}

static void
on_tree_parse_string_byte(
 const jiffy_parser_t * const p,
 const uint8_t byte
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->bytes[parse_data->num_bytes++] = byte;
}

static void
on_tree_parse_array_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  jiffy_value_t * const val = parse_data->tree->vals + parse_data->num_vals;
  parse_data->num_vals++;

  val->type = JIFFY_TYPE_ARRAY;
  val->v_ary.len = 0;

  parse_data->stack[parse_data->stack_pos++] = val;
}

static void
on_tree_parse_array_end(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->stack_pos--;
}

static void
on_tree_parse_array_element_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  const size_t ofs = parse_data->num_ary_rows;

  // populate array row
  parse_data->ary_rows[ofs].ary = parse_data->stack[parse_data->stack_pos - 1];
  parse_data->ary_rows[ofs].val = parse_data->tree->vals + parse_data->num_vals;

  // increment array element count
  parse_data->ary_rows[ofs].ary->v_ary.len++;

  // increment total array row count
  parse_data->num_ary_rows++;
}

static void
on_tree_parse_object_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  jiffy_value_t * const val = parse_data->tree->vals + parse_data->num_vals;
  parse_data->num_vals++;

  val->type = JIFFY_TYPE_OBJECT;
  val->v_obj.len = 0;

  parse_data->stack[parse_data->stack_pos++] = val;
}

static void
on_tree_parse_object_end(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->stack_pos--;
}

static void
on_tree_parse_object_key_start(
 const jiffy_parser_t * const p
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  const size_t ofs = parse_data->num_obj_rows;

  // populate object row
  parse_data->obj_rows[ofs].obj = parse_data->stack[parse_data->stack_pos - 1];
  parse_data->obj_rows[ofs].key = parse_data->tree->vals + parse_data->num_vals;
  parse_data->obj_rows[ofs].val = parse_data->tree->vals + parse_data->num_vals + 1;

  // increment object row count
  parse_data->obj_rows[ofs].obj->v_obj.len++;

  // increment total object row count
  parse_data->num_obj_rows++;
}

static void
on_tree_parse_error(
 const jiffy_parser_t * const p,
 const jiffy_err_t err
) {
  jiffy_tree_parse_data_t *parse_data = jiffy_parser_get_user_data(p);
  parse_data->err = err;
}

static const jiffy_parser_cbs_t
TREE_PARSE_CBS = {
  .on_null                = on_tree_parse_null,
  .on_true                = on_tree_parse_true,
  .on_false               = on_tree_parse_false,

  .on_number_start        = on_tree_parse_number_start,
  .on_number_end          = on_tree_parse_number_end,
  .on_number_byte         = on_tree_parse_number_byte,

  .on_string_start        = on_tree_parse_string_start,
  .on_string_end          = on_tree_parse_string_end,
  .on_string_byte         = on_tree_parse_string_byte,

  .on_array_start         = on_tree_parse_array_start,
  .on_array_end           = on_tree_parse_array_end,
  .on_array_element_start = on_tree_parse_array_element_start,

  .on_object_start        = on_tree_parse_object_start,
  .on_object_end          = on_tree_parse_object_end,
  .on_object_key_start    = on_tree_parse_object_key_start,

  .on_error               = on_tree_parse_error,
};

static bool
jiffy_tree_parse(
  jiffy_tree_parse_data_t * const parse_data,
  jiffy_parser_state_t * const stack,
  const size_t stack_len,
  const void * const src,
  const size_t len
) {
  return jiffy_parse(&TREE_PARSE_CBS, stack, stack_len, src, len, parse_data);
}

static void *
jiffy_tree_parser_malloc(
  const jiffy_tree_t * const tree,
  const size_t num_bytes
) {
  if (tree->cbs && tree->cbs->malloc) {
    return tree->cbs->malloc(num_bytes, tree->user_data);
  } else {
    return malloc(num_bytes);
  }
}

static void
jiffy_tree_parser_free(
  const jiffy_tree_t * const tree,
  void * const ptr
) {
  if (tree->cbs && tree->cbs->free) {
    tree->cbs->free(ptr, tree->user_data);
  } else {
    free(ptr);
  }
}

static void *
jiffy_tree_output_malloc(
  const jiffy_tree_t * const tree,
  const jiffy_tree_scan_data_t * const scan_data
) {
  // calculate total number of bytes needed for output data
  const size_t bytes_needed = (
    // space needed for all values
    sizeof(jiffy_value_t) * scan_data->num_vals +

    // space needed for array rows
    sizeof(jiffy_value_t*) * scan_data->num_ary_rows +

    // space needed for object key/value rows
    sizeof(jiffy_value_t*) * 2 * scan_data->num_obj_rows +

    // space needed for object key/value rows
    scan_data->num_bytes
  );

  // allocate and return memory
  return jiffy_tree_parser_malloc(tree, bytes_needed);
}

static void *
jiffy_tree_parse_data_malloc(
  const jiffy_tree_t * const tree,
  const jiffy_tree_scan_data_t * const scan_data
) {
  // calculate total number of bytes needed for parse data
  const size_t bytes_needed = (
    // space for object stack
    sizeof(jiffy_value_t*) * scan_data->max_depth +

    // space for array rows
    sizeof(jiffy_tree_parse_ary_row_t) * scan_data->num_ary_rows +

    // space for object rows
    sizeof(jiffy_tree_parse_obj_row_t) * scan_data->num_obj_rows
  );

  // allocate and return memory
  return jiffy_tree_parser_malloc(tree, bytes_needed);
}

static int
tree_parse_ary_row_sort_cb(
  const void * const a_ptr,
  const void * const b_ptr
) {
  const jiffy_tree_parse_ary_row_t *a = a_ptr;
  const jiffy_tree_parse_ary_row_t *b = b_ptr;

  if (a->ary == b->ary) {
    return (a->val > b->val) ? -1 : ((a->val == b->val) ? 0 : -1);
  } else {
    return (a->ary > b->ary) ? -1 : ((a->ary == b->ary) ? 0 : -1);
  }
}

static void
tree_parse_fill_ary_rows(
  const jiffy_tree_parse_data_t * const parse_data
) {
  // sort array rows
  qsort(
    parse_data->ary_rows,
    parse_data->num_ary_rows,
    sizeof(jiffy_tree_parse_ary_row_t),
    tree_parse_ary_row_sort_cb
  );

  // get output row pointer
  jiffy_value_t ** const rows = (jiffy_value_t**) (
    parse_data->tree->data +
    sizeof(jiffy_value_t) * parse_data->tree->num_vals
  );

  jiffy_value_t *curr = NULL;
  for (size_t i = 0; i < parse_data->num_ary_rows; i++) {
    if (curr != parse_data->ary_rows[i].ary) {
      // set array vals pointer
      parse_data->ary_rows[i].ary->v_ary.vals = rows + i;
      curr = parse_data->ary_rows[i].ary;
    }

    // populate value
    rows[i] = parse_data->ary_rows[i].val;
  }
}

static int
tree_parse_obj_row_sort_cb(
  const void * const a_ptr,
  const void * const b_ptr
) {
  const jiffy_tree_parse_obj_row_t *a = a_ptr;
  const jiffy_tree_parse_obj_row_t *b = b_ptr;

  if (a->obj == b->obj) {
    return (a->val > b->val) ? -1 : ((a->val == b->val) ? 0 : -1);
  } else {
    return (a->obj > b->obj) ? -1 : ((a->obj == b->obj) ? 0 : -1);
  }
}

static void
tree_parse_fill_obj_rows(
  const jiffy_tree_parse_data_t * const parse_data
) {
  // sort object rows
  qsort(
    parse_data->obj_rows,
    parse_data->num_obj_rows,
    sizeof(jiffy_tree_parse_obj_row_t),
    tree_parse_obj_row_sort_cb
  );

  // get output pointer
  jiffy_value_t ** const vals = (jiffy_value_t**) (
    parse_data->tree->data +
    sizeof(jiffy_value_t) * parse_data->tree->num_vals +
    sizeof(jiffy_value_t*) * parse_data->num_ary_rows
  );

  jiffy_value_t *curr = NULL;
  for (size_t i = 0; i < parse_data->num_obj_rows; i++) {
    if (curr != parse_data->obj_rows[i].obj) {
      // set array vals pointer
      parse_data->obj_rows[i].obj->v_obj.vals = vals + 2 * i;
      curr = parse_data->obj_rows[i].obj;
    }

    // populate key/value
    vals[2 * i] = parse_data->obj_rows[i].key;
    vals[2 * i + 1] = parse_data->obj_rows[i].val;
  }
}

// invoke on_error callback with error code
#define TREE_FAIL(tree, err) do { \
  if ((tree)->cbs && (tree)->cbs->on_error) { \
    (tree)->cbs->on_error((tree), err); \
  } \
  return false; \
} while (0)

bool
jiffy_tree_new_ex(
  jiffy_tree_t * const tree,
  const jiffy_tree_cbs_t * const cbs,
  jiffy_parser_state_t * const stack,
  const size_t stack_len,
  const void * const src,
  const size_t len,
  void * const user_data
) {
  // check to make sure tree, is not null
  if (!tree) {
    // return failure
    return false;
  }

  // populate initial tree values
  tree->cbs = cbs;
  tree->user_data = user_data;

  // populate scan data
  jiffy_tree_scan_data_t scan_data;
  if (!jiffy_tree_scan(&scan_data, stack, stack_len, src, len)) {
    TREE_FAIL(tree, scan_data.err);
  }

  // save tree value count
  tree->data = NULL;
  tree->vals = NULL;
  tree->num_vals = scan_data.num_vals;

  if (tree->num_vals > 0) {
    // allocate output data, check for error
    tree->data = jiffy_tree_output_malloc(tree, &scan_data);
    if (!tree->data) {
      TREE_FAIL(tree, JIFFY_ERR_TREE_OUTPUT_MALLOC_FAILED);
    }

    // populate output tree data
    tree->vals = (jiffy_value_t*) tree->data;

    // allocate parse data memory, check for error
    uint8_t * const parse_mem = jiffy_tree_parse_data_malloc(tree, &scan_data);
    if (!parse_mem) {
      TREE_FAIL(tree, JIFFY_ERR_TREE_PARSE_MALLOC_FAILED);
    }

    // populate parse data
    jiffy_tree_parse_data_t parse_data = {
      // scan data
      .scan_data = &scan_data,

      // output tree
      .tree = tree,

      // allocated parse memory
      .data = parse_mem,

      // number of values parsed so far
      .num_vals = 0,

      // container stack and depth
      .stack = (jiffy_value_t**) parse_mem,
      .stack_pos = 0,

      // array rows
      .ary_rows = (jiffy_tree_parse_ary_row_t*) (
        parse_mem +
        sizeof(jiffy_value_t*) * scan_data.max_depth
      ),

      // number of array rows
      .num_ary_rows = 0,

      // object rows
      .obj_rows = (jiffy_tree_parse_obj_row_t*) (
        parse_mem +
        sizeof(jiffy_value_t*) * scan_data.max_depth +
        sizeof(jiffy_tree_parse_ary_row_t) * scan_data.num_ary_rows
      ),

      // number of object rows
      .num_obj_rows = 0,

      // byte data
      .bytes = (
        // output data pointer
        tree->data +

        // space needed for all values
        sizeof(jiffy_value_t) * scan_data.num_vals +

        // space needed for array rows
        sizeof(jiffy_value_t*) * scan_data.num_ary_rows +

        // space needed for object key/value rows
        sizeof(jiffy_value_t*) * 2 * scan_data.num_obj_rows
      ),

      // number of bytes
      .num_bytes = 0,

      // default error
      .err = JIFFY_ERR_OK,
    };

    // parse tree, check for error
    if (!jiffy_tree_parse(&parse_data, stack, stack_len, src, len)) {
      TREE_FAIL(tree, parse_data.err);
    }

    // fill array rows
    tree_parse_fill_ary_rows(&parse_data);

    // fill object rows
    tree_parse_fill_obj_rows(&parse_data);

    // free parser memory
    jiffy_tree_parser_free(tree, parse_mem);
  }

  // return success
  return true;
}

static size_t
jiffy_tree_get_required_stack_depth(
  jiffy_tree_t * const tree,
  const void * const src_ptr,
  const size_t len,
  size_t *ret_depth
) {
  const uint8_t * const src = src_ptr;
  size_t depth = 0;
  size_t max_depth = 16;
  bool in_str = false;

  for (size_t i = 0; i < len; i++) {
    if (in_str) {
      switch (src[i]) {
      case '"':
        // decriment depth
        depth -= 2;
        in_str = false;

        break;
      case '\\':
        if ((i < len - 1) && src[i + 1] == '"') {
          // skip escaped quote
          i++;
        }

        break;
      }
    } else {
      switch (src[i]) {
      case '{':
        depth += 4;
        max_depth = MAX(depth, max_depth);

        break;
      case '}':
        depth -= 4;

        break;
      case '[':
        depth += 3;
        max_depth = MAX(depth, max_depth);

        break;
      case ']':
        depth -= 3;

        break;
      case '"':
        depth += 2;
        max_depth = MAX(depth, max_depth);
        in_str = true;

        break;
      }
    }
  }

  if (depth != 0) {
    // depth is nonzero, which means string isn't valid json
    TREE_FAIL(tree, JIFFY_ERR_TREE_STACK_SCAN_FAILED);
  }

  if (ret_depth) {
    // save result
    *ret_depth = max_depth;
  }

  // return success
  return true;
}

bool
jiffy_tree_new(
  jiffy_tree_t * const tree,
  const jiffy_tree_cbs_t * const cbs,
  const void * const src,
  const size_t len,
  void * const user_data
) {
  // check to make sure tree, is not null
  if (!tree) {
    // return failure
    return false;
  }

  // populate initial tree values
  tree->cbs = cbs;
  tree->user_data = user_data;

  // get needed stack size
  size_t stack_len;
  if (!jiffy_tree_get_required_stack_depth(tree, src, len, &stack_len)) {
    // return failure
    return false;
  }

  // get number of bytes needed for stack
  const size_t num_bytes = sizeof(jiffy_parser_state_t) * stack_len;

  // alloc stack, check for error
  jiffy_parser_state_t *stack = jiffy_tree_parser_malloc(tree, num_bytes);
  if (!stack) {
    TREE_FAIL(tree, JIFFY_ERR_TREE_STACK_MALLOC_FAILED);
  }

  // parse tree, get result
  const bool r = jiffy_tree_new_ex(tree, cbs, stack, stack_len, src, len, user_data);

  // free stack
  jiffy_tree_parser_free(tree, stack);

  // return result
  return r;
}

void *
jiffy_tree_get_user_data(
  const jiffy_tree_t * const tree
) {
  return tree->user_data;
}

const jiffy_value_t *
jiffy_tree_get_root_value(
  const jiffy_tree_t * const tree
) {
  return (tree->num_vals > 0) ? tree->vals : NULL;
}

void
jiffy_tree_free(
  jiffy_tree_t * const tree
) {
  if (tree->data) {
    // free tree data
    jiffy_tree_parser_free(tree, tree->data);
    tree->data = NULL;
    tree->vals = NULL;

    // clear value count
    tree->num_vals = 0;
  }
}

/**
 * Writer states.
 */
#define JIFFY_WRITER_STATE_LIST \
  JIFFY_DEF_WRITER_STATE(INIT), \
  JIFFY_DEF_WRITER_STATE(DONE), \
  JIFFY_DEF_WRITER_STATE(FAIL), \
  JIFFY_DEF_WRITER_STATE(STRING), \
  JIFFY_DEF_WRITER_STATE(NUMBER), \
  JIFFY_DEF_WRITER_STATE(NUMBER_START), \
  JIFFY_DEF_WRITER_STATE(NUMBER_AFTER_SIGN), \
  JIFFY_DEF_WRITER_STATE(NUMBER_AFTER_LEADING_ZERO), \
  JIFFY_DEF_WRITER_STATE(NUMBER_INT), \
  JIFFY_DEF_WRITER_STATE(NUMBER_AFTER_DOT), \
  JIFFY_DEF_WRITER_STATE(NUMBER_FRAC), \
  JIFFY_DEF_WRITER_STATE(NUMBER_EXP_START), \
  JIFFY_DEF_WRITER_STATE(NUMBER_EXP_AFTER_SIGN), \
  JIFFY_DEF_WRITER_STATE(NUMBER_EXP), \
  JIFFY_DEF_WRITER_STATE(ARRAY), \
  JIFFY_DEF_WRITER_STATE(ARRAY_AFTER_VALUE), \
  JIFFY_DEF_WRITER_STATE(OBJECT), \
  JIFFY_DEF_WRITER_STATE(OBJECT_KEY), \
  JIFFY_DEF_WRITER_STATE(OBJECT_VALUE), \
  JIFFY_DEF_WRITER_STATE(OBJECT_AFTER_VALUE), \
  JIFFY_DEF_WRITER_STATE(LAST),

/**
 * Writer states.
 */
enum jiffy_writer_states {
#define JIFFY_DEF_WRITER_STATE(a) WRITER_STATE_##a
JIFFY_WRITER_STATE_LIST
#undef JIFFY_DEF_WRITER_STATE
};

/**
 * Writer state strings.  Used by jiffy_writer_state_to_s().
 */
static const char *
JIFFY_WRITER_STATES[] = {
#define JIFFY_DEF_WRITER_STATE(a) "WRITER_STATE_" # a
JIFFY_WRITER_STATE_LIST
#undef JIFFY_DEF_WRITER_STATE
};

const char *
jiffy_writer_state_to_s(
  const jiffy_writer_state_t state
) {
  const size_t ofs = (state < WRITER_STATE_LAST) ? state : WRITER_STATE_LAST;
  return JIFFY_WRITER_STATES[ofs];
}

void *
jiffy_writer_get_user_data(
  const jiffy_writer_t * const w
) {
  return w->user_data;
}

// get the current writer state
#define WRITER_GET_STATE(w) ((w)->stack_ptr[(w)->stack_pos])

// set the current writer state
#define WRITER_SWAP(w, state) (w)->stack_ptr[(w)->stack_pos] = (state)

// invoke on_error callback with error code, set the state to
// WRITER_STATE_FAIL, and then return false.
#define WRITER_FAIL(w, err) do { \
  if ((w)->cbs && (w)->cbs->on_error) { \
    (w)->cbs->on_error((w), err); \
  } \
  WRITER_SWAP((w), WRITER_STATE_FAIL); \
  return false; \
} while (0)

// call given callback, if it is non-NULL
#define WRITER_FIRE(w, cb_name) do { \
  if ((w)->cbs && (w)->cbs->cb_name) { \
    (w)->cbs->cb_name(w); \
  } \
} while (0)

// call on_write callback with buffer and length, if it is non-NULL
#define WRITER_WRITE(w, buf, len) do { \
  if ((w)->cbs && (w)->cbs->on_write) { \
    (w)->cbs->on_write(w, (buf), (len)); \
  } \
} while (0)

/**
 * Push writer state.  Returns false on stack overflow.
 *
 * Note: this is defined as an inline function rather than a macro to
 * give the compiler more flexibility in terms of inlining.
 */
static inline bool
jiffy_writer_push_state(
  jiffy_writer_t * const w,
  const jiffy_writer_state_t state
) {
  if (w->stack_pos < w->stack_len - 1) {
    w->stack_ptr[++w->stack_pos] = state;
    return true;
  } else {
    WRITER_FAIL(w, JIFFY_ERR_STACK_OVERFLOW);
  }
}

/**
 * Pop parser state.  Returns false on stack underflow.
 *
 * Note: this is defined as an inline function rather than a macro to
 * give the compiler more flexibility in terms of inlining.
 */
static inline bool
jiffy_writer_pop_state(
  jiffy_writer_t * const w
) {
  // check for underflow
  if (!w->stack_pos) {
    // got underflow, return error
    WRITER_FAIL(w, JIFFY_ERR_STACK_UNDERFLOW);
  }

  // decriment position
  w->stack_pos--;

  // check for done
  if (!w->stack_pos && WRITER_GET_STATE(w) == WRITER_STATE_INIT) {
    WRITER_SWAP(w, WRITER_STATE_DONE);
  }

  // return success
  return true;
}

// push writer state and return false if an error occurred.
#define WRITER_PUSH(w, state) do { \
  if (!jiffy_writer_push_state((w), (state))) { \
    return false; \
  } \
} while (0)

// pop writer state and return false if an error occurred.
#define WRITER_POP(w) do { \
  if (!jiffy_writer_pop_state(w)) { \
    return false; \
  } \
} while (0)

bool
jiffy_writer_init(
  jiffy_writer_t * const w,
  const jiffy_writer_cbs_t *cbs,
  jiffy_writer_state_t * const stack_ptr,
  const size_t stack_len,
  void *user_data
) {
  // verify that the following conditions are true:
  // * the writer context is non-null
  // * the stack memory pointer is non-null
  // * the number of stack memory elements is greater than 1
  if (!w || !stack_ptr || stack_len < 2) {
    // return failure
    return false;
  }

  w->cbs = cbs;
  w->stack_ptr = stack_ptr;
  w->stack_len = stack_len;
  w->stack_pos = 0;
  w->stack_ptr[0] = WRITER_STATE_INIT;
  w->user_data = user_data;

  // return success
  return true;
}

static inline bool
jiffy_writer_write_literal(
  jiffy_writer_t * const w,
  const char * const val,
  size_t len
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_INIT:
    WRITER_WRITE(w, val, len);
    WRITER_SWAP(w, WRITER_STATE_DONE);
    break;
  case WRITER_STATE_ARRAY:
    WRITER_WRITE(w, val, len);
    WRITER_SWAP(w, WRITER_STATE_ARRAY_AFTER_VALUE);
    break;
  case WRITER_STATE_ARRAY_AFTER_VALUE:
    WRITER_WRITE(w, ",", 1);
    WRITER_WRITE(w, val, len);
    break;
  case WRITER_STATE_OBJECT_VALUE:
    WRITER_WRITE(w, val, len);
    WRITER_SWAP(w, WRITER_STATE_OBJECT_AFTER_VALUE);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_null(
  jiffy_writer_t * const w
) {
  return jiffy_writer_write_literal(w, "null", 4);
}

bool
jiffy_writer_write_true(
  jiffy_writer_t * const w
) {
  return jiffy_writer_write_literal(w, "true", 4);
}

bool
jiffy_writer_write_false(
  jiffy_writer_t * const w
) {
  return jiffy_writer_write_literal(w, "false", 5);
}

bool
jiffy_writer_write_object_start(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_INIT:
  case WRITER_STATE_ARRAY:
  case WRITER_STATE_OBJECT_VALUE:
    WRITER_PUSH(w, WRITER_STATE_OBJECT);
    WRITER_WRITE(w, "{", 1);
    break;
  case WRITER_STATE_ARRAY_AFTER_VALUE:
    WRITER_WRITE(w, ",", 1);
    WRITER_PUSH(w, WRITER_STATE_OBJECT);
    WRITER_WRITE(w, "{", 1);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_object_end(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_OBJECT:
  case WRITER_STATE_OBJECT_AFTER_VALUE:
    WRITER_WRITE(w, "}", 1);
    WRITER_POP(w);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_array_start(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_INIT:
  case WRITER_STATE_ARRAY:
  case WRITER_STATE_OBJECT_VALUE:
    WRITER_PUSH(w, WRITER_STATE_ARRAY);
    WRITER_WRITE(w, "[", 1);
    break;
  case WRITER_STATE_ARRAY_AFTER_VALUE:
    WRITER_WRITE(w, ",", 1);
    WRITER_PUSH(w, WRITER_STATE_ARRAY);
    WRITER_WRITE(w, "[", 1);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_array_end(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_ARRAY:
  case WRITER_STATE_ARRAY_AFTER_VALUE:
    WRITER_WRITE(w, "]", 1);
    WRITER_POP(w);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_object_key_start(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_OBJECT:
    WRITER_PUSH(w, WRITER_STATE_OBJECT_KEY);
    WRITER_WRITE(w, "\"", 1);
    break;
  case WRITER_STATE_OBJECT_AFTER_VALUE:
    WRITER_WRITE(w, ",\"", 2);
    WRITER_SWAP(w, WRITER_STATE_OBJECT_KEY);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_object_key_end(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_OBJECT_KEY:
    WRITER_WRITE(w, "\":", 2);
    WRITER_SWAP(w, WRITER_STATE_OBJECT_VALUE);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_number_start(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_INIT:
  case WRITER_STATE_OBJECT_VALUE:
    WRITER_PUSH(w, WRITER_STATE_NUMBER);
    WRITER_PUSH(w, WRITER_STATE_NUMBER_START);
    break;
  case WRITER_STATE_ARRAY_AFTER_VALUE:
    WRITER_WRITE(w, ",", 1);
    WRITER_PUSH(w, WRITER_STATE_NUMBER);
    WRITER_PUSH(w, WRITER_STATE_NUMBER_START);
    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

static inline bool
jiffy_writer_write_number_byte(
  jiffy_writer_t * const w,
  const uint8_t byte
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_NUMBER_START:
    switch (byte) {
    case '+':
    case '-':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_AFTER_SIGN);
      break;
    case '0':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_AFTER_LEADING_ZERO);
      break;
    CASE_NONZERO_NUMBER
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_INT);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_AFTER_SIGN:
    switch (byte) {
    case '0':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_AFTER_LEADING_ZERO);
      break;
    CASE_NONZERO_NUMBER
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_INT);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_AFTER_LEADING_ZERO:
    switch (byte) {
    case '.':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_FRAC);
      break;
    case 'e':
    case 'E':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_EXP_START);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_INT:
    switch (byte) {
    CASE_NUMBER
      WRITER_WRITE(w, &byte, 1);
      break;
    case '.':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_AFTER_DOT);
      break;
    case 'e':
    case 'E':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_EXP_START);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_AFTER_DOT:
    switch (byte) {
    CASE_NUMBER
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_FRAC);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_FRAC:
    switch (byte) {
    CASE_NUMBER
      WRITER_WRITE(w, &byte, 1);
      break;
    case 'e':
    case 'E':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_EXP_START);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_EXP_START:
    switch (byte) {
    CASE_NUMBER
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_EXP);
      break;
    case '+':
    case '-':
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_EXP_AFTER_SIGN);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_EXP_AFTER_SIGN:
    switch (byte) {
    CASE_NONZERO_NUMBER
      WRITER_WRITE(w, &byte, 1);
      WRITER_SWAP(w, WRITER_STATE_NUMBER_EXP);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case WRITER_STATE_NUMBER_EXP:
    switch (byte) {
    CASE_NUMBER
      WRITER_WRITE(w, &byte, 1);
      break;
    default:
      WRITER_FAIL(w, JIFFY_ERR_BAD_BYTE);
    }

    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_number_data(
  jiffy_writer_t * const w,
  const void * const ptr,
  size_t len
) {
  const uint8_t * const buf = ptr;

  for (size_t i = 0; i < len; i++) {
    if (!jiffy_writer_write_number_byte(w, buf[i])) {
      return false;
    }
  }

  // return success
  return true;
}

bool
jiffy_writer_write_number_end(
  jiffy_writer_t * const w
) {
  switch (WRITER_GET_STATE(w)) {
  case WRITER_STATE_NUMBER_AFTER_LEADING_ZERO:
  case WRITER_STATE_NUMBER_INT:
  case WRITER_STATE_NUMBER_FRAC:
    WRITER_POP(w);
    WRITER_POP(w);

    break;
  default:
    WRITER_FAIL(w, JIFFY_ERR_BAD_STATE);
  }

  // return success
  return true;
}

bool
jiffy_writer_write_number_from_string(
  jiffy_writer_t * const w,
  const void * const ptr,
  const size_t len
) {
  const uint8_t * const buf = ptr;

  return (
    jiffy_writer_write_number_start(w) &&
    jiffy_writer_write_number_data(w, buf, len) &&
    jiffy_writer_write_number_end(w)
  );
}
