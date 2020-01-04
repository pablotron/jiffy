#include <stdbool.h> // bool
#include "jiffy.h"

#define CASE_WHITESPACE \
  case ' ': \
  case '\t': \
  case '\v': \
  case '\n': \
  case '\r':

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

#define CASE_HEX_AF_LO \
  case 'a': \
  case 'b': \
  case 'c': \
  case 'd': \
  case 'e': \
  case 'f':

#define CASE_HEX_AF_HI \
  case 'A': \
  case 'B': \
  case 'C': \
  case 'D': \
  case 'E': \
  case 'F':

#define CASE_HEX \
  CASE_NUMBER \
  CASE_HEX_AF_LO \
  CASE_HEX_AF_HI

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

static const char *
JIFFY_ERRORS[] = {
#define E(a, b) b
JIFFY_ERROR_LIST
#undef E
};

const char *
jiffy_err_to_s(const jiffy_err_t err) {
  const size_t ofs = err < JIFFY_ERR_LAST ? err : JIFFY_ERR_LAST;
  return JIFFY_ERRORS[ofs];
}

#define JIFFY_STATE_LIST \
  S(INIT), \
  S(DONE), \
  S(FAIL), \
  S(VALUE), \
  S(LIT_N), \
  S(LIT_NU), \
  S(LIT_NUL), \
  S(LIT_T), \
  S(LIT_TR), \
  S(LIT_TRU), \
  S(LIT_F), \
  S(LIT_FA), \
  S(LIT_FAL), \
  S(LIT_FALS), \
  S(NUMBER_AFTER_SIGN), \
  S(NUMBER_AFTER_LEADING_ZERO), \
  S(NUMBER_INT), \
  S(NUMBER_AFTER_DOT), \
  S(NUMBER_FRAC), \
  S(NUMBER_AFTER_EXP), \
  S(NUMBER_AFTER_EXP_SIGN), \
  S(NUMBER_EXP_NUM), \
  S(STRING), \
  S(STRING_ESC), \
  S(STRING_UNICODE), \
  S(STRING_UNICODE_X), \
  S(STRING_UNICODE_XX), \
  S(STRING_UNICODE_XXX), \
  S(OBJECT_START), \
  S(ARRAY_START), \
  S(ARRAY_ELEMENT), \
  S(OBJECT_KEY), \
  S(AFTER_OBJECT_KEY), \
  S(BEFORE_OBJECT_KEY), \
  S(AFTER_OBJECT_VALUE), \
  S(LAST),

typedef enum {
#define S(a) STATE_##a
JIFFY_STATE_LIST
#undef S
} jiffy_parser_state_t;

static const char *
JIFFY_STATES[] = {
#define S(a) "STATE_" # a
JIFFY_STATE_LIST
#undef S
};

const char *
jiffy_state_to_s(const uint32_t state) {
  return JIFFY_STATES[(state < STATE_LAST) ? state : STATE_LAST];
}

#define GET_STATE(p) ((p)->stack.ptr[(p)->pos])
#define SWAP(p, state) (p)->stack.ptr[(p)->pos] = (state)
#define FAIL(p, err) do { \
  if ((p)->cbs && (p)->cbs->on_error) { \
    (p)->cbs->on_error((p), err); \
  } \
  SWAP((p), STATE_FAIL); \
  return false; \
} while (0)

#define FIRE(p, cb_name) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p); \
  } \
} while (0)

#define EMIT(p, cb_name, byte) do { \
  if ((p)->cbs && (p)->cbs->cb_name) { \
    (p)->cbs->cb_name(p, (byte)); \
  } \
} while (0)

void
jiffy_parser_init(
  jiffy_parser_t * const p,
  const jiffy_parser_cbs_t * const cbs,
  jiffy_stack_t * const stack,
  void * const user_data
) {
  p->cbs = cbs;
  p->stack = *stack;
  p->pos = 0;
  p->num_bytes = 0;
  SWAP(p, STATE_INIT);
  p->user_data = user_data;
}

void *
jiffy_parser_get_user_data(
  const jiffy_parser_t * const p
) {
  return p->user_data;
}

static inline bool
jiffy_parser_push_state(
  jiffy_parser_t * const p,
  const uint32_t state
) {
  if (p->pos < p->stack.len - 1) {
    p->stack.ptr[++p->pos] = state;
    return true;
  } else {
    FAIL(p, JIFFY_ERR_STACK_OVERFLOW);
  }
}

static inline bool
jiffy_parser_pop_state(
  jiffy_parser_t * const p
) {
  // check for underflow
  if (!p->pos) {
    // got underflow, return error
    FAIL(p, JIFFY_ERR_STACK_UNDERFLOW);
  }

  // decriment position
  p->pos--;

  // check for done
  if (!p->pos && GET_STATE(p) == STATE_INIT) {
    SWAP(p, STATE_DONE);
  }

  // return success
  return true;
}

#define PUSH(p, state) do { \
  if (!jiffy_parser_push_state((p), (state))) { \
    return false; \
  } \
} while (0)

#define POP(p) do { \
  if (!jiffy_parser_pop_state(p)) { \
    return false; \
  } \
} while (0)

static void
jiffy_parser_push_codepoint(
  jiffy_parser_t * const p,
  const uint32_t code
) {
  if (code < 0x80) {
    EMIT(p, on_string_byte, code);
  } else if (code < 0x0800) {
    EMIT(p, on_string_byte, (0x03 << 6) | ((code >> 6) & 0x1f));
    EMIT(p, on_string_byte, (0x01 << 7) | (code & 0x3f));
  } else if (code < 0x10000) {
    EMIT(p, on_string_byte, (0x0e << 4) | ((code >> 12) & 0x0f));
    EMIT(p, on_string_byte, (0x01 << 7) | ((code >> 6) & 0x3f));
    EMIT(p, on_string_byte, (0x01 << 7) | (code & 0x3f));
  } else if (code < 0x110000) {
    EMIT(p, on_string_byte, (0x0f << 4) | ((code >> 18) & 0x07));
    EMIT(p, on_string_byte, (0x01 << 7) | ((code >> 12) & 0x3f));
    EMIT(p, on_string_byte, (0x01 << 7) | ((code >> 6) & 0x3f));
    EMIT(p, on_string_byte, (0x01 << 7) | (code & 0x3f));
  } else {
    // FIXME: emit something here
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
      jiffy_parser_push_codepoint(p, p->hex);
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
    if (!jiffy_parser_push_byte(p, buf[i])) {
      return false;
    }
  }

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
  if (p->pos || GET_STATE(p) != STATE_DONE) {
    FAIL(p, JIFFY_ERR_NOT_DONE);
  }

  // return success
  return true;
}
