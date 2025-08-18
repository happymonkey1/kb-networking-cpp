#ifndef KB_NETWORKING_TESTS_KB_TESTING_H
#define KB_NETWORKING_TESTS_KB_TESTING_H

#include "kb/kb_networking.h"

#define KB_ASSERT_EQ(x, y, ...) KB_ASSERT((x) == (y), __VA_ARGS__)
#define KB_ASSERT_TRUE(x, ...) KB_ASSERT((x) == true, __VA_ARGS__)
#define KB_ASSERT_FALSE(x, ...) KB_ASSERT((x) == false, __VA_ARGS__)

#endif // KB_NETWORKING_TESTS_KB_TESTING_H