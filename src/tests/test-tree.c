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
dump_scan_data(
  const jiffy_tree_scan_data_t * const data
) {
  warnx("scan_data->num_bytes = %zu", data->num_bytes);
  warnx("scan_data->num_vals = %zu", data->num_vals);
  warnx("scan_data->num_objs = %zu", data->num_objs);
  warnx("scan_data->num_obj_rows = %zu", data->num_obj_rows);
  // warnx("scan_data->num_arys = %zu", data->num_arys);
  warnx("scan_data->num_ary_rows = %zu", data->num_ary_rows);
  warnx("scan_data->curr_depth = %zu", data->curr_depth);
  warnx("scan_data->max_depth = %zu", data->max_depth);
}

static void
dump_parse_data(
  const jiffy_tree_parse_data_t * const data
) {
  dump_scan_data(data->scan_data);

  const jiffy_value_t * const vals = data->tree->vals;

  for (size_t i = 0; i < data->num_vals; i++) {
    warnx("data->vals[%zu] = %s", i, jiffy_type_to_s(vals[i].type));
  }

  warnx("data->num_ary_rows = %zu", data->num_ary_rows);
  for (size_t i = 0; i < data->num_ary_rows; i++) {
    const jiffy_tree_parse_ary_row_t * const row = data->ary_rows + i;
    warnx(
      "data->ary_rows[%zu] = { .ary = %zd, .val = %zd }",
      i, row[i].ary - vals, row[i].val - vals
    );
  }

  warnx("data->num_obj_rows = %zu", data->num_obj_rows);
  for (size_t i = 0; i < data->num_obj_rows; i++) {
    const jiffy_tree_parse_obj_row_t * const row = data->obj_rows + i;
    warnx(
      "data->obj_rows[%zu] = { .obj = %zd, .key = %zd, .val = %zd }",
      i, row[i].obj - vals, row[i].key - vals, row[i].val - vals
    );
  }

  warnx("data->num_bytes = %zu", data->num_bytes);
  fputs("bytes = ", stderr);
  fwrite(data->bytes, data->num_bytes, 1, stderr);
  fputs("\n", stderr);
}

static void
on_parse_data(
  const jiffy_tree_parse_data_t * const data,
  void * const user_data
) {
  (void) user_data;
  dump_parse_data(data);
}

static void
on_parse_error(
  const jiffy_tree_t * const tree,
  const jiffy_err_t err
) {
  (void) tree;
  errx(EXIT_FAILURE, "parse error: %s", jiffy_err_to_s(err));
}

static const jiffy_tree_cbs_t
TREE_CBS = {
  .on_parse_data = on_parse_data,
  .on_error      = on_parse_error,
};

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
      if (len < 2 || buf[0] == '#') {
        // skip empty lines
        continue;
      }

      // strip newline, terminate
      buf[len - 1] = '\0';
      warnx("src_buf = \"%s\"", buf);

      // create parse tree, check for error
      jiffy_tree_t tree;
      // if (!jiffy_tree_new_ex(&tree, NULL, stack_mem, STACK_LEN, buf, len - 1, NULL))
      if (!jiffy_tree_new(&tree, &TREE_CBS, buf, len - 1, NULL)) {
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
