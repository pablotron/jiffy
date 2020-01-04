#include <stdbool.h> // bool
#include "jiffy.h"

/* 
 * #define JIFFY_ERROR_LIST \
 *   E(OK, "ok"), \
 *   E(BAD_BYTE, "bad byte"), \
 *   E(BAD_STATE, "bad state"), \
 *   E(BAD_ESCAPE, "bad escape"), \
 *   E(BAD_UNICODE_ESCAPE, "bad unicode escape"), \
 *   E(STACK_UNDERFLOW, "stack underflow"), \
 *   E(STACK_OVERFLOW, "stack overflow"), \
 *   E(EXPECTED_ARRAY_ELEMENT, "expected array element"), \
 *   E(EXPECTED_COMMA_OR_ARRAY_END, "expected comma or array end"), \
 *   E(EXPECTED_STRING_OR_OBJECT_END, "expected string or object end"), \
 *   E(EXPECTED_COMMA_OR_OBJECT_END, "expected comma or object end"), \
 *   E(EXPECTED_OBJECT_KEY, "expected object key"), \
 *   E(EXPECTED_COLON, "expected colon"), \
 *   E(NOT_DONE, "not done"), \
 *   E(LAST, "unknown error"),
 * 
 * typedef enum {
 * #define E(a, b) JIFFY_ERR_##a
 * JIFFY_ERROR_LIST
 * #undef E
 * } jiffy_err_t;
 * 
 */ 
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

/* 
 * typedef struct jiffy_parser_t_ jiffy_parser_t;
 * 
 * typedef void (*jiffy_event_cb_t)(const jiffy_parser_t *);
 * typedef void (*jiffy_byte_cb_t)(const jiffy_parser_t *, const uint8_t);
 * 
 * typedef struct {
 *   const jiffy_event_cb_t on_null,
 *                          on_true,
 *                          on_false,
 *                          on_array_start,
 *                          on_array_end,
 *                          on_array_element_start,
 *                          on_array_element_end,
 *                          on_object_start,
 *                          on_object_end,
 *                          on_object_key_start,
 *                          on_object_key_end,
 *                          on_object_value_start,
 *                          on_object_value_end,
 *                          on_string_start,
 *                          on_string_end,
 *                          on_number_start,
 *                          on_number_end;
 * 
 *   const jiffy_byte_cb_t on_string_byte,
 *                         on_number_byte;
 * 
 *   void (*on_error)(
 *     const jiffy_parser_t *,
 *     const jiffy_err_t
 *   );
 * } jiffy_parser_cbs_t;
 * 
 * typedef struct {
 *   uint32_t *ptr;
 *   size_t len;
 * } jiffy_stack_t;
 * 
 * struct jiffy_parser_t_ {
 *   const jiffy_parser_cbs_t *cbs;
 *   jiffy_stack_t stack;
 *   size_t pos, num_bytes;
 *   uint8_t hex;
 *   
 *   void *user_data;
 * };
 */ 

typedef enum {
  STATE_INIT,
  STATE_DONE,
  STATE_FAIL,
  STATE_VALUE,
  STATE_LIT_N,
  STATE_LIT_NU,
  STATE_LIT_NUL,
  STATE_LIT_T,
  STATE_LIT_TR,
  STATE_LIT_TRU,
  STATE_LIT_F,
  STATE_LIT_FA,
  STATE_LIT_FAL,
  STATE_LIT_FALS,
  STATE_NUMBER_AFTER_SIGN,
  STATE_NUMBER_AFTER_LEADING_ZERO,
  STATE_NUMBER_INT,
  STATE_NUMBER_AFTER_DOT,
  STATE_NUMBER_FRAC,
  STATE_NUMBER_AFTER_EXP,
  STATE_NUMBER_AFTER_EXP_SIGN,
  STATE_NUMBER_EXP_NUM,
  STATE_STRING,
  STATE_STRING_ESC,
  STATE_STRING_UNICODE,
  STATE_STRING_UNICODE_X,
  STATE_STRING_UNICODE_XX,
  STATE_STRING_UNICODE_XXX,
  STATE_OBJECT_START,
  STATE_ARRAY_START,
  STATE_ARRAY_ELEMENT,
  STATE_OBJECT_KEY,
  STATE_AFTER_OBJECT_KEY,
  STATE_BEFORE_OBJECT_KEY,
  STATE_AFTER_OBJECT_VALUE,
  STATE_LAST,
} jiffy_parser_state_t;

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

#define FIRE_BYTE(p, cb_name, byte) do { \
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
    return 10 + byte - 'a';
  CASE_HEX_AF_HI
    return 10 + byte - 'A';
  default:
    // FIXME
    return 0;
  }
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
  // TODO: emit code as utf-8
  (void) p;
  (void) code;
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
      // p->cbs->on_number_start(p);
      // p->cbs->on_number_byte(p, byte);
      FIRE(p, on_number_start);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    case '0':
      SWAP(p, STATE_NUMBER_AFTER_LEADING_ZERO);
      FIRE(p, on_number_start);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    CASE_NONZERO_NUMBER
      SWAP(p, STATE_NUMBER_INT);
      FIRE(p, on_number_start);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    case '{':
      SWAP(p, STATE_OBJECT_START);
      // p->cbs->on_object_start(p);
      FIRE(p, on_object_start);
      break;
    case '[':
      SWAP(p, STATE_ARRAY_START);
      // p->cbs->on_array_start(p);
      FIRE(p, on_array_start);
      break;
    case '"':
      SWAP(p, STATE_STRING);
      // p->cbs->on_string_start(p);
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
      // p->cbs->on_null(p);
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
      // p->cbs->on_true(p);
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
      // p->cbs->on_false(p);
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
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    CASE_NONZERO_NUMBER
      SWAP(p, STATE_NUMBER_INT);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_AFTER_LEADING_ZERO:
    if (byte == '.') {
      SWAP(p, STATE_NUMBER_AFTER_DOT);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
    } else if (byte == 'e' || byte == 'E') {
      SWAP(p, STATE_NUMBER_AFTER_EXP);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
    } else {
      // p->cbs->on_number_end(p);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_NUMBER_INT:
    switch (byte) {
    CASE_NUMBER
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    case '.':
      SWAP(p, STATE_NUMBER_AFTER_DOT);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    case 'e':
    case 'E':
      SWAP(p, STATE_NUMBER_AFTER_EXP);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      // p->cbs->on_number_end(p);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_NUMBER_AFTER_DOT:
    switch (byte) {
    CASE_NUMBER
      SWAP(p, STATE_NUMBER_FRAC);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_FRAC:
    switch (byte) {
    CASE_NUMBER
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    case 'e':
    case 'E':
      SWAP(p, STATE_NUMBER_AFTER_EXP);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      // p->cbs->on_number_end(p);
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
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    CASE_NUMBER
      SWAP(p, STATE_NUMBER_EXP_NUM);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }
    
    break;
  case STATE_NUMBER_AFTER_EXP_SIGN:
    switch (byte) {
    CASE_NUMBER
      SWAP(p, STATE_NUMBER_EXP_NUM);
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      FAIL(p, JIFFY_ERR_BAD_BYTE);
    }

    break;
  case STATE_NUMBER_EXP_NUM:
    switch (byte) {
    CASE_NUMBER
      // p->cbs->on_number_byte(p, byte);
      FIRE_BYTE(p, on_number_byte, byte);
      break;
    default:
      // p->cbs->on_number_end(p);
      FIRE(p, on_number_end);
      POP(p);
      goto retry;
    }

    break;
  case STATE_STRING:
    switch (byte) {
    case '"':
      // p->cbs->on_string_end(p);
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
      // p->cbs->on_string_byte(p, byte);
      FIRE_BYTE(p, on_string_byte, byte);
    }

    break;
  case STATE_STRING_ESC:
    switch (byte) {
    case '\\':
      // p->cbs->on_string_byte(p, '\\');
      FIRE_BYTE(p, on_string_byte, '\\');
      POP(p);
      break;
    case '/':
      // p->cbs->on_string_byte(p, '/');
      FIRE_BYTE(p, on_string_byte, '/');
      POP(p);
      break;
    case '\"':
      // p->cbs->on_string_byte(p, '\"');
      FIRE_BYTE(p, on_string_byte, '\"');
      POP(p);
      break;
    case 'n':
      // p->cbs->on_string_byte(p, '\n');
      FIRE_BYTE(p, on_string_byte, '\n');
      POP(p);
      break;
    case 'r':
      // p->cbs->on_string_byte(p, '\r');
      FIRE_BYTE(p, on_string_byte, '\r');
      POP(p);
      break;
    case 't':
      // p->cbs->on_string_byte(p, '\t');
      FIRE_BYTE(p, on_string_byte, '\t');
      POP(p);
      break;
    case 'v':
      // p->cbs->on_string_byte(p, '\v');
      FIRE_BYTE(p, on_string_byte, '\v');
      POP(p);
      break;
    case 'f':
      // p->cbs->on_string_byte(p, '\f');
      FIRE_BYTE(p, on_string_byte, '\f');
      POP(p);
      break;
    case 'b':
      // p->cbs->on_string_byte(p, '\b');
      FIRE_BYTE(p, on_string_byte, '\b');
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
      // p->cbs->on_array_end(p);
      FIRE(p, on_array_end);
      POP(p);
      break;
    case ',':
      FAIL(p, JIFFY_ERR_EXPECTED_ARRAY_ELEMENT);
    default:
      PUSH(p, STATE_ARRAY_ELEMENT);
      PUSH(p, STATE_VALUE);
      // p->cbs->on_array_element_start(p);
      FIRE(p, on_array_element_start);

      break;
    }

    break;
  case STATE_ARRAY_ELEMENT:
    switch (byte) {
    CASE_WHITESPACE
      // ignore
      break;
    case ']':
      // end element
      // p->cbs->on_array_element_end(p);
      FIRE(p, on_array_element_end);
      POP(p);

      // end array
      // p->cbs->on_array_end(p);
      FIRE(p, on_array_end);
      POP(p);

      break;
    case ',':
      // end element
      // p->cbs->on_array_element_end(p);
      FIRE(p, on_array_element_end);

      // start element
      PUSH(p, STATE_VALUE);
      // p->cbs->on_array_element_start(p);
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
      // p->cbs->on_object_end(p);
      FIRE(p, on_object_end);
      POP(p);

      break;
    case '"':
      PUSH(p, STATE_OBJECT_KEY);
      PUSH(p, STATE_STRING);
      // p->cbs->on_object_key_start(p);
      // p->cbs->on_string_start(p);
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
      // p->cbs->on_object_key_end(p);
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
      // p->cbs->on_object_value_start(p);
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
      // p->cbs->on_object_value_end(p);
      FIRE(p, on_object_value_end);
      SWAP(p, STATE_BEFORE_OBJECT_KEY);
      break;
    case '}':
      // p->cbs->on_object_value_end(p);
      FIRE(p, on_object_value_end);
      POP(p);
      // p->cbs->on_object_end(p);
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
      // p->cbs->on_object_key_start(p);
      // p->cbs->on_string_start(p);
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

_Bool
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

_Bool
jiffy_parser_fini(
  jiffy_parser_t * const p
) {
  if (p->pos || GET_STATE(p) != STATE_DONE) {
    FAIL(p, JIFFY_ERR_NOT_DONE);
    return false;
  }

  // return success
  return true;
}
