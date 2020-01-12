#ifndef TEST_SET_H
#define TEST_SET_H

#include <stdlib.h> // size_t
#include <stdio.h> // FILE

typedef struct {
  FILE *fh;
  int argc, pos;
  char **argv;
} test_set_t;

_Bool
test_set_init(
  test_set_t * const,
  int, 
  char **
);

_Bool
test_set_next(
  test_set_t * const,
  char * const,
  const size_t,
  _Bool * const,
  size_t * const
);

#endif /* TEST_SET_H */
