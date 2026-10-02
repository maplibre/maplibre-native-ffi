#ifndef MLN_NATIVE_TESTS_TEST_SUPPORT_H
#define MLN_NATIVE_TESTS_TEST_SUPPORT_H

// Everything a native test file uses from the support layer, with Unity, the
// harness, and the C library headers that most cases need.

#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "env.h"
#include "harness.h"
#include "render.h"
#include "status.h"
#include "tables.h"
#include "unity.h"
#include "wait.h"

#endif
