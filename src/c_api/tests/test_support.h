#ifndef MLN_C_API_TEST_SUPPORT_H
#define MLN_C_API_TEST_SUPPORT_H

#include <stdbool.h>
#include <stddef.h>

#include "maplibre_native_c.h"

typedef struct mln_test_render_fixture {
  mln_render_session* session;
  void* backend_state;
} mln_test_render_fixture;

mln_runtime* mln_test_create_runtime(void);
mln_map* mln_test_create_map(mln_runtime* runtime);
void mln_test_destroy_runtime(mln_runtime* runtime);
void mln_test_destroy_map(mln_map* map);
void mln_test_sleep_millisecond(void);

bool mln_test_render_fixture_create(
  mln_map* map, mln_test_render_fixture* fixture
);
void mln_test_render_fixture_destroy(mln_test_render_fixture* fixture);

#endif
