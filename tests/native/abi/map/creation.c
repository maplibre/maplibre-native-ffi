// Map creation through the public ABI: the map the call returns at once, what
// it accepts while it is still creating, and its release before it is ready.

#include "support/test_support.h"

// Parks the runtime worker inside a completion on a map of its own, so a map
// created next stays in its creating state until the case releases the gate.
static mln_map park_worker(mln_runtime runtime, mln_test_gate* gate) {
  mln_map parking = mln_test_create_map(runtime);
  mln_test_gate_init(gate);
  const mln_completion parked = mln_test_gate_completion(gate);
  MLN_TEST_OK(mln_map_set_debug_options(parking, 0, &parked, NULL));
  TEST_ASSERT_TRUE_MESSAGE(
    mln_test_gate_wait_entered(gate), "the runtime worker never parked"
  );
  return parking;
}

static void a_creating_map_runs_accepted_work_once_it_is_ready(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_gate gate;
  mln_map parking = park_worker(runtime, &gate);

  mln_test_completion created = mln_test_completion_default(0);
  const mln_map_options map_options = mln_test_map_options();
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create_map(
    runtime, &map_options, &map, &created.descriptor, NULL
  ));
  TEST_ASSERT_NOT_EQUAL_UINT64(MLN_HANDLE_NULL, map);

  // Calls that read the native map wait for the completion.
  mln_map_snapshot snapshot = {.size = sizeof(snapshot)};
  MLN_TEST_INVALID_STATE(mln_map_get_snapshot(map, &snapshot, NULL));
  const mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options)
  };
  mln_test_completion attach = mln_test_completion_default(0);
  mln_test_render_fixture fixture = {0};
  MLN_TEST_INVALID_STATE(mln_test_render_fixture_start_attach(
    map, &options, &attach.descriptor, &fixture
  ));
  mln_test_completion_reject(&attach);
  mln_test_completion_destroy(&attach);

  // Ordered work queues behind creation.
  mln_test_completion style = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_set_style_json(
    map, mln_test_empty_style_json, &style.descriptor, NULL
  ));
  TEST_ASSERT_FALSE(mln_test_completion_poll(&created));
  mln_test_gate_release(&gate);
  MLN_TEST_OK(mln_test_completion_settle(&created));
  MLN_TEST_OK(mln_test_completion_settle(&style));
  MLN_TEST_OK(mln_map_get_snapshot(map, &snapshot, NULL));

  mln_test_destroy_map(map);
  mln_test_destroy_map(parking);
  mln_test_destroy_runtime(runtime);
}

// Release accepts a creating map, which finishes creation and reports it
// before the map retires.
static void a_map_released_while_creating_reports_creation_first(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_test_gate gate;
  mln_map parking = park_worker(runtime, &gate);

  mln_test_completion created = mln_test_completion_default(0);
  const mln_map_options map_options = mln_test_map_options();
  mln_map map = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_runtime_create_map(
    runtime, &map_options, &map, &created.descriptor, NULL
  ));
  mln_test_completion released = mln_test_completion_default(0);
  MLN_TEST_OK(mln_map_release(map, &released.descriptor, NULL));
  mln_test_gate_release(&gate);

  MLN_TEST_OK(mln_test_completion_settle(&released));
  TEST_ASSERT_TRUE(mln_test_completion_poll(&created));
  MLN_TEST_OK(mln_test_completion_settle(&created));

  mln_test_destroy_map(parking);
  mln_test_destroy_runtime(runtime);
}

MLN_TEST_GROUP {
  RUN_TEST(a_creating_map_runs_accepted_work_once_it_is_ready);
  RUN_TEST(a_map_released_while_creating_reports_creation_first);
}
