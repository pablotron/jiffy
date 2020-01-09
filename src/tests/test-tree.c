#include <stdio.h> // printf()
#include <string.h> // strlen()
#include <stdlib.h> // EXIT_*
#include <err.h> // err()
#include "../jiffy.h"

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
        fprintf(stderr, "E: jiffy_number_bet_bytes()\n");
        exit(EXIT_FAILURE);
      }

      fputs(": ", stderr);
      fwrite(ptr, len, 1, stderr);
    }

    break;
  case JIFFY_TYPE_STRING:
    {
      size_t len;
      const uint8_t * const ptr = jiffy_string_get_bytes(value, &len);
      if (!ptr) {
        fprintf(stderr, "E: jiffy_number_bet_bytes()\n");
        exit(EXIT_FAILURE);
      }

      fputs(": ", stderr);
      fwrite(ptr, len, 1, stderr);
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
        fputs(": {\n", stderr);
        for (size_t i = 0; i < len; i++) {
          dump_value(jiffy_object_get_nth_key(value, i), depth + 1);
          dump_value(jiffy_object_get_nth_value(value, i), depth + 1);
        }
        indent(depth);
        fputc('}', stderr);
      } else {
        fputs(": {}", stderr);
      }
    }

    break;
  default:
    exit(-1);
  }

  fprintf(stderr, "\n");
}

void test_tree(int argc, char *argv[]) {
  char buf[1024];

  for (int i = 0; i < argc; i++) {
    // open input file
    FILE *fh = fopen(argv[i], "rb");
    if (!fh) {
      err(EXIT_FAILURE, "fopen(): ");
    }

    while (fgets(buf, sizeof(buf), fh)) {
      // get line length
      const size_t len = strlen(buf);
      if (len < 2) {
        // skip empty lines
        continue;
      }

      // strip newline
      buf[len - 1] = '\0';

      // create parse tree, check for error
      jiffy_tree_t tree;
      // if (!jiffy_tree_new_ex(&tree, NULL, stack_mem, STACK_LEN, buf, len - 1, NULL))
      if (!jiffy_tree_new(&tree, NULL, buf, len - 1, NULL)) {
        errx(EXIT_FAILURE, "jiffy_tree_new()");
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

    // close input file, check for error
    if (fclose(fh)) {
      // warn on fclose() failure, continue
      warn("fclose(): ");
    }
  }
}
