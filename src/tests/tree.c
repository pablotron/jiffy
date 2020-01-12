#include <stdbool.h> // bool
#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include <err.h> // err()
#include "../jiffy.h"
#include "test-set.h"

static void
indent(
  const size_t depth
) {
  for (size_t i = 0; i < depth; i++) {
    fputs("  ", stderr);
  }
}

static void
dump_value(
  const jiffy_value_t * const value,
  const size_t depth
) {
  const jiffy_type_t type = jiffy_value_get_type(value);
  const char * const type_name = jiffy_type_to_s(type);

  indent(depth);
  fprintf(stderr, "%s", type_name);
  switch (type) {
  case JIFFY_TYPE_NULL:
  case JIFFY_TYPE_TRUE:
  case JIFFY_TYPE_FALSE:
    break;
  case JIFFY_TYPE_NUMBER:
    {
      size_t len;
      const uint8_t * const ptr = jiffy_number_get_bytes(value, &len);
      if (!ptr) {
        errx(EXIT_FAILURE, "E: jiffy_number_get_bytes()");
      }

      fputs(": ", stderr);
      if (len > 0 && !fwrite(ptr, len, 1, stderr)) {
        err(EXIT_FAILURE, "E: number fwrite(): ");
      }
    }

    break;
  case JIFFY_TYPE_STRING:
    {
      size_t len;
      const uint8_t * const ptr = jiffy_string_get_bytes(value, &len);
      if (!ptr) {
        errx(EXIT_FAILURE, "E: jiffy_string_get_bytes()");
      }

      fputs(": \"", stderr);
      if (len > 0 && !fwrite(ptr, len, 1, stderr)) {
        err(EXIT_FAILURE, "E: string fwrite(): ");
      }
      fputs("\"", stderr);
    }

    break;
  case JIFFY_TYPE_ARRAY:
    {
      const size_t len = jiffy_array_get_size(value);

      if (len > 0) {
        fputs(": [\n", stderr);
        for (size_t i = 0; i < len; i++) {
          dump_value(jiffy_array_get_nth(value, i), depth + 1);
        }
        indent(depth);
        fputc(']', stderr);
      } else {
        fputs(": []", stderr);
      }
    }

    break;
  case JIFFY_TYPE_OBJECT:
    {
      const size_t len = jiffy_object_get_size(value);
      if (len > 0) {
        fprintf(stderr, ": (%zu) {\n", len);
        for (size_t i = 0; i < len; i++) {
          const jiffy_value_t * const key = jiffy_object_get_nth_key(value, i);
          // warnx("key = %p, key->type = %02x, key->v_str.ptr = %p, key->v_str.len = %zu", (void*) key, key->type, key->v_str.ptr, key->v_str.len);
          dump_value(key, depth + 1);

          const jiffy_value_t * const val = jiffy_object_get_nth_value(value, i);
          dump_value(val, depth + 1);
        }
        indent(depth);
        fputc('}', stderr);
      } else {
        fputs(": {}", stderr);
      }
    }

    break;
  default:
    errx(EXIT_FAILURE, "Unknown object type: %02x", type);
  }

  fprintf(stderr, "\n");
}

static void
on_parse_error(
  const jiffy_tree_t * const tree,
  const jiffy_err_t err
) {
  (void) tree;
  warnx("parse error: %s", jiffy_err_to_s(err));
}

static const jiffy_tree_cbs_t
TREE_CBS = {
  .on_error      = on_parse_error,
};

void test_tree(int argc, char *argv[]) {
  char buf[1024];

  test_set_t set;
  if (!test_set_init(&set, argc, argv)) {
    return;
  }

  bool expect;
  size_t len;
  while (test_set_next(&set, buf, sizeof(buf), &expect, &len)) {
    warnx("src_buf = \"%s\"", buf);

    // create parse tree, check for error
    jiffy_tree_t tree;
    // if (!jiffy_tree_new_ex(&tree, NULL, stack_mem, STACK_LEN, buf, len - 1, NULL))
    if (jiffy_tree_new(&tree, &TREE_CBS, buf, len, NULL) != expect) {
      errx(EXIT_FAILURE, "jiffy_tree_new()");
    }

    if (!expect) {
      continue;
    }

    // get the root value of tree
    const jiffy_value_t *root = jiffy_tree_get_root_value(&tree);
    if (!root) {
      errx(EXIT_FAILURE, "jiffy_tree_get_root_value() failed");
    }

    // print root value type
    const jiffy_type_t root_type = jiffy_value_get_type(root);
    const char * const type_name = jiffy_type_to_s(root_type);
    fprintf(stderr, "D: type = %s\n", type_name);

    dump_value(root, 0);

    // free tree
    jiffy_tree_free(&tree);
  }
}
