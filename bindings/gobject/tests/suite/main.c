// What the GObject library adds to the C API: its errors, completions that
// finish on the main context that started them, handle objects, boxed values,
// option objects, callback registrations, and render sessions whose wakes run
// on the main context.

#include <maplibre-native-ffi.h>
#include <mln_test_graphics.h>
#include <string.h>

// How long a wait may take before the case fails, instead of hanging.
enum { timeout_seconds = 60 };

static const char* const empty_style =
  "{\"version\":8,\"sources\":{},\"layers\":[]}";
static const char* const red_style =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\","
  "\"type\":\"background\",\"paint\":{\"background-color\":\"#ff0000\"}}]}";

static gboolean timed_out(gpointer user_data) {
  g_error("timed out waiting for %s", (const char*)user_data);
  return G_SOURCE_REMOVE;
}

/** Iterates the thread-default main context until `*done` holds. */
static void wait_for(const gboolean* done, const char* what) {
  GMainContext* context = g_main_context_ref_thread_default();
  GSource* timeout = g_timeout_source_new_seconds(timeout_seconds);
  g_source_set_callback(timeout, timed_out, (gpointer)what, NULL);
  g_source_attach(timeout, context);
  while (!*done) g_main_context_iteration(context, TRUE);
  g_source_destroy(timeout);
  g_source_unref(timeout);
  g_main_context_unref(context);
}

/** The result of an asynchronous call, once its callback stores it. */
typedef struct {
  gboolean done;
  GAsyncResult* result;
} Pending;

static void store_result(GObject* source, GAsyncResult* result, gpointer data) {
  (void)source;
  Pending* pending = data;
  pending->result = g_object_ref(result);
  pending->done = TRUE;
}

static GAsyncResult* take(Pending* pending, const char* what) {
  wait_for(&pending->done, what);
  pending->done = FALSE;
  return g_steal_pointer(&pending->result);
}

static MaplibreNativeFfiRuntime* create_runtime(void) {
  g_autoptr(GError) error = NULL;
  MaplibreNativeFfiRuntime* runtime =
    maplibre_native_ffi_runtime_create(NULL, &error);
  g_assert_no_error(error);
  return runtime;
}

static void release_runtime(MaplibreNativeFfiRuntime* runtime) {
  Pending pending = {0};
  maplibre_native_ffi_runtime_release_async(runtime, store_result, &pending);
  g_autoptr(GAsyncResult) result = take(&pending, "a runtime release");
  g_autoptr(GError) error = NULL;
  g_assert_true(
    maplibre_native_ffi_runtime_release_finish(runtime, result, &error)
  );
  g_assert_no_error(error);
  g_object_unref(runtime);
}

static MaplibreNativeFfiMap* create_map(
  MaplibreNativeFfiRuntime* runtime, MaplibreNativeFfiRuntimeEventFlag events,
  guint32 width, guint32 height
) {
  g_autoptr(MaplibreNativeFfiMapOptions) options =
    maplibre_native_ffi_map_options_new();
  const MaplibreNativeFfiLogicalExtent extent = {
    .width = width, .height = height, .scale_factor = 1.0
  };
  maplibre_native_ffi_map_options_set_initial_extent(options, &extent);
  maplibre_native_ffi_map_options_set_map_mode(
    options, MAPLIBRE_NATIVE_FFI_MAP_MODE_CONTINUOUS
  );
  if (events != 0) {
    maplibre_native_ffi_map_options_set_event_mask(options, events);
  }
  g_autoptr(GError) error = NULL;
  MaplibreNativeFfiMap* map =
    maplibre_native_ffi_runtime_create_map(runtime, options, &error);
  g_assert_no_error(error);
  Pending pending = {0};
  maplibre_native_ffi_map_ready_async(map, store_result, &pending);
  g_autoptr(GAsyncResult) result = take(&pending, "a map");
  g_assert_true(maplibre_native_ffi_map_ready_finish(map, result, &error));
  g_assert_no_error(error);
  return map;
}

static void release_map(MaplibreNativeFfiMap* map) {
  Pending pending = {0};
  maplibre_native_ffi_map_release_async(map, store_result, &pending);
  g_autoptr(GAsyncResult) result = take(&pending, "a map release");
  g_autoptr(GError) error = NULL;
  g_assert_true(maplibre_native_ffi_map_release_finish(map, result, &error));
  g_assert_no_error(error);
  g_object_unref(map);
}

static void set_style(MaplibreNativeFfiMap* map, const char* json) {
  Pending pending = {0};
  maplibre_native_ffi_map_set_style_json_async(
    map, json, store_result, &pending
  );
  g_autoptr(GAsyncResult) result = take(&pending, "a style");
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiCommandCompletion) completion =
    maplibre_native_ffi_map_set_style_json_finish(map, result, &error);
  g_assert_no_error(error);
  g_assert_nonnull(completion);
}

static void a_failed_status_is_an_error_of_the_domain(void) {
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiStyleExpression) expression =
    maplibre_native_ffi_style_expression_create("[\"nonsense\"]", NULL, &error);
  g_assert_null(expression);
  g_assert_error(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT
  );
  // The message is the call's diagnostic.
  g_assert_cmpstr(error->message, !=, "");
}

typedef struct {
  MaplibreNativeFfiRuntime* runtime;
  GThread* callback_thread;
  gboolean done;
  gboolean succeeded;
} ContextCase;

static void listed(GObject* source, GAsyncResult* result, gpointer data) {
  ContextCase* state = data;
  g_autoptr(GError) error = NULL;
  g_autoptr(GPtrArray) regions =
    maplibre_native_ffi_runtime_list_offline_regions_finish(
      MAPLIBRE_NATIVE_FFI_RUNTIME(source), result, &error
    );
  state->succeeded = error == NULL && regions != NULL;
  state->callback_thread = g_thread_self();
  state->done = TRUE;
}

static gpointer list_on_own_context(gpointer data) {
  ContextCase* state = data;
  GMainContext* context = g_main_context_new();
  g_main_context_push_thread_default(context);
  maplibre_native_ffi_runtime_list_offline_regions_async(
    state->runtime, listed, state
  );
  wait_for(&state->done, "a completion on the thread's context");
  g_main_context_pop_thread_default(context);
  g_main_context_unref(context);
  return NULL;
}

static void a_completion_finishes_on_the_context_that_started_it(void) {
  ContextCase state = {.runtime = create_runtime()};
  GThread* thread = g_thread_new("caller", list_on_own_context, &state);
  g_thread_join(thread);
  g_assert_true(state.succeeded);
  g_assert_true(state.callback_thread == thread);
  release_runtime(state.runtime);
}

static void a_command_without_a_callback_still_applies(void) {
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  MaplibreNativeFfiMap* map = create_map(runtime, 0, 64, 64);
  // A discarded command passes no completion. The map applies commands in
  // order, so it has applied the first once the second completes.
  maplibre_native_ffi_map_set_debug_flags_async(
    map, MAPLIBRE_NATIVE_FFI_MAP_DEBUG_TILE_BORDERS, NULL, NULL
  );
  Pending pending = {0};
  maplibre_native_ffi_map_request_repaint_async(map, store_result, &pending);
  g_autoptr(GAsyncResult) result = take(&pending, "a repaint");
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiCommandCompletion) completion =
    maplibre_native_ffi_map_request_repaint_finish(map, result, &error);
  g_assert_no_error(error);
  g_autoptr(MaplibreNativeFfiMapSnapshot) snapshot =
    maplibre_native_ffi_map_get_snapshot(map, &error);
  g_assert_no_error(error);
  g_assert_cmpuint(snapshot->generation, >=, completion->generation);
  g_assert_true(
    snapshot->debug_flags & MAPLIBRE_NATIVE_FFI_MAP_DEBUG_TILE_BORDERS
  );
  release_map(map);
  release_runtime(runtime);
}

static void a_map_is_ready_to_every_wait(void) {
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  // The helper waits once the map is ready, so this wait starts after the
  // readiness settled.
  MaplibreNativeFfiMap* map = create_map(runtime, 0, 64, 64);
  Pending pending = {0};
  maplibre_native_ffi_map_ready_async(map, store_result, &pending);
  g_autoptr(GAsyncResult) result = take(&pending, "a second wait");
  g_autoptr(GError) error = NULL;
  g_assert_true(maplibre_native_ffi_map_ready_finish(map, result, &error));
  g_assert_no_error(error);
  release_map(map);
  release_runtime(runtime);
}

static void a_map_keeps_its_runtime_alive(void) {
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  MaplibreNativeFfiMap* map = create_map(runtime, 0, 64, 64);
  g_object_unref(runtime);
  // The map's object holds its runtime, whose native runtime still serves it.
  set_style(map, empty_style);
  g_object_unref(map);
}

static void a_holder_copies_its_members(void) {
  g_autoptr(MaplibreNativeFfiCameraOptions) camera =
    g_object_new(MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_OPTIONS, "zoom", 3.0, NULL);
  g_autoptr(MaplibreNativeFfiCameraUpdate) update =
    maplibre_native_ffi_camera_update_new();
  maplibre_native_ffi_camera_update_set_camera(update, camera);
  g_object_set(camera, "zoom", 5.0, NULL);
  // The update copied the camera.
  g_assert_cmpfloat(
    maplibre_native_ffi_camera_options_get_zoom(
      maplibre_native_ffi_camera_update_get_camera(update)
    ),
    ==, 3
  );

  g_autoptr(MaplibreNativeFfiFeatureId) id =
    maplibre_native_ffi_feature_id_new_string_value("feature");
  g_assert_true(maplibre_native_ffi_feature_id_is_string_value(id));
  g_assert_false(maplibre_native_ffi_feature_id_is_uint_value(id));
  g_autoptr(MaplibreNativeFfiFeatureId) id_copy =
    maplibre_native_ffi_feature_id_copy(id);
  g_assert_cmpstr(
    maplibre_native_ffi_feature_id_get_string_value(id_copy), ==, "feature"
  );
}

/** Moves the map's camera to `camera`'s members at once. */
static void jump(
  MaplibreNativeFfiMap* map, MaplibreNativeFfiCameraOptions* camera
) {
  g_autoptr(MaplibreNativeFfiCameraUpdate) update =
    maplibre_native_ffi_camera_update_new();
  maplibre_native_ffi_camera_update_set_mode(
    update, MAPLIBRE_NATIVE_FFI_CAMERA_UPDATE_MODE_JUMP
  );
  maplibre_native_ffi_camera_update_set_camera(update, camera);
  maplibre_native_ffi_map_update_camera_async(map, update, NULL, NULL);
}

static void an_option_is_unset_until_a_property_sets_it(void) {
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  MaplibreNativeFfiMap* map = create_map(runtime, 0, 64, 64);
  g_autoptr(MaplibreNativeFfiCameraOptions) turn = g_object_new(
    MAPLIBRE_NATIVE_FFI_TYPE_CAMERA_OPTIONS, "bearing", 30.0, NULL
  );
  g_assert_true(maplibre_native_ffi_camera_options_has_bearing(turn));
  g_assert_false(maplibre_native_ffi_camera_options_has_zoom(turn));
  jump(map, turn);

  g_autoptr(MaplibreNativeFfiCameraOptions) move =
    maplibre_native_ffi_camera_options_new();
  g_assert_false(maplibre_native_ffi_camera_options_has_zoom(move));
  const MaplibreNativeFfiLatLng center = {.latitude = 10, .longitude = 20};
  g_object_set(move, "zoom", 4.0, "center", &center, NULL);
  g_assert_true(maplibre_native_ffi_camera_options_has_zoom(move));
  g_assert_true(maplibre_native_ffi_camera_options_has_center(move));
  g_assert_false(maplibre_native_ffi_camera_options_has_bearing(move));
  jump(map, move);

  // The map applies commands in order, so the query sees both jumps, and the
  // second, which leaves the bearing unset, keeps the first's bearing.
  Pending pending = {0};
  maplibre_native_ffi_map_get_camera_async(map, store_result, &pending);
  g_autoptr(GAsyncResult) result = take(&pending, "the camera");
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiCameraQueryResult) found =
    maplibre_native_ffi_map_get_camera_finish(map, result, &error);
  g_assert_no_error(error);
  g_assert_cmpfloat_with_epsilon(found->camera.zoom, 4, 1e-9);
  g_assert_cmpfloat_with_epsilon(found->camera.center.latitude, 10, 1e-9);
  g_assert_cmpfloat_with_epsilon(found->camera.center.longitude, 20, 1e-9);
  g_assert_cmpfloat_with_epsilon(found->camera.bearing, 30, 1e-9);
  release_map(map);
  release_runtime(runtime);
}

static void a_bulk_projection_returns_every_result(void) {
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  MaplibreNativeFfiMap* map = create_map(runtime, 0, 64, 64);
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiMapSnapshot) snapshot =
    maplibre_native_ffi_map_get_snapshot(map, &error);
  g_assert_no_error(error);
  g_autoptr(MaplibreNativeFfiMapProjection) projection =
    maplibre_native_ffi_map_projection_create(&snapshot->view, &error);
  g_assert_no_error(error);

  const MaplibreNativeFfiLatLng coordinates[] = {
    {.latitude = 10, .longitude = 20}, {.latitude = -30, .longitude = 40}
  };
  g_autoptr(GPtrArray) points =
    maplibre_native_ffi_map_projection_pixels_for_lat_lngs(
      projection, coordinates, G_N_ELEMENTS(coordinates), &error
    );
  g_assert_no_error(error);
  g_assert_cmpuint(points->len, ==, G_N_ELEMENTS(coordinates));
  MaplibreNativeFfiScreenPoint pixels[G_N_ELEMENTS(coordinates)];
  for (guint index = 0; index < points->len; index++) {
    pixels[index] = *(MaplibreNativeFfiScreenPoint*)points->pdata[index];
  }
  g_autoptr(GPtrArray) found =
    maplibre_native_ffi_map_projection_lat_lngs_for_pixels(
      projection, pixels, G_N_ELEMENTS(pixels), &error
    );
  g_assert_no_error(error);
  g_assert_cmpuint(found->len, ==, G_N_ELEMENTS(coordinates));
  for (guint index = 0; index < found->len; index++) {
    const MaplibreNativeFfiLatLng* coordinate = found->pdata[index];
    g_assert_cmpfloat_with_epsilon(
      coordinate->latitude, coordinates[index].latitude, 1e-6
    );
    g_assert_cmpfloat_with_epsilon(
      coordinate->longitude, coordinates[index].longitude, 1e-6
    );
  }
  g_clear_object(&projection);
  release_map(map);
  release_runtime(runtime);
}

static void caller_sized_text_returns_all_of_a_long_result(void) {
  GString* text = g_string_new(NULL);
  for (int index = 0; index < 600; index++) g_string_append_c(text, 'a');
  g_autofree char* json = g_strdup_printf("[\"literal\", \"%s\"]", text->str);
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiStyleExpression) expression =
    maplibre_native_ffi_style_expression_create(json, NULL, &error);
  g_assert_no_error(error);
  g_autofree char* result =
    maplibre_native_ffi_style_expression_evaluate(expression, NULL, &error);
  g_assert_no_error(error);
  g_autofree char* expected = g_strdup_printf("\"%s\"", text->str);
  g_assert_cmpstr(result, ==, expected);
  g_string_free(text, TRUE);
}

typedef struct {
  guint64 map;
  GThread* thread;
  gboolean style_loaded;
  gboolean other_thread;
  gboolean freed;
  int frees;
} EventCase;

static void on_events(GPtrArray* events, gpointer user_data) {
  EventCase* state = user_data;
  if (g_thread_self() != state->thread) state->other_thread = TRUE;
  for (guint index = 0; index < events->len; index++) {
    const MaplibreNativeFfiRuntimeEvent* event = events->pdata[index];
    if (
      maplibre_native_ffi_runtime_event_get_source(event) == state->map &&
      maplibre_native_ffi_runtime_event_get_event_type(event) ==
        MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_MAP_STYLE_LOADED
    ) {
      state->style_loaded = TRUE;
    }
  }
}

static void count_free(gpointer user_data) {
  EventCase* state = user_data;
  state->frees++;
  state->freed = TRUE;
}

static void events_reach_the_handler_on_the_main_context(void) {
  EventCase state = {.thread = g_thread_self()};
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  g_autoptr(MaplibreNativeFfiRuntimeEventHandler) handler =
    maplibre_native_ffi_runtime_event_handler_new();
  maplibre_native_ffi_runtime_event_handler_set_callback(
    handler, on_events, &state, count_free
  );
  maplibre_native_ffi_runtime_set_event_handler_async(
    runtime, handler, NULL, NULL
  );
  g_clear_pointer(&handler, maplibre_native_ffi_runtime_event_handler_free);
  MaplibreNativeFfiMap* map = create_map(
    runtime, MAPLIBRE_NATIVE_FFI_RUNTIME_EVENT_FLAG_MAP_STYLE_LOADED, 64, 64
  );
  state.map = maplibre_native_ffi_map_get_id(map);
  set_style(map, empty_style);
  wait_for(&state.style_loaded, "a style loaded event");
  g_assert_false(state.other_thread);

  Pending pending = {0};
  maplibre_native_ffi_runtime_clear_event_handler_async(
    runtime, store_result, &pending
  );
  g_autoptr(GAsyncResult) result = take(&pending, "a cleared handler");
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiCommandCompletion) completion =
    maplibre_native_ffi_runtime_clear_event_handler_finish(
      runtime, result, &error
    );
  g_assert_no_error(error);
  // Native code releases the replaced handler, which frees its data once.
  wait_for(&state.freed, "the handler's data to be freed");
  g_assert_cmpint(state.frees, ==, 1);
  release_map(map);
  release_runtime(runtime);
  g_assert_cmpint(state.frees, ==, 1);
}

static void count(gpointer user_data) { (*(int*)user_data)++; }

static void never_called(GPtrArray* events, gpointer user_data) {
  (void)events;
  (void)user_data;
  g_assert_not_reached();
}

static void a_rejected_call_frees_its_registrations(void) {
  int frees = 0;
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  Pending pending = {0};
  maplibre_native_ffi_runtime_release_async(runtime, store_result, &pending);
  g_autoptr(GAsyncResult) released = take(&pending, "a release");
  g_assert_true(
    maplibre_native_ffi_runtime_release_finish(runtime, released, NULL)
  );

  MaplibreNativeFfiRuntimeEventHandler* handler =
    maplibre_native_ffi_runtime_event_handler_new();
  maplibre_native_ffi_runtime_event_handler_set_callback(
    handler, never_called, &frees, count
  );
  maplibre_native_ffi_runtime_set_event_handler_async(
    runtime, handler, store_result, &pending
  );
  g_autoptr(GAsyncResult) result = take(&pending, "a rejected handler");
  g_autoptr(GError) error = NULL;
  g_assert_null(maplibre_native_ffi_runtime_set_event_handler_finish(
    runtime, result, &error
  ));
  g_assert_error(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT
  );
  // Only the handler's own reference keeps the data now.
  g_assert_cmpint(frees, ==, 0);
  maplibre_native_ffi_runtime_event_handler_free(handler);
  g_assert_cmpint(frees, ==, 1);
  g_object_unref(runtime);
}

#if defined(__linux__)
static void a_dma_buf_ring_converts_its_formats(void) {
  MaplibreNativeFfiRuntime* runtime = create_runtime();
  MaplibreNativeFfiMap* map = create_map(runtime, 0, 64, 48);
  // NV12, which the library never renders, with a linear modifier.
  const MaplibreNativeFfiDmaBufFormat format = {
    .fourcc = 0x3231564e, .modifier = 0
  };
  g_autoptr(MaplibreNativeFfiDmaBufRingDescriptor) descriptor =
    maplibre_native_ffi_dma_buf_ring_descriptor_new();
  const MaplibreNativeFfiLogicalSize size = {.width = 64, .height = 48};
  maplibre_native_ffi_dma_buf_ring_descriptor_set_logical_size(
    descriptor, &size
  );
  maplibre_native_ffi_dma_buf_ring_descriptor_set_buffer_count(descriptor, 3);
  maplibre_native_ffi_dma_buf_ring_descriptor_set_formats(
    descriptor, &format, 1
  );
  g_autoptr(MaplibreNativeFfiRenderSessionAttachOptions) options =
    maplibre_native_ffi_render_session_attach_options_new();
  maplibre_native_ffi_render_session_attach_options_set_driver(
    options, MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER
  );
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiRenderSession) session =
    maplibre_native_ffi_map_attach_dma_buf_ring(
      map, descriptor, NULL, options, &error
    );
  g_assert_null(session);
  g_assert_error(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MAPLIBRE_NATIVE_FFI_ERROR_UNSUPPORTED
  );
  release_map(map);
  release_runtime(runtime);
}
#endif

// A render session whose frame wake marks that a frame result is queued.
typedef struct {
  MaplibreNativeFfiRuntime* runtime;
  MaplibreNativeFfiMap* map;
  MaplibreNativeFfiRenderSession* session;
  mln_test_graphics* graphics;
  mln_test_graphics_texture* texture;
  gboolean woken;
  guint64 token;
  GThread* thread;
  gboolean wake_elsewhere;
  int serviced;
} Target;

static void on_frame(gpointer user_data) {
  Target* target = user_data;
  if (g_thread_self() != target->thread) target->wake_elsewhere = TRUE;
  target->woken = TRUE;
}

static void on_driver_work(gpointer user_data) {
  Target* target = user_data;
  // The session can wake before the attach returns it.
  if (target->session == NULL) return;
  g_autoptr(GError) error = NULL;
  maplibre_native_ffi_render_session_service_driver_work(
    target->session, &error
  );
  g_assert_no_error(error);
  target->serviced++;
}

static MaplibreNativeFfiRenderSessionAttachOptions* attach_options(
  Target* target, MaplibreNativeFfiRenderDriverKind driver
) {
  MaplibreNativeFfiRenderSessionAttachOptions* options =
    maplibre_native_ffi_render_session_attach_options_new();
  maplibre_native_ffi_render_session_attach_options_set_driver(options, driver);
  g_autoptr(MaplibreNativeFfiWake) frame = maplibre_native_ffi_wake_new();
  maplibre_native_ffi_wake_set_callback(frame, on_frame, target, NULL);
  maplibre_native_ffi_render_session_attach_options_set_frame_wake(
    options, frame
  );
  if (driver == MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) {
    g_autoptr(MaplibreNativeFfiWake) work = maplibre_native_ffi_wake_new();
    maplibre_native_ffi_wake_set_callback(work, on_driver_work, target, NULL);
    maplibre_native_ffi_render_session_attach_options_set_driver_work_wake(
      options, work
    );
  }
  return options;
}

static void wait_until_ready(Target* target) {
  Pending pending = {0};
  maplibre_native_ffi_render_session_ready_async(
    target->session, store_result, &pending
  );
  g_autoptr(GAsyncResult) result = take(&pending, "a session");
  g_autoptr(GError) error = NULL;
  g_assert_true(maplibre_native_ffi_render_session_ready_finish(
    target->session, result, &error
  ));
  g_assert_no_error(error);
}

/** Demands a frame and returns its result once the frame wake reports it. */
static MaplibreNativeFfiRenderFrameResult* render(Target* target) {
  const MaplibreNativeFfiFrameDemand demand = {
    .flags = MAPLIBRE_NATIVE_FFI_FRAME_DEMAND_WAIT_FOR_UPDATE,
    .token = ++target->token,
  };
  g_autoptr(GError) error = NULL;
  g_assert_true(maplibre_native_ffi_render_session_demand_frame(
    target->session, &demand, &error
  ));
  g_assert_no_error(error);
  for (;;) {
    wait_for(&target->woken, "a frame wake");
    target->woken = FALSE;
    g_autoptr(GPtrArray) results =
      maplibre_native_ffi_render_session_drain_frame_results(
        target->session, &error
      );
    g_assert_no_error(error);
    for (guint index = 0; index < results->len; index++) {
      MaplibreNativeFfiRenderFrameResult* result = results->pdata[index];
      if (result->token == demand.token) {
        return maplibre_native_ffi_render_frame_result_copy(result);
      }
    }
  }
}

static void release_target(Target* target) {
  Pending pending = {0};
  maplibre_native_ffi_render_session_release_async(
    target->session, store_result, &pending
  );
  g_autoptr(GAsyncResult) result = take(&pending, "a session release");
  g_autoptr(GError) error = NULL;
  g_assert_true(maplibre_native_ffi_render_session_release_finish(
    target->session, result, &error
  ));
  g_assert_no_error(error);
  g_clear_object(&target->session);
  release_map(target->map);
  release_runtime(target->runtime);
  if (target->texture != NULL) {
    mln_test_graphics_texture_destroy(target->texture);
  }
  if (target->graphics != NULL) mln_test_graphics_destroy(target->graphics);
}

static void a_session_renders_pixels_that_the_library_reads_back(void) {
  Target target = {.thread = g_thread_self()};
  target.runtime = create_runtime();
  target.map = create_map(target.runtime, 0, 32, 16);
  const MaplibreNativeFfiOffscreenDescriptor descriptor = {
    .logical_size = {.width = 32, .height = 16}
  };
  g_autoptr(MaplibreNativeFfiRenderSessionAttachOptions) options =
    attach_options(&target, MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER);
  g_autoptr(GError) error = NULL;
  target.session = maplibre_native_ffi_map_attach_offscreen(
    target.map, &descriptor, options, NULL, &error
  );
  g_assert_no_error(error);
  wait_until_ready(&target);
  set_style(target.map, red_style);

  // A frame may draw before the style's paint applies, so keep rendering
  // until one is red.
  for (;;) {
    g_autoptr(MaplibreNativeFfiRenderFrameResult) frame = render(&target);
    g_assert_cmpint(
      frame->disposition, ==, MAPLIBRE_NATIVE_FFI_RENDER_RESULT_RENDERED
    );
    Pending pending = {0};
    maplibre_native_ffi_render_session_read_texture_async(
      target.session, store_result, &pending
    );
    g_autoptr(GAsyncResult) result = take(&pending, "a readback");
    g_autoptr(MaplibreNativeFfiTextureReadbackResult) image =
      maplibre_native_ffi_render_session_read_texture_finish(
        target.session, result, &error
      );
    g_assert_no_error(error);
    const MaplibreNativeFfiTextureImageInfo* info =
      maplibre_native_ffi_texture_readback_result_get_info(image);
    g_assert_cmpuint(info->width, ==, 32);
    g_assert_cmpuint(info->height, ==, 16);
    gsize length = 0;
    const guint8* pixels = g_bytes_get_data(
      maplibre_native_ffi_texture_readback_result_get_data(image), &length
    );
    g_assert_cmpuint(length, ==, info->byte_length);
    const guint8 red[] = {0xff, 0x00, 0x00, 0xff};
    if (memcmp(pixels, red, sizeof red) == 0) break;
  }
  g_assert_false(target.wake_elsewhere);
  release_target(&target);
}

typedef struct {
  gboolean fetched;
  gboolean release_finished;
  gboolean refused;
  gboolean answered;
} SourceCase;

static void refused_release(
  GObject* source, GAsyncResult* result, gpointer data
) {
  SourceCase* state = data;
  g_autoptr(GError) error = NULL;
  g_assert_false(maplibre_native_ffi_map_release_finish(
    MAPLIBRE_NATIVE_FFI_MAP(source), result, &error
  ));
  state->refused = g_error_matches(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE
  );
  state->release_finished = TRUE;
}

static void tile_answered(
  GObject* source, GAsyncResult* result, gpointer data
) {
  SourceCase* state = data;
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiCommandCompletion) completion =
    maplibre_native_ffi_map_set_custom_geometry_source_tile_data_finish(
      MAPLIBRE_NATIVE_FFI_MAP(source), result, &error
    );
  g_assert_no_error(error);
  state->answered = TRUE;
}

static void fetch_tile(
  MaplibreNativeFfiMap* map, const char* source_id,
  MaplibreNativeFfiCanonicalTileId* tile_id, gpointer user_data
) {
  SourceCase* state = user_data;
  g_assert_cmpstr(source_id, ==, "custom");
  if (state->fetched) return;
  state->fetched = TRUE;
  // The callback lends the map, which it can use but not release.
  maplibre_native_ffi_map_dispose(map);
  maplibre_native_ffi_map_release_async(map, refused_release, state);
  maplibre_native_ffi_map_set_custom_geometry_source_tile_data_async(
    map, source_id, tile_id, "{\"type\":\"FeatureCollection\",\"features\":[]}",
    tile_answered, state
  );
}

static void a_custom_source_answers_through_the_map_it_borrows(void) {
  SourceCase state = {0};
  Target target = {.thread = g_thread_self()};
  target.runtime = create_runtime();
  target.map = create_map(target.runtime, 0, 32, 16);
  const MaplibreNativeFfiOffscreenDescriptor descriptor = {
    .logical_size = {.width = 32, .height = 16}
  };
  g_autoptr(MaplibreNativeFfiRenderSessionAttachOptions) options =
    attach_options(&target, MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER);
  g_autoptr(GError) error = NULL;
  target.session = maplibre_native_ffi_map_attach_offscreen(
    target.map, &descriptor, options, NULL, &error
  );
  g_assert_no_error(error);
  wait_until_ready(&target);
  set_style(target.map, empty_style);

  g_autoptr(MaplibreNativeFfiCustomGeometrySourceOptions) source =
    maplibre_native_ffi_custom_geometry_source_options_new();
  maplibre_native_ffi_custom_geometry_source_options_set_fetch_tile(
    source, fetch_tile, &state, NULL
  );
  maplibre_native_ffi_map_add_custom_geometry_source_async(
    target.map, "custom", source, NULL, NULL
  );
  maplibre_native_ffi_map_add_style_layer_json_async(
    target.map, "{\"id\":\"custom\",\"type\":\"circle\",\"source\":\"custom\"}",
    NULL, NULL, NULL
  );
  while (!state.fetched) {
    g_autoptr(MaplibreNativeFfiRenderFrameResult) frame = render(&target);
  }
  wait_for(&state.answered, "a tile answer");
  wait_for(&state.release_finished, "a refused release");
  g_assert_true(state.refused);
  release_target(&target);
}

static gboolean backend_supported(MaplibreNativeFfiRenderBackendFlag backend) {
  return (maplibre_native_ffi_supported_render_backend_mask() & backend) != 0;
}

static void attach_borrowed(Target* target, uint32_t backend) {
  target->graphics = mln_test_graphics_create(backend);
  g_assert_nonnull(target->graphics);
  if (backend == MLN_TEST_GRAPHICS_BACKEND_EGL) {
    // The session's driver work runs on this thread, which the wake
    // services, so the context is current here.
    g_assert_true(mln_test_graphics_make_current(target->graphics));
  }
  target->texture = mln_test_graphics_texture_create(target->graphics, 32, 16);
  g_assert_nonnull(target->texture);
  mln_test_graphics_texture_info info = {0};
  g_assert_true(mln_test_graphics_texture_get_info(target->texture, &info));
  mln_test_graphics_context context = {0};
  g_assert_true(mln_test_graphics_get_context(target->graphics, &context));
  const MaplibreNativeFfiLogicalSize size = {.width = 32, .height = 16};
  g_autoptr(GError) error = NULL;
  if (backend == MLN_TEST_GRAPHICS_BACKEND_VULKAN) {
    g_autoptr(MaplibreNativeFfiRenderSessionAttachOptions) options =
      attach_options(target, MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CORE_WORKER);
    g_autoptr(MaplibreNativeFfiVulkanContextDescriptor) vulkan =
      maplibre_native_ffi_vulkan_context_descriptor_new();
    maplibre_native_ffi_vulkan_context_descriptor_set_instance(
      vulkan, context.vulkan_instance
    );
    maplibre_native_ffi_vulkan_context_descriptor_set_physical_device(
      vulkan, context.vulkan_physical_device
    );
    maplibre_native_ffi_vulkan_context_descriptor_set_device(
      vulkan, context.vulkan_device
    );
    maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue(
      vulkan, context.vulkan_queue
    );
    maplibre_native_ffi_vulkan_context_descriptor_set_graphics_queue_family_index(
      vulkan, context.vulkan_queue_family_index
    );
    maplibre_native_ffi_vulkan_context_descriptor_set_get_instance_proc_addr(
      vulkan, context.vulkan_get_instance_proc_addr
    );
    maplibre_native_ffi_vulkan_context_descriptor_set_get_device_proc_addr(
      vulkan, context.vulkan_get_device_proc_addr
    );
    MaplibreNativeFfiVulkanBorrowedTexture texture = {
      .image = info.vulkan_image, .image_view = info.vulkan_image_view
    };
    g_autoptr(MaplibreNativeFfiVulkanBorrowedTextureDescriptor) descriptor =
      maplibre_native_ffi_vulkan_borrowed_texture_descriptor_new();
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_logical_size(
      descriptor, &size
    );
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_width(
      descriptor, info.width
    );
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_physical_height(
      descriptor, info.height
    );
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_context(
      descriptor, vulkan
    );
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_textures(
      descriptor, &texture, 1
    );
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_format(
      descriptor, info.format
    );
    maplibre_native_ffi_vulkan_borrowed_texture_descriptor_set_final_layout(
      descriptor, info.vulkan_final_layout
    );
    target->session = maplibre_native_ffi_map_attach_vulkan_borrowed_texture(
      target->map, descriptor, options, &error
    );
  } else {
    g_autoptr(MaplibreNativeFfiRenderSessionAttachOptions) options =
      attach_options(
        target, MAPLIBRE_NATIVE_FFI_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
      );
    g_autoptr(MaplibreNativeFfiEglContextDescriptor) egl =
      maplibre_native_ffi_egl_context_descriptor_new();
    maplibre_native_ffi_egl_context_descriptor_set_display(
      egl, context.egl_display
    );
    maplibre_native_ffi_egl_context_descriptor_set_config(
      egl, context.egl_config
    );
    maplibre_native_ffi_egl_context_descriptor_set_share_context(
      egl, context.egl_context
    );
    maplibre_native_ffi_egl_context_descriptor_set_client_api(
      egl, MAPLIBRE_NATIVE_FFI_OPEN_GL_CLIENT_API_GLES
    );
    maplibre_native_ffi_egl_context_descriptor_set_get_proc_address(
      egl, context.get_proc_address
    );
    g_autoptr(MaplibreNativeFfiOpenGlContextDescriptorData) platform =
      maplibre_native_ffi_open_gl_context_descriptor_data_new_egl(egl);
    g_autoptr(MaplibreNativeFfiOpenGlContextDescriptor) opengl =
      maplibre_native_ffi_open_gl_context_descriptor_new();
    maplibre_native_ffi_open_gl_context_descriptor_set_ownership(
      opengl, MAPLIBRE_NATIVE_FFI_OPEN_GL_CONTEXT_OWNERSHIP_SHARED
    );
    maplibre_native_ffi_open_gl_context_descriptor_set_data(opengl, platform);
    MaplibreNativeFfiOpenGlBorrowedTexture texture = {
      .texture = info.opengl_texture
    };
    g_autoptr(MaplibreNativeFfiOpenGlBorrowedTextureDescriptor) descriptor =
      maplibre_native_ffi_open_gl_borrowed_texture_descriptor_new();
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_logical_size(
      descriptor, &size
    );
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_width(
      descriptor, info.width
    );
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_physical_height(
      descriptor, info.height
    );
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_context(
      descriptor, opengl
    );
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_textures(
      descriptor, &texture, 1
    );
    maplibre_native_ffi_open_gl_borrowed_texture_descriptor_set_target(
      descriptor, info.opengl_target
    );
    target->session = maplibre_native_ffi_map_attach_open_gl_borrowed_texture(
      target->map, descriptor, options, &error
    );
  }
  g_assert_no_error(error);
  wait_until_ready(target);
}

static void a_borrowed_texture_session_lends_its_frames(void) {
  uint32_t backend = 0;
  if (backend_supported(MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_VULKAN)) {
    backend = MLN_TEST_GRAPHICS_BACKEND_VULKAN;
  } else if (
    backend_supported(MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_OPEN_GL)
  ) {
    backend = MLN_TEST_GRAPHICS_BACKEND_EGL;
  } else {
    g_test_skip("this build renders with neither Vulkan nor EGL");
    return;
  }
  Target target = {.thread = g_thread_self()};
  target.runtime = create_runtime();
  target.map = create_map(target.runtime, 0, 32, 16);
  attach_borrowed(&target, backend);
  set_style(target.map, red_style);
  g_autoptr(GError) error = NULL;
  g_autoptr(MaplibreNativeFfiAcquiredFrame) frame = NULL;
  while (frame == NULL) {
    g_autoptr(MaplibreNativeFfiRenderFrameResult) result = render(&target);
    frame =
      maplibre_native_ffi_render_session_acquire_frame(target.session, &error);
    g_assert_no_error(error);
  }
  g_autoptr(MaplibreNativeFfiRenderFrameResult) result =
    maplibre_native_ffi_acquired_frame_get_result(frame, &error);
  g_assert_no_error(error);
  g_assert_cmpint(
    result->disposition, ==, MAPLIBRE_NATIVE_FFI_RENDER_RESULT_RENDERED
  );
  mln_test_graphics_texture_info info = {0};
  g_assert_true(mln_test_graphics_texture_get_info(target.texture, &info));
  if (backend == MLN_TEST_GRAPHICS_BACKEND_VULKAN) {
    g_autoptr(MaplibreNativeFfiVulkanTextureFrame) texture =
      maplibre_native_ffi_acquired_frame_get_vulkan_texture(frame, &error);
    g_assert_no_error(error);
    g_assert_cmpuint(
      maplibre_native_ffi_vulkan_texture_frame_get_image(texture), ==,
      info.vulkan_image
    );
  } else {
    g_autoptr(MaplibreNativeFfiOpenGlTextureFrame) texture =
      maplibre_native_ffi_acquired_frame_get_open_gl_texture(frame, &error);
    g_assert_no_error(error);
    g_assert_cmpuint(texture->texture, ==, info.opengl_texture);
  }
  const MaplibreNativeFfiGpuSync done = {
    .kind = MAPLIBRE_NATIVE_FFI_GPU_SYNC_CPU_COMPLETE
  };
  // The frame lends its producer's sync, as it lends its texture.
  g_autoptr(MaplibreNativeFfiGpuSync) producer =
    maplibre_native_ffi_acquired_frame_get_producer_sync(frame, &error);
  g_assert_no_error(error);
  g_assert_true(
    maplibre_native_ffi_acquired_frame_release(frame, &done, &error)
  );
  g_assert_no_error(error);
  // A released frame's object no longer holds it.
  g_assert_false(
    maplibre_native_ffi_acquired_frame_release(frame, &done, &error)
  );
  g_assert_error(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MAPLIBRE_NATIVE_FFI_ERROR_INVALID_ARGUMENT
  );
  g_clear_object(&frame);
  if (backend == MLN_TEST_GRAPHICS_BACKEND_EGL) {
    g_assert_cmpint(target.serviced, >, 0);
  }
  release_target(&target);
}

static void a_disposed_session_takes_driver_work_until_it_ends(void) {
  if (!backend_supported(MAPLIBRE_NATIVE_FFI_RENDER_BACKEND_FLAG_OPEN_GL)) {
    g_test_skip("this build has no session driven by the caller");
    return;
  }
  Target target = {.thread = g_thread_self()};
  target.runtime = create_runtime();
  target.map = create_map(target.runtime, 0, 32, 16);
  attach_borrowed(&target, MLN_TEST_GRAPHICS_BACKEND_EGL);
  MaplibreNativeFfiRenderSession* session = g_steal_pointer(&target.session);
  maplibre_native_ffi_render_session_dispose(session);
  // The disposal finishes in driver work on this thread.
  g_autoptr(GError) error = NULL;
  g_assert_true(
    maplibre_native_ffi_render_session_service_driver_work(session, &error)
  );
  g_assert_no_error(error);
  while (
    maplibre_native_ffi_render_session_service_driver_work(session, &error)) {
    g_main_context_iteration(NULL, FALSE);
  }
  g_assert_error(
    error, MAPLIBRE_NATIVE_FFI_ERROR, MAPLIBRE_NATIVE_FFI_ERROR_INVALID_STATE
  );
  g_object_unref(session);
  release_map(target.map);
  release_runtime(target.runtime);
  mln_test_graphics_texture_destroy(target.texture);
  mln_test_graphics_destroy(target.graphics);
}

int main(int argc, char** argv) {
  g_test_init(&argc, &argv, NULL);
  g_test_add_func(
    "/errors/a-failed-status-is-an-error-of-the-domain",
    a_failed_status_is_an_error_of_the_domain
  );
  g_test_add_func(
    "/completions/finish-on-the-context-that-started-them",
    a_completion_finishes_on_the_context_that_started_it
  );
  g_test_add_func(
    "/completions/a-command-without-a-callback-still-applies",
    a_command_without_a_callback_still_applies
  );
  g_test_add_func(
    "/handles/a-map-is-ready-to-every-wait", a_map_is_ready_to_every_wait
  );
  g_test_add_func(
    "/handles/a-map-keeps-its-runtime-alive", a_map_keeps_its_runtime_alive
  );
  g_test_add_func(
    "/values/a-holder-copies-its-members", a_holder_copies_its_members
  );
  g_test_add_func(
    "/values/an-option-is-unset-until-a-property-sets-it",
    an_option_is_unset_until_a_property_sets_it
  );
  g_test_add_func(
    "/values/a-bulk-projection-returns-every-result",
    a_bulk_projection_returns_every_result
  );
  g_test_add_func(
    "/values/caller-sized-text-returns-all-of-a-long-result",
    caller_sized_text_returns_all_of_a_long_result
  );
  g_test_add_func(
    "/callbacks/events-reach-the-handler-on-the-main-context",
    events_reach_the_handler_on_the_main_context
  );
  g_test_add_func(
    "/callbacks/a-rejected-call-frees-its-registrations",
    a_rejected_call_frees_its_registrations
  );
#if defined(__linux__)
  g_test_add_func(
    "/render/a-dma-buf-ring-converts-its-formats",
    a_dma_buf_ring_converts_its_formats
  );
#endif
  g_test_add_func(
    "/render/a-session-renders-pixels-that-the-library-reads-back",
    a_session_renders_pixels_that_the_library_reads_back
  );
  g_test_add_func(
    "/render/a-custom-source-answers-through-the-map-it-borrows",
    a_custom_source_answers_through_the_map_it_borrows
  );
  g_test_add_func(
    "/render/a-borrowed-texture-session-lends-its-frames",
    a_borrowed_texture_session_lends_its_frames
  );
  g_test_add_func(
    "/render/a-disposed-session-takes-driver-work-until-it-ends",
    a_disposed_session_takes_driver_work_until_it_ends
  );
  return g_test_run();
}
