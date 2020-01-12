#include <stdbool.h> // bool
#include <stdlib.h> // size_t
#include <err.h> // warn(), err()
#include <string.h> // strlen(), memcpy()
#include "test-set.h"

static bool
test_set_next_file(
  test_set_t * const set
) {
  // close current fh, check for error
  if (set->fh && fclose(set->fh)) {
    warn("fclose()");
    set->fh = NULL;
  }

  if (set->pos >= set->argc) {
    // return failure
    return false;
  }

  // get next path, increment argc
  const char * const path = set->argv[set->pos++];

  // open path, check for error
  set->fh = fopen(path, "rb");
  if (!set->fh) {
    err(EXIT_FAILURE, "fopen(\"%s\")", path);
  }

  // return success
  return true;
}

bool
test_set_init(
  test_set_t * const set,
  int argc, 
  char **argv
) {
  set->argc = argc;
  set->argv = argv;
  set->pos = 0;
  set->fh = NULL;

  // open first file, ignore error
  return test_set_next_file(set);
}

bool
test_set_next(
  test_set_t * const set,
  char * const dst_buf,
  const size_t dst_len,
  bool * const should_pass,
  size_t * const ret_len
) {
  char buf[1024];

  // check for required return length
  if (!ret_len) {
    errx(EXIT_FAILURE, "ret_len is NULL");
  }

  // check for required should_pass
  if (!should_pass) {
    errx(EXIT_FAILURE, "should_pass is NULL");
  }

  for (;;) {
    if (!fgets(buf, sizeof(buf), set->fh)) {
      // advance to next test file, check for error
      if (!test_set_next_file(set)) {
        return false;
      }

      continue;
    }

    // get length
    size_t len = strlen(buf);

    // strip trailing newline
    if (buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
      len--;
    }

    // check for empty lines and comments
    if (len < 3 || buf[0] == '#') {
      // skip blank lines and comments
      continue;
    }

    if (buf[0] != 'P' && buf[0] != 'F') {
      // warn on invalid lines and continue
      warnx("Skipping invalid test (missing P/F prefix): %s", buf);
      continue;
    } else if (buf[1] != ' ') {
      // warn on invalid lines and continue
      warnx("Skipping invalid test (missing space after P/F prefix): %s", buf);
      continue;
    }

    // check dst_len
    if (dst_len < len) {
      errx(EXIT_FAILURE, "Buffer to small: have %zu, need %zu", len, dst_len);
    }

    // get should_pass
    *should_pass = (buf[0] == 'P');

    // copy to buf to dst_buf, omitting trailing nul
    memcpy(dst_buf, buf + 2, len - 2);
    dst_buf[len - 2] = '\0';

    // return length
    *ret_len = len - 2;

    fprintf(stderr, "next test (%zu bytes): \"", len - 2);
    fwrite(dst_buf, *ret_len, 1, stderr);
    fputs("\"\n", stderr);

    // return success
    return true;
  }

  // never reached
  return false;
}
