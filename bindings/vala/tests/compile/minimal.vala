[CCode(cname = "dlopen", cheader_filename = "dlfcn.h")]
extern void* dlopen(string filename, int flags);

[CCode(cname = "dlsym", cheader_filename = "dlfcn.h")]
extern void* dlsym(void* handle, string symbol);

[CCode(has_target = false)]
delegate void* MetalCreateSystemDefaultDeviceFunc();

[SimpleType]
[CCode(cname = "MlnValaMetalWindowLayer")]
struct MetalWindowLayer {
  public void* window;
  public void* layer;
}

[CCode(cname = "mln_vala_metal_test_window_layer_create")]
extern bool metal_test_window_layer_create(uint32 width, uint32 height, out MetalWindowLayer layer);

[CCode(cname = "mln_vala_metal_test_window_layer_destroy")]
extern void metal_test_window_layer_destroy(ref MetalWindowLayer layer);

[CCode(cname = "mln_vala_metal_test_texture_create")]
extern void* metal_test_texture_create(void* device, uint32 width, uint32 height);

[CCode(cname = "mln_vala_metal_test_object_release")]
extern void metal_test_object_release(void* object);

[SimpleType]
[CCode(cname = "MlnValaVulkanTestContext")]
struct VulkanTestContext {
  public void* instance;
  public void* physical_device;
  public void* device;
  public void* graphics_queue;
  public uint32 graphics_queue_family_index;
  public void* get_instance_proc_addr;
  public void* get_device_proc_addr;
}

[SimpleType]
[CCode(cname = "MlnValaVulkanBorrowedImage")]
struct VulkanBorrowedImage {
  public void* image;
  public void* image_view;
  public void* memory;
  public uint32 format;
  public uint32 initial_layout;
  public uint32 final_layout;
}

[CCode(cname = "mln_vala_vulkan_test_context_create")]
extern bool vulkan_test_context_create(out VulkanTestContext context);

[CCode(cname = "mln_vala_vulkan_test_borrowed_image_create")]
extern bool vulkan_test_borrowed_image_create(ref VulkanTestContext context, uint32 width, uint32 height, out VulkanBorrowedImage image);

[CCode(cname = "mln_vala_vulkan_test_borrowed_image_destroy")]
extern void vulkan_test_borrowed_image_destroy(ref VulkanTestContext context, ref VulkanBorrowedImage image);

[CCode(cname = "mln_vala_vulkan_test_surface_supported")]
extern bool vulkan_test_surface_supported();

[CCode(cname = "mln_vala_vulkan_test_surface_create")]
extern bool vulkan_test_surface_create(ref VulkanTestContext context, void* metal_layer, out void* surface);

[CCode(cname = "mln_vala_vulkan_test_surface_destroy")]
extern void vulkan_test_surface_destroy(ref VulkanTestContext context, void* surface);

[CCode(cname = "mln_vala_vulkan_test_context_destroy")]
extern void vulkan_test_context_destroy(ref VulkanTestContext context);

[SimpleType]
[CCode(cname = "MlnValaOpenGLTestContext")]
struct OpenGLTestContext {
  public uint32 platform;
  public void* display;
  public void* config;
  public void* context;
  public void* surface;
  public void* get_proc_address;
}

[CCode(cname = "mln_vala_opengl_test_context_supported")]
extern bool opengl_test_context_supported();

[CCode(cname = "mln_vala_opengl_test_context_create")]
extern bool opengl_test_context_create(uint32 width, uint32 height, out OpenGLTestContext context);

[CCode(cname = "mln_vala_opengl_test_context_destroy")]
extern void opengl_test_context_destroy(ref OpenGLTestContext context);

int log_count = 0;
int resource_transform_count = 0;
int resource_provider_request_count = 0;
int resource_provider_async_complete_count = 0;
int resource_provider_one_shot_error_count = 0;
int custom_geometry_fetch_count = 0;
int custom_geometry_cancel_count = 0;
Mutex callback_count_mutex;

void increment_callback_count(ref int count) {
  callback_count_mutex.lock();
  count++;
  callback_count_mutex.unlock();
}

int read_callback_count(ref int count) {
  callback_count_mutex.lock();
  var value = count;
  callback_count_mutex.unlock();
  return value;
}

void* create_system_default_metal_device() {
  var module = dlopen("/System/Library/Frameworks/Metal.framework/Metal", 1);
  if (module == null) {
    return null;
  }
  var symbol = dlsym(module, "MTLCreateSystemDefaultDevice");
  if (symbol == null) {
    return null;
  }
  var create_device = (MetalCreateSystemDefaultDeviceFunc) symbol;
  return create_device();
}

void inspect_runtime_event_payload(MaplibreNative.RuntimeEvent event) {
  event.payload_type.to_string();
  if (event.render_frame != null) {
    event.render_frame.mode.to_string();
    event.render_frame.stats.frame_count.to_string();
  }
  if (event.render_map != null) {
    event.render_map.mode.to_string();
  }
  if (event.style_image_missing != null) {
    event.style_image_missing.get_image_id_utf8().length.to_string();
  }
  if (event.tile_action != null) {
    event.tile_action.operation.to_string();
    event.tile_action.tile_id.canonical_x.to_string();
    event.tile_action.get_source_id_utf8().length.to_string();
  }
  if (event.offline_region_status != null) {
    event.offline_region_status.region_id.value.to_string();
    event.offline_region_status.status.download_state.to_string();
  }
  if (event.offline_region_response_error != null) {
    event.offline_region_response_error.reason.to_string();
  }
  if (event.offline_region_tile_count_limit != null) {
    event.offline_region_tile_count_limit.limit.to_string();
  }
  if (event.offline_operation_completed != null) {
    assert(event.offline_operation_completed.operation != null);
    event.offline_operation_completed.operation_kind.to_string();
    event.offline_operation_completed.result_kind.to_string();
  }
}

bool wait_for_runtime_event(MaplibreNative.RuntimeHandle runtime, MaplibreNative.RuntimeEventType event_type, uint attempts) throws MaplibreNative.Error {
  for (uint attempt = 0; attempt < attempts; attempt++) {
    runtime.run_once();
    var event = runtime.poll_event();
    if (event != null) {
      inspect_runtime_event_payload(event);
      if (event.event_type == event_type) {
        return true;
      }
    }
    GLib.Thread.usleep(1000);
  }
  return false;
}

void exercise_runtime_close_race() throws MaplibreNative.Error {
  var runtime = new MaplibreNative.RuntimeHandle();
  Mutex state_mutex = Mutex();
  Cond state_changed = Cond();
  bool lease_acquired = false;
  bool release_lease = false;
  bool close_rejected_new_call = false;
  bool holder_failed = false;

  var holder = new GLib.Thread<void>("vala-runtime-lease-holder", () => {
    try {
      var lease = runtime.require_live();
      state_mutex.lock();
      lease_acquired = true;
      state_changed.broadcast();
      while (!release_lease) {
        state_changed.wait(state_mutex);
      }
      state_mutex.unlock();
      assert(lease.native != null);
    } catch (MaplibreNative.Error error) {
      holder_failed = true;
    }
  });

  state_mutex.lock();
  while (!lease_acquired) {
    state_changed.wait(state_mutex);
  }
  state_mutex.unlock();

  var coordinator = new GLib.Thread<void>("vala-runtime-close-coordinator", () => {
    while (true) {
      try {
        var lease = runtime.require_live();
        assert(lease.native != null);
      } catch (MaplibreNative.Error.INVALID_STATE error) {
        state_mutex.lock();
        close_rejected_new_call = true;
        release_lease = true;
        state_changed.broadcast();
        state_mutex.unlock();
        return;
      } catch (MaplibreNative.Error error) {
        state_mutex.lock();
        holder_failed = true;
        release_lease = true;
        state_changed.broadcast();
        state_mutex.unlock();
        return;
      }
      GLib.Thread.usleep(100);
    }
  });

  runtime.close();
  holder.join();
  coordinator.join();
  assert(!holder_failed);
  assert(close_rejected_new_call);
  assert(runtime.closed);
  bool closed_call_failed = false;
  try {
    runtime.run_once();
  } catch (MaplibreNative.Error.INVALID_STATE error) {
    closed_call_failed = true;
  }
  assert(closed_call_failed);
}

void exercise_render_session_close_race(MaplibreNative.RenderSessionHandle session) throws MaplibreNative.Error {
  Mutex state_mutex = Mutex();
  Cond state_changed = Cond();
  bool lease_acquired = false;
  bool release_lease = false;
  bool close_rejected_new_call = false;
  bool holder_failed = false;

  var holder = new GLib.Thread<void>("vala-render-session-lease-holder", () => {
    try {
      var lease = session.require_live();
      state_mutex.lock();
      lease_acquired = true;
      state_changed.broadcast();
      while (!release_lease) {
        state_changed.wait(state_mutex);
      }
      state_mutex.unlock();
      assert(lease.native != null);
    } catch (MaplibreNative.Error error) {
      holder_failed = true;
    }
  });

  state_mutex.lock();
  while (!lease_acquired) {
    state_changed.wait(state_mutex);
  }
  state_mutex.unlock();

  var coordinator = new GLib.Thread<void>("vala-render-session-close-coordinator", () => {
    while (true) {
      try {
        var lease = session.require_live();
        assert(lease.native != null);
      } catch (MaplibreNative.Error.INVALID_STATE error) {
        state_mutex.lock();
        close_rejected_new_call = true;
        release_lease = true;
        state_changed.broadcast();
        state_mutex.unlock();
        return;
      } catch (MaplibreNative.Error error) {
        state_mutex.lock();
        holder_failed = true;
        release_lease = true;
        state_changed.broadcast();
        state_mutex.unlock();
        return;
      }
      GLib.Thread.usleep(100);
    }
  });

  session.close();
  holder.join();
  coordinator.join();
  assert(!holder_failed);
  assert(close_rejected_new_call);
  assert(session.closed);
}

void exercise_projection_close_race(MaplibreNative.MapProjectionHandle projection) throws MaplibreNative.Error {
  Mutex state_mutex = Mutex();
  Cond state_changed = Cond();
  bool lease_acquired = false;
  bool release_lease = false;
  bool close_rejected_new_call = false;
  bool holder_failed = false;

  var holder = new GLib.Thread<void>("vala-projection-lease-holder", () => {
    try {
      var lease = projection.require_live();
      state_mutex.lock();
      lease_acquired = true;
      state_changed.broadcast();
      while (!release_lease) {
        state_changed.wait(state_mutex);
      }
      state_mutex.unlock();
      assert(lease.native != null);
    } catch (MaplibreNative.Error error) {
      holder_failed = true;
    }
  });

  state_mutex.lock();
  while (!lease_acquired) {
    state_changed.wait(state_mutex);
  }
  state_mutex.unlock();

  var coordinator = new GLib.Thread<void>("vala-projection-close-coordinator", () => {
    while (true) {
      try {
        var lease = projection.require_live();
        assert(lease.native != null);
      } catch (MaplibreNative.Error.INVALID_STATE error) {
        state_mutex.lock();
        close_rejected_new_call = true;
        release_lease = true;
        state_changed.broadcast();
        state_mutex.unlock();
        return;
      } catch (MaplibreNative.Error error) {
        state_mutex.lock();
        holder_failed = true;
        release_lease = true;
        state_changed.broadcast();
        state_mutex.unlock();
        return;
      }
      GLib.Thread.usleep(100);
    }
  });

  projection.close();
  holder.join();
  coordinator.join();
  assert(!holder_failed);
  assert(close_rejected_new_call);
  assert(projection.closed);
}

void exercise_frame_access_state() throws MaplibreNative.Error {
  var state = new MaplibreNative.FrameAccessState("test frame");
  MaplibreNative.FrameAccessLease? access = state.acquire();
  bool active_close_failed = false;
  try {
    state.begin_close();
  } catch (MaplibreNative.Error.INVALID_STATE error) {
    active_close_failed = true;
  }
  assert(active_close_failed);
  access = null;
  assert(state.begin_close());
  state.finish_close(true);
  bool closed_access_failed = false;
  try {
    state.acquire();
  } catch (MaplibreNative.Error.INVALID_STATE error) {
    closed_access_failed = true;
  }
  assert(closed_access_failed);
}

void exercise_unknown_feature_identifier_rejection() throws MaplibreNative.Error {
  MaplibreNative.Raw.Feature native = {};
  native.identifier_type = 99;
  native.identifier_string_value = MaplibreNative.Raw.StringView() {
    data = (char*) 1,
    size = 4096
  };
  try {
    MaplibreNative.FeatureIdentifier.from_native(native);
    assert_not_reached();
  } catch (MaplibreNative.Error.UNSUPPORTED error) {
    assert(error.message == "unknown native feature identifier type 99");
  }

  MaplibreNative.Raw.Feature identifier_less = {};
  MaplibreNative.FeatureIdentifier.none().apply_to_native(ref identifier_less);
  assert(identifier_less.identifier_type == (uint32) MaplibreNative.FeatureIdentifierType.NULL);

  MaplibreNative.Raw.FeatureExtensionResultInfo unknown_extension = {};
  unknown_extension.type = 99;
  try {
    MaplibreNative.FeatureExtensionResult.from_native(unknown_extension);
    assert_not_reached();
  } catch (MaplibreNative.Error.UNSUPPORTED error) {
    assert(error.message == "unknown feature extension result type 99");
  }
}

void exercise_json_null_rejection() throws MaplibreNative.Error {
  MaplibreNative.JsonValue[] null_values = new MaplibreNative.JsonValue[1];
  try {
    MaplibreNative.JsonValue.array_value(null_values);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "JSON array value at index 0 is null");
  }

  MaplibreNative.JsonMember[] null_members = new MaplibreNative.JsonMember[1];
  try {
    MaplibreNative.JsonValue.object_value(null_members);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "JSON object member at index 0 is null");
  }

  try {
    new MaplibreNative.JsonMember("null", null_values[0]);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "JSON object member value is null");
  }

  try {
    new MaplibreNative.Feature(
      MaplibreNative.Geometry.point(MaplibreNative.LatLng(0.0, 0.0)),
      null_members);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "feature property at index 0 is null");
  }

  MaplibreNative.Feature[] null_features = new MaplibreNative.Feature[1];
  try {
    new MaplibreNative.FeatureCollection(null_features);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "feature collection item at index 0 is null");
  }

  MaplibreNative.Raw.JsonValue native_array = {};
  native_array.type = (uint32) MaplibreNative.JsonValueType.ARRAY;
  native_array.array_value.value_count = 1;
  try {
    MaplibreNative.JsonValue.from_native(native_array);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "native JSON array values are null");
  }

  MaplibreNative.Raw.JsonMember native_member = {};
  MaplibreNative.Raw.JsonValue native_object = {};
  native_object.type = (uint32) MaplibreNative.JsonValueType.OBJECT;
  native_object.object_value.members = &native_member;
  native_object.object_value.member_count = 1;
  try {
    MaplibreNative.JsonValue.from_native(native_object);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "native JSON object member value is null");
  }
}

void exercise_defensive_byte_snapshots(MaplibreNative.RuntimeHandle runtime) throws MaplibreNative.Error {
  uint8[] prior_data = { 1, 2, 3 };
  MaplibreNative.Raw.ResourceRequest native_request = {};
  native_request.prior_data = prior_data;
  native_request.prior_data_size = prior_data.length;
  var request = new MaplibreNative.ResourceRequest.from_native(&native_request);
  var request_data = request.prior_data;
  assert(request_data != null);
  request_data[0] = 9;
  assert(request.prior_data[0] == 1);

  uint8[] response_data = { 7, 8, 9 };
  var response = MaplibreNative.ResourceResponse.data(response_data);
  response.must_revalidate = true;
  response.modified_unix_ms = 1;
  response.expires_unix_ms = 2;
  response.etag = "etag";
  response.retry_after_unix_ms = 3;
  var response_copy = response.copy();
  assert(response.equal(response_copy));
  response_data[0] = 0;
  var returned_response_data = response_copy.bytes;
  returned_response_data[0] = 0;
  assert(response.equal(response_copy));
  response_copy.must_revalidate = false;
  assert(!response.equal(response_copy));

  uint8[] payload = { 4, 5, 6 };
  MaplibreNative.Raw.RuntimeEvent native_event = {};
  native_event.payload_type = 99;
  native_event.payload = payload;
  native_event.payload_size = payload.length;
  var event = new MaplibreNative.RuntimeEvent(runtime, native_event);
  var event_data = event.payload_bytes;
  event_data[0] = 9;
  assert(event.payload_bytes[0] == 4);

  native_event.payload = null;
  native_event.payload_size = 1;
  try {
    new MaplibreNative.RuntimeEvent(runtime, native_event);
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message == "runtime event payload data is null");
  }
}

void exercise_option_value_semantics() throws MaplibreNative.Error {
  var map_options = new MaplibreNative.MapOptions();
  assert(map_options.equal(map_options.copy()));
  var map_options_changed = map_options.copy();
  map_options_changed.width++;
  assert(!map_options.equal(map_options_changed));
  map_options_changed = map_options.copy();
  map_options_changed.height++;
  assert(!map_options.equal(map_options_changed));
  map_options_changed = map_options.copy();
  map_options_changed.scale_factor = 2.0;
  assert(!map_options.equal(map_options_changed));
  map_options_changed = map_options.copy();
  map_options_changed.mode = MaplibreNative.MapMode.STATIC;
  assert(!map_options.equal(map_options_changed));

  var viewport_options = new MaplibreNative.MapViewportOptions();
  assert(viewport_options.equal(viewport_options.copy()));
  var viewport_changed = viewport_options.copy();
  viewport_changed.set_north_orientation((MaplibreNative.NorthOrientation) 0);
  assert(!viewport_options.equal(viewport_changed));
  viewport_changed = viewport_options.copy();
  viewport_changed.set_constrain_mode((MaplibreNative.ConstrainMode) 0);
  assert(!viewport_options.equal(viewport_changed));
  viewport_changed = viewport_options.copy();
  viewport_changed.set_viewport_mode((MaplibreNative.ViewportMode) 0);
  assert(!viewport_options.equal(viewport_changed));
  viewport_changed = viewport_options.copy();
  viewport_changed.set_frustum_offset(MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
  assert(!viewport_options.equal(viewport_changed));

  var tile_options = new MaplibreNative.MapTileOptions();
  assert(tile_options.equal(tile_options.copy()));
  var tile_changed = tile_options.copy();
  tile_changed.set_prefetch_zoom_delta(0);
  assert(!tile_options.equal(tile_changed));
  tile_changed = tile_options.copy();
  tile_changed.set_lod_min_radius(0.0);
  assert(!tile_options.equal(tile_changed));
  tile_changed = tile_options.copy();
  tile_changed.set_lod_scale(0.0);
  assert(!tile_options.equal(tile_changed));
  tile_changed = tile_options.copy();
  tile_changed.set_lod_pitch_threshold(0.0);
  assert(!tile_options.equal(tile_changed));
  tile_changed = tile_options.copy();
  tile_changed.set_lod_zoom_shift(0.0);
  assert(!tile_options.equal(tile_changed));
  tile_changed = tile_options.copy();
  tile_changed.set_lod_mode((MaplibreNative.TileLodMode) 0);
  assert(!tile_options.equal(tile_changed));

  var animation_options = new MaplibreNative.AnimationOptions();
  assert(animation_options.equal(animation_options.copy()));
  var animation_changed = animation_options.copy();
  animation_changed.set_duration_ms(0.0);
  assert(!animation_options.equal(animation_changed));
  animation_changed = animation_options.copy();
  animation_changed.set_velocity(0.0);
  assert(!animation_options.equal(animation_changed));
  animation_changed = animation_options.copy();
  animation_changed.set_min_zoom(0.0);
  assert(!animation_options.equal(animation_changed));
  animation_changed = animation_options.copy();
  animation_changed.set_easing(MaplibreNative.UnitBezier(0.0, 0.0, 0.0, 0.0));
  assert(!animation_options.equal(animation_changed));

  var fit_options = new MaplibreNative.CameraFitOptions();
  assert(fit_options.equal(fit_options.copy()));
  var fit_changed = fit_options.copy();
  fit_changed.set_padding(MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
  assert(!fit_options.equal(fit_changed));
  fit_changed = fit_options.copy();
  fit_changed.set_bearing(0.0);
  assert(!fit_options.equal(fit_changed));
  fit_changed = fit_options.copy();
  fit_changed.set_pitch(0.0);
  assert(!fit_options.equal(fit_changed));

  var bound_options = new MaplibreNative.BoundOptions();
  assert(bound_options.equal(bound_options.copy()));
  var bound_changed = bound_options.copy();
  bound_changed.set_bounds(MaplibreNative.LatLngBounds(
    MaplibreNative.LatLng(0.0, 0.0),
    MaplibreNative.LatLng(0.0, 0.0)));
  assert(!bound_options.equal(bound_changed));
  bound_changed = bound_options.copy();
  bound_changed.set_min_zoom(0.0);
  assert(!bound_options.equal(bound_changed));
  bound_changed = bound_options.copy();
  bound_changed.set_max_zoom(0.0);
  assert(!bound_options.equal(bound_changed));
  bound_changed = bound_options.copy();
  bound_changed.set_min_pitch(0.0);
  assert(!bound_options.equal(bound_changed));
  bound_changed = bound_options.copy();
  bound_changed.set_max_pitch(0.0);
  assert(!bound_options.equal(bound_changed));

  var free_camera_options = new MaplibreNative.FreeCameraOptions();
  assert(free_camera_options.equal(free_camera_options.copy()));
  var free_camera_changed = free_camera_options.copy();
  free_camera_changed.set_position(MaplibreNative.Vec3(0.0, 0.0, 0.0));
  assert(!free_camera_options.equal(free_camera_changed));
  free_camera_changed = free_camera_options.copy();
  free_camera_changed.set_orientation(MaplibreNative.Quaternion(0.0, 0.0, 0.0, 0.0));
  assert(!free_camera_options.equal(free_camera_changed));

  var projection_mode = new MaplibreNative.ProjectionMode();
  assert(projection_mode.equal(projection_mode.copy()));
  var projection_mode_changed = projection_mode.copy();
  projection_mode_changed.set_axonometric(false);
  assert(!projection_mode.equal(projection_mode_changed));
  projection_mode_changed = projection_mode.copy();
  projection_mode_changed.set_x_skew(0.0);
  assert(!projection_mode.equal(projection_mode_changed));
  projection_mode_changed = projection_mode.copy();
  projection_mode_changed.set_y_skew(0.0);
  assert(!projection_mode.equal(projection_mode_changed));

  var runtime_options = new MaplibreNative.RuntimeOptions();
  assert(runtime_options.equal(runtime_options.copy()));
  var runtime_changed = runtime_options.copy();
  runtime_changed.asset_path = "";
  assert(!runtime_options.equal(runtime_changed));
  runtime_changed = runtime_options.copy();
  runtime_changed.cache_path = "";
  assert(!runtime_options.equal(runtime_changed));
  runtime_changed = runtime_options.copy();
  runtime_changed.maximum_cache_size = 0;
  assert(!runtime_options.equal(runtime_changed));

  uint8[] utf8_source = { 'a', 0, 'b' };
  var utf8_value = new MaplibreNative.Utf8String.from_bytes(utf8_source);
  utf8_source[0] = 'z';
  assert(utf8_value.to_bytes()[0] == 'a');
  var returned_utf8 = utf8_value.to_bytes();
  returned_utf8[2] = 'z';
  assert(utf8_value.to_bytes()[2] == 'b');
  assert(utf8_value.equal(utf8_value.copy()));
  assert(utf8_value.to_string_or_null() == null);
  var multibyte_utf8 = new MaplibreNative.Utf8String("é");
  assert(multibyte_utf8.length == 2);
  assert(multibyte_utf8.to_bytes()[0] == 0xc3 && multibyte_utf8.to_bytes()[1] == 0xa9);
  try {
    utf8_value.to_string();
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_STATE error) {
    assert(error.message != "");
  }
  try {
    new MaplibreNative.Utf8String.from_bytes({ 0xc0, 0x80 });
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message != "");
  }
  try {
    MaplibreNative.JsonValue.string_utf8_bytes({ 0xff });
    assert_not_reached();
  } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
    assert(error.message != "");
  }

  var rendered_options = new MaplibreNative.RenderedFeatureQueryOptions();
  assert(rendered_options.equal(rendered_options.copy()));
  var rendered_changed = rendered_options.copy();
  rendered_changed.set_layer_ids({});
  assert(!rendered_options.equal(rendered_changed));
  rendered_changed = rendered_options.copy();
  rendered_changed.set_filter(MaplibreNative.JsonValue.null_value());
  assert(!rendered_options.equal(rendered_changed));
  rendered_options.set_layer_ids({ "one", "two" });
  rendered_options.set_filter(MaplibreNative.JsonValue.object_value({
    new MaplibreNative.JsonMember(
      "nested",
      MaplibreNative.JsonValue.array_value({ MaplibreNative.JsonValue.int_value(1) }))
  }));
  var rendered_copy = rendered_options.copy();
  assert(rendered_options.equal(rendered_copy));
  rendered_copy.set_layer_ids({ "changed" });
  assert(!rendered_options.equal(rendered_copy));
  rendered_options.set_layer_ids_utf8({ utf8_value });
  assert(rendered_options.equal(rendered_options.copy()));

  var source_options = new MaplibreNative.SourceFeatureQueryOptions();
  assert(source_options.equal(source_options.copy()));
  var source_changed = source_options.copy();
  source_changed.set_source_layer_ids({});
  assert(!source_options.equal(source_changed));
  source_changed = source_options.copy();
  source_changed.set_filter(MaplibreNative.JsonValue.null_value());
  assert(!source_options.equal(source_changed));
  source_options.set_source_layer_ids({ "one", "two" });
  source_options.set_filter(MaplibreNative.JsonValue.array_value({
    MaplibreNative.JsonValue.string_value("nested")
  }));
  var source_copy = source_options.copy();
  assert(source_options.equal(source_copy));
  source_copy.set_source_layer_ids({ "changed" });
  assert(!source_options.equal(source_copy));
  source_options.set_source_layer_ids_utf8({ utf8_value });
  assert(source_options.equal(source_options.copy()));

  var query_point = MaplibreNative.RenderedQueryGeometry.point(MaplibreNative.ScreenPoint(1.0, 2.0));
  assert(query_point.equal(query_point.copy()));
  assert(!query_point.equal(MaplibreNative.RenderedQueryGeometry.point(MaplibreNative.ScreenPoint(2.0, 1.0))));
  var query_line = MaplibreNative.RenderedQueryGeometry.line_string({
    MaplibreNative.ScreenPoint(1.0, 2.0),
    MaplibreNative.ScreenPoint(3.0, 4.0)
  });
  assert(query_line.equal(query_line.copy()));
  assert(!query_line.equal(query_point));

  var state_selector = new MaplibreNative.FeatureStateSelector.from_utf8(utf8_value);
  state_selector.set_source_layer_id_utf8(utf8_value);
  state_selector.set_feature_id("feature");
  state_selector.set_state_key("state");
  assert(state_selector.equal(state_selector.copy()));
  var changed_state_selector = state_selector.copy();
  changed_state_selector.set_state_key("changed");
  assert(!state_selector.equal(changed_state_selector));

  uint8[] response_source = { 1, 2, 3 };
  var resource_response = MaplibreNative.ResourceResponse.data(response_source);
  response_source[0] = 9;
  assert(resource_response.bytes[0] == 1);
  var returned_response_bytes = resource_response.bytes;
  returned_response_bytes[1] = 9;
  assert(resource_response.bytes[1] == 2);

  var tile_source_options = new MaplibreNative.StyleTileSourceOptions();
  assert(tile_source_options.equal(tile_source_options.copy()));
  var tile_source_changed = tile_source_options.copy();
  tile_source_changed.min_zoom = 0.0;
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.max_zoom = 0.0;
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.attribution = "";
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.set_attribution_utf8(utf8_value);
  assert(tile_source_changed.get_attribution_utf8().equal(utf8_value));
  assert(tile_source_changed.attribution == null);
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.scheme = (MaplibreNative.StyleTileScheme) 0;
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.bounds = MaplibreNative.LatLngBounds(
    MaplibreNative.LatLng(0.0, 0.0),
    MaplibreNative.LatLng(0.0, 0.0));
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.tile_size = 0;
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.vector_encoding = (MaplibreNative.StyleVectorTileEncoding) 0;
  assert(!tile_source_options.equal(tile_source_changed));
  tile_source_changed = tile_source_options.copy();
  tile_source_changed.raster_encoding = (MaplibreNative.StyleRasterDemEncoding) 0;
  assert(!tile_source_options.equal(tile_source_changed));

  var image_options = new MaplibreNative.StyleImageOptions();
  assert(image_options.equal(image_options.copy()));
  var image_changed = image_options.copy();
  image_changed.pixel_ratio = 0.0f;
  assert(!image_options.equal(image_changed));
  image_changed = image_options.copy();
  image_changed.sdf = false;
  assert(!image_options.equal(image_changed));

  var camera_options = new MaplibreNative.CameraOptions();
  assert(camera_options.equal(camera_options.copy()));
  var camera_changed = camera_options.copy();
  camera_changed.set_center(MaplibreNative.LatLng(0.0, 0.0));
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_center_altitude(0.0);
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_padding(MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_anchor(MaplibreNative.ScreenPoint(0.0, 0.0));
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_zoom(0.0);
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_bearing(0.0);
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_pitch(0.0);
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_roll(0.0);
  assert(!camera_options.equal(camera_changed));
  camera_changed = camera_options.copy();
  camera_changed.set_field_of_view(0.0);
  assert(!camera_options.equal(camera_changed));
}

MaplibreNative.RuntimeEventOfflineOperationCompleted? wait_for_offline_operation(MaplibreNative.RuntimeHandle runtime, MaplibreNative.OfflineOperationHandle operation) throws MaplibreNative.Error {
  for (uint attempt = 0; attempt < 5000; attempt++) {
    runtime.run_once();
    while (true) {
      var event = runtime.poll_event();
      if (event == null) {
        break;
      }
      inspect_runtime_event_payload(event);
      if (event.offline_operation_completed != null && event.offline_operation_completed.operation == operation) {
        return event.offline_operation_completed;
      }
    }
    GLib.Thread.usleep(1000);
  }
  return null;
}

bool handle_log(MaplibreNative.LogSeverity severity, MaplibreNative.LogEvent event, int64 code, string? message) {
  increment_callback_count(ref log_count);
  return false;
}

uint8[] bytes_from_string(string value) {
  uint8[] bytes = new uint8[value.length];
  for (var index = 0; index < value.length; index++) {
    bytes[index] = value[index];
  }
  return bytes;
}

string? transform_resource(MaplibreNative.ResourceKind kind, string url) {
  increment_callback_count(ref resource_transform_count);
  if (url == "http://maplibre-vala.invalid/style.json") {
    return "unsupported://rewritten-style.json";
  }
  return null;
}

MaplibreNative.ResourceProviderDecision provide_resource(MaplibreNative.ResourceRequest request, MaplibreNative.ResourceRequestHandle handle) {
  increment_callback_count(ref resource_provider_request_count);
  if (request.url == "custom://async-style.json") {
    new GLib.Thread<void>("vala-resource-provider", () => {
      try {
        var response = MaplibreNative.ResourceResponse.data(bytes_from_string("{\"version\":8,\"sources\":{},\"layers\":[]}"));
        handle.complete_and_release(response);
        increment_callback_count(ref resource_provider_async_complete_count);
      } catch (MaplibreNative.Error error) {
      }
    });
    return MaplibreNative.ResourceProviderDecision.HANDLE;
  }
  if (request.url != "custom://style.json") {
    return MaplibreNative.ResourceProviderDecision.PASS_THROUGH;
  }
  try {
    var response = MaplibreNative.ResourceResponse.data(bytes_from_string("{\"version\":8,\"sources\":{},\"layers\":[]}"));
    try {
      handle.release();
    } catch (MaplibreNative.Error error) {
      increment_callback_count(ref resource_provider_one_shot_error_count);
    }
    if (!handle.cancelled()) {
      handle.complete(response);
    }
    try {
      handle.complete_and_release(response);
    } catch (MaplibreNative.Error error) {
      increment_callback_count(ref resource_provider_one_shot_error_count);
    }
    assert(!handle.released);
  } catch (MaplibreNative.Error error) {
  }
  return MaplibreNative.ResourceProviderDecision.HANDLE;
}

void fetch_custom_geometry_tile(MaplibreNative.CanonicalTileId tile_id) {
  increment_callback_count(ref custom_geometry_fetch_count);
}

void cancel_custom_geometry_tile(MaplibreNative.CanonicalTileId tile_id) {
  increment_callback_count(ref custom_geometry_cancel_count);
}

void compile_location_indicator_property_wrappers(MaplibreNative.MapHandle map) throws MaplibreNative.Error {
  map.set_location_indicator_location("location", MaplibreNative.LatLng(1.0, 2.0), 3.0);
  map.set_location_indicator_bearing("location", 45.0);
  map.set_location_indicator_accuracy_radius("location", 12.0);
  map.set_location_indicator_image_name("location", MaplibreNative.LocationIndicatorImageKind.TOP, "location-marker");
}

string create_offline_merge_database() throws MaplibreNative.Error {
  var side_database_path = "%s/maplibre-native-vala-side-offline-%lld.db".printf(GLib.Environment.get_tmp_dir(), GLib.get_monotonic_time());
  GLib.FileUtils.remove(side_database_path);
  var side_options = new MaplibreNative.RuntimeOptions();
  side_options.cache_path = side_database_path;
  var side_runtime = new MaplibreNative.RuntimeHandle(side_options);
  var bounds = MaplibreNative.LatLngBounds(MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0));
  var tile_definition = MaplibreNative.OfflineRegionDefinition.tile_pyramid("maplibre://styles/offline", bounds, 0.0, 1.0);
  uint8[] metadata = { 8, 9, 10 };
  var create_id = side_runtime.offline_region_create_start(tile_definition, metadata);
  assert(wait_for_offline_operation(side_runtime, create_id) != null);
  side_runtime.offline_region_create_take_result(create_id);
  side_runtime.close();
  return side_database_path;
}

void compile_offline_region_wrappers(MaplibreNative.RuntimeHandle runtime, string? merge_database_path = null) throws MaplibreNative.Error {
  var bounds = MaplibreNative.LatLngBounds(MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0));
  var tile_definition = MaplibreNative.OfflineRegionDefinition.tile_pyramid("maplibre://styles/offline", bounds, 0.0, 1.0);
  var geometry_definition = MaplibreNative.OfflineRegionDefinition.geometry_region("maplibre://styles/offline", MaplibreNative.Geometry.point(MaplibreNative.LatLng(0.0, 0.0)), 0.0, 1.0);
  assert(tile_definition.equal(tile_definition.copy()));
  assert(geometry_definition.equal(geometry_definition.copy()));
  assert(!tile_definition.equal(geometry_definition));
  uint8[] metadata = { 1, 2, 3 };
  var create_id = runtime.offline_region_create_start(tile_definition, metadata);
  var completion = wait_for_offline_operation(runtime, create_id);
  assert(completion != null && completion.operation_kind == MaplibreNative.OfflineOperationKind.REGION_CREATE && completion.result_kind == MaplibreNative.OfflineOperationResultKind.REGION);
  bool wrong_take_failed = false;
  try {
    runtime.offline_regions_list_take_result(create_id);
  } catch (MaplibreNative.Error error) {
    wrong_take_failed = true;
  }
  assert(wrong_take_failed);
  var info = runtime.offline_region_create_take_result(create_id);
  assert(info.id.value > 0 && info.metadata.length == metadata.length);
  var info_copy = info.copy();
  assert(info.equal(info_copy));
  var copied_metadata = info.metadata;
  copied_metadata[0] = 99;
  assert(info.metadata[0] == metadata[0]);
  if (GLib.Environment.get_variable("MLN_VALA_RUN_OFFLINE_REGION_SMOKE") == "1") {
    var region_id = info.id;
    var geometry_create_id = runtime.offline_region_create_start(geometry_definition, metadata);
    runtime.discard_offline_operation(geometry_create_id);
    var get_id = runtime.offline_region_get_start(region_id);
    assert(wait_for_offline_operation(runtime, get_id) != null);
    var get_snapshot = runtime.offline_region_get_take_result(get_id);
    assert(get_snapshot != null);
    var list_id = runtime.offline_regions_list_start();
    assert(wait_for_offline_operation(runtime, list_id) != null);
    var list = runtime.offline_regions_list_take_result(list_id);
    assert(list.length > 0);
    if (merge_database_path != null) {
      var merge_id = runtime.offline_regions_merge_database_start(merge_database_path);
      assert(wait_for_offline_operation(runtime, merge_id) != null);
      var merged_list = runtime.offline_regions_merge_database_take_result(merge_id);
      assert(merged_list.length > 0);
    }
    uint8[] updated_metadata = { 4, 5, 6, 7 };
    var metadata_id = runtime.offline_region_update_metadata_start(region_id, updated_metadata);
    assert(wait_for_offline_operation(runtime, metadata_id) != null);
    var metadata_info = runtime.offline_region_update_metadata_take_result(metadata_id);
    assert(metadata_info.metadata.length == updated_metadata.length);
    var status_id = runtime.offline_region_get_status_start(region_id);
    assert(wait_for_offline_operation(runtime, status_id) != null);
    var status = runtime.offline_region_get_status_take_result(status_id);
    status.download_state.to_string();
    var observed_id = runtime.offline_region_set_observed_start(region_id, false);
    assert(wait_for_offline_operation(runtime, observed_id) != null);
    observed_id.close();
    var download_id = runtime.offline_region_set_download_state_start(region_id, MaplibreNative.OfflineRegionDownloadState.INACTIVE);
    assert(wait_for_offline_operation(runtime, download_id) != null);
    download_id.close();
    var invalidate_id = runtime.offline_region_invalidate_start(region_id);
    assert(wait_for_offline_operation(runtime, invalidate_id) != null);
    invalidate_id.close();
    var delete_id = runtime.offline_region_delete_start(region_id);
    assert(wait_for_offline_operation(runtime, delete_id) != null);
    delete_id.close();
  }
}

void compile_style_light_wrappers(MaplibreNative.MapHandle map) throws MaplibreNative.Error {
  map.set_style_light_json(MaplibreNative.JsonValue.object_value({
    new MaplibreNative.JsonMember("anchor", MaplibreNative.JsonValue.string_value("viewport")),
    new MaplibreNative.JsonMember("color", MaplibreNative.JsonValue.string_value("white")),
    new MaplibreNative.JsonMember("intensity", MaplibreNative.JsonValue.double_value(0.5))
  }));
  map.set_style_light_property("intensity", MaplibreNative.JsonValue.double_value(0.75));
  map.get_style_light_property("intensity");
}

void compile_utf8_string_view_wrappers(
  MaplibreNative.MapHandle map,
  MaplibreNative.RenderSessionHandle session,
  MaplibreNative.GeoJson geojson,
  MaplibreNative.Feature feature,
  MaplibreNative.PremultipliedRgba8Image image,
  MaplibreNative.CustomGeometrySourceOptions custom_options) throws MaplibreNative.Error {
  var id = new MaplibreNative.Utf8String("id");
  var other = new MaplibreNative.Utf8String("other");
  var url = new MaplibreNative.Utf8String("https://example.invalid/{z}/{x}/{y}");
  MaplibreNative.Utf8String[] tiles = { url };
  var json = MaplibreNative.JsonValue.null_value();
  var tile_options = new MaplibreNative.StyleTileSourceOptions();
  tile_options.set_attribution_utf8(id);
  var tile_id = MaplibreNative.CanonicalTileId(0, 0, 0);
  var bounds = MaplibreNative.LatLngBounds(
    MaplibreNative.LatLng(-1.0, -1.0),
    MaplibreNative.LatLng(1.0, 1.0));
  MaplibreNative.LatLng[] coordinates = {
    MaplibreNative.LatLng(-1.0, -1.0),
    MaplibreNative.LatLng(1.0, -1.0),
    MaplibreNative.LatLng(1.0, 1.0),
    MaplibreNative.LatLng(-1.0, 1.0)
  };

  map.add_geojson_source_url_utf8(id, url);
  map.add_geojson_source_data_utf8(id, geojson);
  map.set_geojson_source_url_utf8(id, url);
  map.set_geojson_source_data_utf8(id, geojson);
  map.add_style_source_json_utf8(id, json);
  map.add_vector_source_url_utf8(id, url, tile_options);
  map.add_vector_source_tiles_utf8(id, tiles, tile_options);
  map.add_raster_source_url_utf8(id, url, tile_options);
  map.add_raster_source_tiles_utf8(id, tiles, tile_options);
  map.add_raster_dem_source_url_utf8(id, url, tile_options);
  map.add_raster_dem_source_tiles_utf8(id, tiles, tile_options);
  map.add_custom_geometry_source_utf8(id, custom_options);
  map.set_custom_geometry_source_tile_data_utf8(id, tile_id, geojson);
  map.invalidate_custom_geometry_source_tile_utf8(id, tile_id);
  map.invalidate_custom_geometry_source_region_utf8(id, bounds);
  map.remove_style_source_utf8(id);
  map.style_source_exists_utf8(id);
  map.get_style_source_type_utf8(id);
  map.get_style_source_info_utf8(id);
  map.copy_style_source_attribution_utf8(id);

  map.add_hillshade_layer_utf8(id, other);
  map.add_color_relief_layer_utf8(id, other, null);
  map.add_location_indicator_layer_utf8(id, null);
  map.set_location_indicator_location_utf8(id, MaplibreNative.LatLng(0.0, 0.0), 0.0);
  map.set_location_indicator_bearing_utf8(id, 0.0);
  map.set_location_indicator_accuracy_radius_utf8(id, 0.0);
  map.set_location_indicator_image_name_utf8(id, MaplibreNative.LocationIndicatorImageKind.TOP, other);
  map.add_style_layer_json_utf8(json, null);
  map.remove_style_layer_utf8(id);
  map.style_layer_exists_utf8(id);
  map.get_style_layer_type_utf8(id);
  map.move_style_layer_utf8(id, null);
  map.get_style_layer_json_utf8(id);
  map.set_style_light_property_utf8(id, json);
  map.get_style_light_property_utf8(id);
  map.set_layer_property_utf8(id, other, json);
  map.get_layer_property_utf8(id, other);
  map.set_layer_filter_utf8(id, json);
  map.get_layer_filter_utf8(id);
  map.set_style_image_utf8(id, image);
  map.remove_style_image_utf8(id);
  map.style_image_exists_utf8(id);
  map.get_style_image_info_utf8(id);
  map.copy_style_image_premultiplied_rgba8_utf8(id);

  map.add_image_source_url_utf8(id, coordinates, url);
  map.add_image_source_image_utf8(id, coordinates, image);
  map.set_image_source_url_utf8(id, url);
  map.set_image_source_image_utf8(id, image);
  map.set_image_source_coordinates_utf8(id, coordinates);
  map.get_image_source_coordinates_utf8(id);

  var selector = new MaplibreNative.FeatureStateSelector.from_utf8(id);
  selector.set_source_layer_id_utf8(other);
  selector.set_feature_id_utf8(other);
  selector.set_state_key_utf8(other);
  var rendered_options = new MaplibreNative.RenderedFeatureQueryOptions();
  rendered_options.set_layer_ids_utf8({ id, other });
  var source_options = new MaplibreNative.SourceFeatureQueryOptions();
  source_options.set_source_layer_ids_utf8({ id, other });
  session.query_source_features_utf8(id, source_options);
  session.query_feature_extensions_utf8(id, feature, other, other, json);
}

void compile_texture_backend_wrappers() throws MaplibreNative.Error {
  var pointer = MaplibreNative.NativePointer.borrowed(1);
  assert(pointer.equal(pointer.copy()));
  assert(!pointer.equal(MaplibreNative.NativePointer.borrowed(2)));
  var physical_size = MaplibreNative.render_target_extent_physical_size(16, 8, 2.0);
  assert(physical_size.equal(physical_size.copy()));
  var metal_owned = new MaplibreNative.MetalOwnedTextureDescriptor(pointer);
  assert(metal_owned.equal(metal_owned.copy()));
  assert(metal_owned.width == 256);
  assert(metal_owned.height == 256);
  var metal_borrowed = new MaplibreNative.MetalBorrowedTextureDescriptor(pointer);
  assert(metal_borrowed.equal(metal_borrowed.copy()));
  assert(metal_borrowed.width == 256);
  assert(metal_borrowed.height == 256);
  assert(metal_borrowed.physical_width == 256);
  assert(metal_borrowed.physical_height == 256);
  metal_borrowed.width = 16;
  metal_borrowed.height = 16;
  metal_borrowed.scale_factor = 1.0;
  var vulkan_context = new MaplibreNative.VulkanContextDescriptor(pointer, pointer, pointer, pointer, 0);
  assert(vulkan_context.equal(vulkan_context.copy()));
  vulkan_context.get_instance_proc_addr = pointer;
  vulkan_context.get_device_proc_addr = pointer;
  assert(vulkan_context.equal(vulkan_context.copy()));
  var egl_context = new MaplibreNative.EglContextDescriptor(pointer, pointer, pointer);
  assert(egl_context.equal(egl_context.copy()));
  egl_context.get_proc_address = pointer;
  assert(egl_context.equal(egl_context.copy()));
  var wgl_context = new MaplibreNative.WglContextDescriptor(pointer, pointer);
  assert(wgl_context.equal(wgl_context.copy()));
  wgl_context.get_proc_address = pointer;
  assert(wgl_context.equal(wgl_context.copy()));
  assert(!wgl_context.equal(egl_context));
  var opengl_owned = new MaplibreNative.OpenGLOwnedTextureDescriptor(egl_context);
  assert(opengl_owned.equal(opengl_owned.copy()));
  var opengl_owned_changed = opengl_owned.copy();
  ((MaplibreNative.EglContextDescriptor) opengl_owned_changed.context).share_context = MaplibreNative.NativePointer.borrowed(2);
  assert(!opengl_owned.equal(opengl_owned_changed));
  assert(opengl_owned.width == 256);
  assert(opengl_owned.height == 256);
  opengl_owned.width = 16;
  opengl_owned.height = 16;
  opengl_owned.scale_factor = 1.0;
  var opengl_borrowed = new MaplibreNative.OpenGLBorrowedTextureDescriptor(wgl_context, 1, 0x0DE1);
  assert(opengl_borrowed.equal(opengl_borrowed.copy()));
  assert(opengl_borrowed.width == 256);
  assert(opengl_borrowed.height == 256);
  assert(opengl_borrowed.physical_width == 256);
  assert(opengl_borrowed.physical_height == 256);
  opengl_borrowed.width = 16;
  opengl_borrowed.height = 16;
  opengl_borrowed.scale_factor = 1.0;
  var vulkan_owned = new MaplibreNative.VulkanOwnedTextureDescriptor(vulkan_context);
  assert(vulkan_owned.equal(vulkan_owned.copy()));
  var vulkan_owned_changed = vulkan_owned.copy();
  vulkan_owned_changed.context.graphics_queue_family_index = 1;
  assert(!vulkan_owned.equal(vulkan_owned_changed));
  assert(vulkan_owned.width == 256);
  assert(vulkan_owned.height == 256);
  vulkan_owned.width = 16;
  vulkan_owned.height = 16;
  vulkan_owned.scale_factor = 1.0;
  var vulkan_borrowed = new MaplibreNative.VulkanBorrowedTextureDescriptor(vulkan_context, pointer, pointer);
  assert(vulkan_borrowed.final_layout == 5);
  assert(vulkan_borrowed.equal(vulkan_borrowed.copy()));
  assert(vulkan_borrowed.width == 256);
  assert(vulkan_borrowed.height == 256);
  assert(vulkan_borrowed.physical_width == 256);
  assert(vulkan_borrowed.physical_height == 256);
  vulkan_borrowed.format = 37;
  vulkan_borrowed.initial_layout = 0;
  vulkan_borrowed.final_layout = 1;
  var metal_surface = new MaplibreNative.MetalSurfaceDescriptor(pointer);
  assert(metal_surface.equal(metal_surface.copy()));
  assert(metal_surface.width == 256);
  assert(metal_surface.height == 256);
  metal_surface.device = pointer;
  var opengl_surface = new MaplibreNative.OpenGLSurfaceDescriptor(egl_context, pointer);
  assert(opengl_surface.equal(opengl_surface.copy()));
  assert(opengl_surface.width == 256);
  assert(opengl_surface.height == 256);
  opengl_surface.width = 16;
  opengl_surface.height = 16;
  opengl_surface.scale_factor = 1.0;
  var vulkan_surface = new MaplibreNative.VulkanSurfaceDescriptor(vulkan_context, pointer);
  assert(vulkan_surface.equal(vulkan_surface.copy()));
  assert(vulkan_surface.width == 256);
  assert(vulkan_surface.height == 256);
  vulkan_surface.width = 16;
  vulkan_surface.height = 16;
  vulkan_surface.scale_factor = 1.0;
}

int main() {
  try {
    assert(MaplibreNative.c_version() == 0);
    exercise_frame_access_state();
    exercise_unknown_feature_identifier_rejection();
    exercise_json_null_rejection();
    exercise_option_value_semantics();
    var backends = MaplibreNative.supported_render_backends();
    assert(backends != 0);
    MaplibreNative.opengl_supported_context_providers();

    MaplibreNative.set_log_async_severity_mask(MaplibreNative.LogSeverityMask.ALL);
    MaplibreNative.set_log_async_severity_mask(MaplibreNative.LogSeverityMask.DEFAULT);

    var future_network_status = MaplibreNative.network_status_from_raw(99);
    assert((uint32) future_network_status == 99);
    assert(MaplibreNative.resource_provider_decision_to_raw((MaplibreNative.ResourceProviderDecision) 99) == 99);
    bool future_status_failed = false;
    try {
      MaplibreNative.check_status((MaplibreNative.Raw.Status) (-9999));
    } catch (MaplibreNative.Error.UNKNOWN_STATUS error) {
      future_status_failed = true;
    }
    assert(future_status_failed);
    assert(MaplibreNative.thread_last_unknown_status() == -9999);
    try {
      MaplibreNative.check_status((MaplibreNative.Raw.Status) (-9998));
      assert_not_reached();
    } catch (MaplibreNative.Error.UNKNOWN_STATUS error) {
      assert(error.message != "");
    }
    try {
      MaplibreNative.NativePointer.borrowed(0).to_native();
      assert_not_reached();
    } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
      assert(error.message != "");
    }
    assert(MaplibreNative.thread_last_unknown_status() == null);
    bool future_network_status_rejected_by_native = false;
    try {
      MaplibreNative.set_network_status(future_network_status);
    } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
      future_network_status_rejected_by_native = error.message == "network status is invalid";
    }
    assert(future_network_status_rejected_by_native);

    var original_status = MaplibreNative.network_status();
    MaplibreNative.set_network_status(MaplibreNative.NetworkStatus.OFFLINE);
    assert(MaplibreNative.network_status() == MaplibreNative.NetworkStatus.OFFLINE);
    MaplibreNative.set_network_status(original_status);

    string? offline_merge_database_path = null;
    if (GLib.Environment.get_variable("MLN_VALA_RUN_OFFLINE_REGION_SMOKE") == "1") {
      offline_merge_database_path = create_offline_merge_database();
    }

    var runtime_options = new MaplibreNative.RuntimeOptions();
    runtime_options.maximum_cache_size = 1024 * 1024;
    var runtime = new MaplibreNative.RuntimeHandle(runtime_options);
    exercise_defensive_byte_snapshots(runtime);
    runtime.set_resource_provider(provide_resource);
    runtime.set_resource_provider(provide_resource);
    runtime.run_once();
    assert(runtime.poll_event() == null);

    var map_options = new MaplibreNative.MapOptions();
    assert(map_options.width == 256 && map_options.height == 256);
    map_options.width = 128;
    map_options.height = 64;
    map_options.scale_factor = 1.0;
    var map = new MaplibreNative.MapHandle(runtime, map_options);
    var empty_rendered_query_options = new MaplibreNative.RenderedFeatureQueryOptions();
    empty_rendered_query_options.set_layer_ids({});
    var empty_rendered_query_native = empty_rendered_query_options.to_native();
    assert((empty_rendered_query_native.fields & (uint32) MaplibreNative.Raw.RenderedFeatureQueryOptionField.LAYER_IDS) != 0);
    assert(empty_rendered_query_native.layer_id_count == 0);

    if ((backends & MaplibreNative.RenderBackendFlags.OPENGL) == 0) {
      var pointer = MaplibreNative.NativePointer.borrowed(1);
      var egl_context = new MaplibreNative.EglContextDescriptor(pointer, pointer, pointer);
      var opengl_texture = new MaplibreNative.OpenGLOwnedTextureDescriptor(egl_context);
      bool opengl_unsupported = false;
      try {
        map.attach_opengl_owned_texture(opengl_texture);
      } catch (MaplibreNative.Error error) {
        opengl_unsupported = true;
      }
      assert(opengl_unsupported);
    }

    bool close_runtime_with_live_map_failed = false;
    try {
      runtime.close();
    } catch (MaplibreNative.Error error) {
      close_runtime_with_live_map_failed = true;
    }
    assert(close_runtime_with_live_map_failed);

    bool provider_replacement_with_live_map_failed = false;
    try {
      runtime.set_resource_provider(provide_resource);
    } catch (MaplibreNative.Error error) {
      provider_replacement_with_live_map_failed = true;
    }
    assert(provider_replacement_with_live_map_failed);
    runtime.set_resource_transform(transform_resource);

    map.request_repaint();
    bool still_image_failed_for_continuous_map = false;
    try {
      map.request_still_image();
    } catch (MaplibreNative.Error error) {
      still_image_failed_for_continuous_map = error.message.length > 0;
    }
    assert(still_image_failed_for_continuous_map);
    map.set_style_url("http://maplibre-vala.invalid/style.json");
    for (uint attempt = 0; attempt < 1000 && read_callback_count(ref resource_transform_count) == 0; attempt++) {
      runtime.run_once();
      runtime.poll_event();
      GLib.Thread.usleep(1000);
    }
    assert(read_callback_count(ref resource_transform_count) > 0);
    runtime.clear_resource_transform();
    var provider_count_before_pass_through = read_callback_count(ref resource_provider_request_count);
    map.set_style_url("custom://pass-through-style.json");
    for (uint attempt = 0; attempt < 1000 && read_callback_count(ref resource_provider_request_count) == provider_count_before_pass_through; attempt++) {
      runtime.run_once();
      runtime.poll_event();
      GLib.Thread.usleep(1000);
    }
    assert(read_callback_count(ref resource_provider_request_count) > provider_count_before_pass_through);
    map.set_style_url("custom://style.json");
    assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_STYLE_LOADED, 128));
    assert(read_callback_count(ref resource_provider_request_count) > 0);
    assert(read_callback_count(ref resource_provider_one_shot_error_count) >= 2);
    map.set_style_url("custom://async-style.json");
    assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_STYLE_LOADED, 128));
    assert(read_callback_count(ref resource_provider_async_complete_count) > 0);
    if (GLib.Environment.get_variable("MLN_VALA_RUN_STYLE_URL_SMOKE") == "1") {
      map.set_style_url("maplibre://styles/vala-smoke");
    }
    map.set_style_json("{\"version\":8,\"sources\":{},\"layers\":[]}");
    map.set_debug_options(MaplibreNative.MapDebugOptions.TILE_BORDERS | MaplibreNative.MapDebugOptions.PARSE_STATUS);
    assert((map.get_debug_options() & MaplibreNative.MapDebugOptions.TILE_BORDERS) != 0);
    map.set_debug_options(MaplibreNative.MapDebugOptions.NONE);
    assert(map.get_debug_options() == MaplibreNative.MapDebugOptions.NONE);
    map.set_rendering_stats_view_enabled(false);
    assert(!map.get_rendering_stats_view_enabled());
    var viewport_options = new MaplibreNative.MapViewportOptions();
    viewport_options.set_north_orientation(MaplibreNative.NorthOrientation.UP);
    viewport_options.set_constrain_mode(MaplibreNative.ConstrainMode.HEIGHT_ONLY);
    viewport_options.set_viewport_mode(MaplibreNative.ViewportMode.DEFAULT);
    viewport_options.set_frustum_offset(MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
    map.set_viewport_options(viewport_options);
    var copied_viewport_options = map.get_viewport_options();
    MaplibreNative.NorthOrientation north_orientation;
    assert(copied_viewport_options.get_north_orientation(out north_orientation) && north_orientation == MaplibreNative.NorthOrientation.UP);
    var tile_options = new MaplibreNative.MapTileOptions();
    tile_options.set_prefetch_zoom_delta(1);
    tile_options.set_lod_mode(MaplibreNative.TileLodMode.DEFAULT);
    map.set_tile_options(tile_options);
    var copied_tile_options = map.get_tile_options();
    uint32 prefetch_zoom_delta;
    assert(copied_tile_options.get_prefetch_zoom_delta(out prefetch_zoom_delta) && prefetch_zoom_delta == 1);
    map.is_fully_loaded();
    MaplibreNative.set_log_callback(handle_log);
    bool parse_error_mapped = false;
    try {
      map.set_style_json("{");
    } catch (MaplibreNative.Error error) {
      parse_error_mapped = error.message.length > 0;
    }
    assert(parse_error_mapped);
    assert(read_callback_count(ref log_count) > 0);
    MaplibreNative.clear_log_callback();
    map.set_style_json("{\"version\":8,\"sources\":{},\"layers\":[]}");

    bool invalid_argument_mapped = false;
    try {
      map.add_geojson_source_url("", "https://example.invalid/points.geojson");
    } catch (MaplibreNative.Error.INVALID_ARGUMENT error) {
      invalid_argument_mapped = error.message.length > 0;
    }
    assert(invalid_argument_mapped);

    uint8[] image_pixels = { 255, 0, 0, 255 };
    var image = new MaplibreNative.PremultipliedRgba8Image(1, 1, 4, image_pixels);
    var image_options = new MaplibreNative.StyleImageOptions();
    image_options.pixel_ratio = 1.0f;
    image_options.sdf = false;
    map.set_style_image("marker", image, image_options);
    assert(map.style_image_exists("marker"));
    var image_info = map.get_style_image_info("marker");
    assert(image_info != null && image_info.width == 1 && image_info.height == 1);
    assert(image_info.equal(image_info.copy()));
    var copied_image = map.copy_style_image_premultiplied_rgba8("marker");
    assert(copied_image != null && copied_image.length == 4 && copied_image[0] == 255);
    assert(map.remove_style_image("marker"));
    assert(!map.remove_style_image("marker"));

    var camera = new MaplibreNative.CameraOptions();
    camera.set_center(MaplibreNative.LatLng(0.0, 0.0));
    camera.set_center_altitude(0.0);
    camera.set_padding(MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
    camera.set_anchor(MaplibreNative.ScreenPoint(0.0, 0.0));
    camera.set_zoom(1.0);
    camera.set_bearing(0.0);
    camera.set_pitch(0.0);
    camera.set_roll(0.0);
    camera.set_field_of_view(0.6435011087932844);
    var camera_copy = camera.copy();
    assert(camera.equal(camera_copy));
    camera_copy.set_zoom(2.0);
    assert(!camera.equal(camera_copy));
    map.jump_to(camera);
    var animation = new MaplibreNative.AnimationOptions();
    animation.set_duration_ms(0.0);
    animation.set_easing(MaplibreNative.UnitBezier(0.0, 0.0, 1.0, 1.0));
    map.ease_to(camera, animation);
    map.fly_to(camera, animation);
    map.move_by(0.0, 0.0);
    map.move_by_animated(0.0, 0.0, animation);
    map.scale_by(1.0);
    map.scale_by_at(1.0, MaplibreNative.ScreenPoint(0.0, 0.0));
    map.scale_by_animated(1.0, animation);
    map.scale_by_at_animated(1.0, MaplibreNative.ScreenPoint(0.0, 0.0), animation);
    map.rotate_by(MaplibreNative.ScreenPoint(0.0, 0.0), MaplibreNative.ScreenPoint(0.0, 0.0));
    map.rotate_by_animated(MaplibreNative.ScreenPoint(0.0, 0.0), MaplibreNative.ScreenPoint(0.0, 0.0), animation);
    map.pitch_by(0.0);
    map.pitch_by_animated(0.0, animation);
    map.cancel_transitions();
    var copied_camera = map.get_camera();
    double zoom;
    copied_camera.get_zoom(out zoom);
    double field_of_view;
    copied_camera.get_field_of_view(out field_of_view);
    var fit_options = new MaplibreNative.CameraFitOptions();
    fit_options.set_padding(MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
    var fit_bounds = MaplibreNative.LatLngBounds(MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0));
    map.camera_for_lat_lng_bounds(fit_bounds, fit_options);
    map.camera_for_lat_lngs({ MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0) }, fit_options);
    map.camera_for_geometry(MaplibreNative.Geometry.point(MaplibreNative.LatLng(0.0, 0.0)), fit_options);
    var line_geometry = MaplibreNative.Geometry.line_string({ MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0) });
    assert(line_geometry.geometry_type == MaplibreNative.GeometryType.LINE_STRING);
    assert(line_geometry.get_coordinates().length == 2);
    var point_geometry = MaplibreNative.Geometry.point(MaplibreNative.LatLng(2.0, 3.0));
    assert(point_geometry.get_point().latitude == 2.0);
    map.camera_for_geometry(line_geometry, fit_options);
    var geometry_line = new MaplibreNative.CoordinateList({ MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0) });
    var geometry_ring = new MaplibreNative.CoordinateList({
      MaplibreNative.LatLng(-1.0, -1.0),
      MaplibreNative.LatLng(-1.0, 1.0),
      MaplibreNative.LatLng(1.0, 1.0),
      MaplibreNative.LatLng(1.0, -1.0),
      MaplibreNative.LatLng(-1.0, -1.0)
    });
    var polygon = new MaplibreNative.Polygon({ geometry_ring });
    assert(polygon.get_rings().length == 1);
    assert(polygon.get_rings()[0].to_array().length == 5);
    map.camera_for_geometry(MaplibreNative.Geometry.polygon(polygon), fit_options);
    map.camera_for_geometry(MaplibreNative.Geometry.multi_line_string({ geometry_line }), fit_options);
    map.camera_for_geometry(MaplibreNative.Geometry.multi_polygon({ polygon, polygon }), fit_options);
    map.camera_for_geometry(MaplibreNative.Geometry.geometry_collection({ MaplibreNative.Geometry.point(MaplibreNative.LatLng(0.0, 0.0)), line_geometry }), fit_options);
    var shared_collection = MaplibreNative.Geometry.geometry_collection({ line_geometry, MaplibreNative.Geometry.multi_polygon({ polygon }) });
    map.camera_for_geometry(MaplibreNative.Geometry.geometry_collection({ shared_collection, shared_collection }), fit_options);
    var empty_line = MaplibreNative.Geometry.line_string({});
    var empty_polygon = MaplibreNative.Geometry.polygon(new MaplibreNative.Polygon({}));
    var empty_multi_point = MaplibreNative.Geometry.multi_point({});
    var empty_multi_line = MaplibreNative.Geometry.multi_line_string({});
    var empty_multi_polygon = MaplibreNative.Geometry.multi_polygon({});
    var empty_collection = MaplibreNative.Geometry.geometry_collection({});
    assert(empty_line.geometry_type == MaplibreNative.GeometryType.LINE_STRING);
    assert(empty_polygon.geometry_type == MaplibreNative.GeometryType.POLYGON);
    assert(empty_multi_point.geometry_type == MaplibreNative.GeometryType.MULTI_POINT);
    assert(empty_multi_line.geometry_type == MaplibreNative.GeometryType.MULTI_LINE_STRING);
    assert(empty_multi_polygon.geometry_type == MaplibreNative.GeometryType.MULTI_POLYGON);
    assert(empty_collection.geometry_type == MaplibreNative.GeometryType.GEOMETRY_COLLECTION);
    assert(empty_line.get_coordinates().length == 0);
    assert(empty_polygon.get_polygon().get_rings().length == 0);
    assert(empty_multi_point.get_coordinates().length == 0);
    assert(empty_multi_line.get_lines().length == 0);
    assert(empty_multi_polygon.get_polygons().length == 0);
    assert(empty_collection.get_geometries().length == 0);
    map.lat_lng_bounds_for_camera(camera);
    map.lat_lng_bounds_for_camera_unwrapped(camera);
    var bound_options = new MaplibreNative.BoundOptions();
    bound_options.set_bounds(MaplibreNative.LatLngBounds(MaplibreNative.LatLng(-80.0, -180.0), MaplibreNative.LatLng(80.0, 180.0)));
    bound_options.set_min_zoom(0.0);
    bound_options.set_max_zoom(22.0);
    map.set_bounds(bound_options);
    var copied_bounds = map.get_bounds();
    MaplibreNative.LatLngBounds constrained_bounds;
    assert(copied_bounds.get_bounds(out constrained_bounds));
    var free_camera = map.get_free_camera_options();
    map.set_free_camera_options(free_camera);
    var projection_mode = map.get_projection_mode();
    map.set_projection_mode(projection_mode);
    var map_pixel = map.pixel_for_lat_lng(MaplibreNative.LatLng(0.0, 0.0));
    var map_coordinate = map.lat_lng_for_pixel(map_pixel);
    assert(map_coordinate.latitude > -90.0 && map_coordinate.latitude < 90.0);
    var map_pixels = map.pixels_for_lat_lngs({ MaplibreNative.LatLng(0.0, 0.0), MaplibreNative.LatLng(1.0, 1.0) });
    assert(map_pixels.length == 2);
    var map_coordinates = map.lat_lngs_for_pixels(map_pixels);
    assert(map_coordinates.length == 2);

    var inline_geojson = MaplibreNative.GeoJson.geometry(MaplibreNative.Geometry.point(MaplibreNative.LatLng(0.0, 0.0)));
    assert(inline_geojson.equal(inline_geojson.copy()));
    map.add_geojson_source_data("inline-point", inline_geojson);
    assert(map.style_source_exists("inline-point"));
    map.set_geojson_source_data("inline-point", inline_geojson);
    assert(map.remove_style_source("inline-point"));

    var nul_source_id = new MaplibreNative.Utf8String.from_bytes({ 'n', 0, 's' });
    map.add_geojson_source_data_utf8(nul_source_id, inline_geojson);
    assert(map.style_source_exists_utf8(nul_source_id));
    assert(map.get_style_source_type_utf8(nul_source_id) == MaplibreNative.StyleSourceType.GEOJSON);
    var source_ids_with_nul = map.list_style_source_ids();
    assert(source_ids_with_nul.contains_utf8(nul_source_id));
    assert(source_ids_with_nul.to_utf8_array().length == source_ids_with_nul.length);
    for (uint source_index = 0; source_index < source_ids_with_nul.length; source_index++) {
      var listed_source_id = source_ids_with_nul.get_utf8(source_index);
      if (listed_source_id.equal(nul_source_id)) {
        assert(listed_source_id.to_bytes()[1] == 0);
      }
    }
    assert(map.remove_style_source_utf8(nul_source_id));

    var shared_json_subtree = MaplibreNative.JsonValue.array_value({ MaplibreNative.JsonValue.string_value("shared") });
    var repeated_json_subtree = MaplibreNative.JsonValue.array_value({ shared_json_subtree, shared_json_subtree });
    var feature = new MaplibreNative.Feature(
      MaplibreNative.Geometry.point(MaplibreNative.LatLng(0.0, 0.0)),
      {
        new MaplibreNative.JsonMember("name", MaplibreNative.JsonValue.string_value("test point")),
        new MaplibreNative.JsonMember("visible", MaplibreNative.JsonValue.bool_value(true)),
        new MaplibreNative.JsonMember("rank", MaplibreNative.JsonValue.int_value(1)),
        new MaplibreNative.JsonMember("tags", MaplibreNative.JsonValue.array_value({ MaplibreNative.JsonValue.string_value("a"), MaplibreNative.JsonValue.string_value("b") })),
        new MaplibreNative.JsonMember("metadata", MaplibreNative.JsonValue.object_value({ new MaplibreNative.JsonMember("source", MaplibreNative.JsonValue.string_value("vala")) })),
        new MaplibreNative.JsonMember("repeated", repeated_json_subtree),
        new MaplibreNative.JsonMember("shared-a", shared_json_subtree),
        new MaplibreNative.JsonMember("shared-b", shared_json_subtree)
      },
      MaplibreNative.FeatureIdentifier.string_value("feature-1"));
    assert(feature.feature_identifier.get_string() == "feature-1");
    var feature_copy = feature.copy();
    assert(feature.equal(feature_copy));
    var feature_changed = new MaplibreNative.Feature(
      MaplibreNative.Geometry.point(MaplibreNative.LatLng(1.0, 0.0)),
      feature_copy.property_members,
      feature_copy.feature_identifier);
    assert(!feature.equal(feature_changed));
    var coordinate_list = new MaplibreNative.CoordinateList({
      MaplibreNative.LatLng(0.0, 0.0),
      MaplibreNative.LatLng(1.0, 1.0)
    });
    assert(coordinate_list.equal(coordinate_list.copy()));
    var value_polygon = new MaplibreNative.Polygon({ coordinate_list });
    assert(value_polygon.equal(value_polygon.copy()));
    var value_geometry = MaplibreNative.Geometry.geometry_collection({
      MaplibreNative.Geometry.line_string(coordinate_list.to_array()),
      MaplibreNative.Geometry.polygon(value_polygon)
    });
    assert(value_geometry.equal(value_geometry.copy()));
    var feature_collection = new MaplibreNative.FeatureCollection({ feature, feature_copy });
    assert(feature_collection.equal(feature_collection.copy()));

    uint8[] embedded_nul = { 'a', 0, 'b' };
    var nul_json = MaplibreNative.JsonValue.string_utf8_bytes(embedded_nul);
    embedded_nul[0] = 'z';
    var copied_nul_bytes = nul_json.get_string_utf8_bytes();
    assert(copied_nul_bytes.length == 3 && copied_nul_bytes[0] == 'a' && copied_nul_bytes[1] == 0 && copied_nul_bytes[2] == 'b');
    bool nul_string_accessor_failed = false;
    try {
      nul_json.get_string();
    } catch (MaplibreNative.Error.INVALID_STATE error) {
      nul_string_accessor_failed = true;
    }
    assert(nul_string_accessor_failed);
    var nul_member = new MaplibreNative.JsonMember.from_utf8_bytes({ 'k', 0, 'y' }, nul_json);
    var nul_object = MaplibreNative.JsonValue.object_value({ nul_member });
    var nul_object_copy = MaplibreNative.JsonValue.from_native(nul_object.to_native());
    assert(nul_object.equal(nul_object_copy));
    assert(nul_object_copy.get_object_members()[0].get_key_utf8_bytes()[1] == 0);
    var nul_identifier = MaplibreNative.FeatureIdentifier.string_utf8_bytes({ 'i', 0, 'd' });
    MaplibreNative.Raw.Feature nul_identifier_native = {};
    nul_identifier.apply_to_native(ref nul_identifier_native);
    var nul_identifier_copy = MaplibreNative.FeatureIdentifier.from_native(nul_identifier_native);
    assert(nul_identifier.equal(nul_identifier_copy));
    assert(nul_identifier_copy.get_string_utf8_bytes()[1] == 0);

    MaplibreNative.JsonValue[] mutable_json_values = { MaplibreNative.JsonValue.string_value("kept") };
    var copied_json_array = MaplibreNative.JsonValue.array_value(mutable_json_values);
    mutable_json_values[0] = MaplibreNative.JsonValue.string_value("changed");
    assert(copied_json_array.get_array_values()[0].get_string() == "kept");
    var returned_json_values = copied_json_array.get_array_values();
    returned_json_values[0] = MaplibreNative.JsonValue.string_value("changed again");
    assert(copied_json_array.get_array_values()[0].get_string() == "kept");
    var feature_geojson = MaplibreNative.GeoJson.feature(feature);
    assert(feature_geojson.equal(feature_geojson.copy()));
    map.add_geojson_source_data("inline-feature", feature_geojson);
    assert(map.style_source_exists("inline-feature"));
    map.set_geojson_source_data("inline-feature", feature_geojson);
    assert(map.remove_style_source("inline-feature"));

    var feature_collection_geojson = MaplibreNative.GeoJson.feature_collection(feature_collection);
    assert(feature_collection_geojson.equal(feature_collection_geojson.copy()));
    map.add_geojson_source_data("inline-feature-collection", feature_collection_geojson);
    assert(map.style_source_exists("inline-feature-collection"));
    map.set_geojson_source_data("inline-feature-collection", feature_collection_geojson);
    assert(map.remove_style_source("inline-feature-collection"));

    map.add_geojson_source_data("state-source", feature_geojson);
    assert(map.style_source_exists("state-source"));

    var custom_options = new MaplibreNative.CustomGeometrySourceOptions(fetch_custom_geometry_tile, cancel_custom_geometry_tile);
    custom_options.min_zoom = 0.0;
    custom_options.max_zoom = 22.0;
    custom_options.tolerance = 0.375;
    custom_options.tile_size = 512;
    custom_options.buffer = 128;
    custom_options.clip = true;
    custom_options.wrap = false;
    var nul_custom_source = new MaplibreNative.Utf8String.from_bytes({ 'c', 0, 'g' });
    map.add_custom_geometry_source_utf8(nul_custom_source, custom_options);
    assert(map.style_source_exists_utf8(nul_custom_source));
    assert(map.remove_style_source_utf8(nul_custom_source));
    map.add_custom_geometry_source("custom-geometry", custom_options);
    assert(map.style_source_exists("custom-geometry"));
    var custom_layer_json = MaplibreNative.JsonValue.object_value({
      new MaplibreNative.JsonMember("id", MaplibreNative.JsonValue.string_value("custom-geometry-circle")),
      new MaplibreNative.JsonMember("type", MaplibreNative.JsonValue.string_value("circle")),
      new MaplibreNative.JsonMember("source", MaplibreNative.JsonValue.string_value("custom-geometry"))
    });
    map.add_style_layer_json(custom_layer_json);
    var custom_tile = MaplibreNative.CanonicalTileId(0, 0, 0);
    map.set_custom_geometry_source_tile_data("custom-geometry", custom_tile, feature_geojson);
    map.invalidate_custom_geometry_source_tile("custom-geometry", custom_tile);
    map.invalidate_custom_geometry_source_region("custom-geometry", MaplibreNative.LatLngBounds(MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0)));

    map.add_geojson_source_url("points", "https://example.invalid/points.geojson");
    assert(map.style_source_exists("points"));
    assert(map.get_style_source_type("points") == MaplibreNative.StyleSourceType.GEOJSON);
    var source_info = map.get_style_source_info("points");
    assert(source_info != null && source_info.source_type == MaplibreNative.StyleSourceType.GEOJSON && source_info.id_byte_length == "points".length);
    assert(source_info.equal(source_info.copy()));
    assert(map.copy_style_source_attribution("points") == null);
    assert(map.list_style_source_ids().contains("points"));
    map.set_geojson_source_url("points", "https://example.invalid/updated.geojson");
    assert(map.remove_style_source("points"));
    assert(!map.remove_style_source("points"));
    assert(map.get_style_source_type("points") == null);

    var source_json = MaplibreNative.JsonValue.object_value({
      new MaplibreNative.JsonMember("type", MaplibreNative.JsonValue.string_value("geojson")),
      new MaplibreNative.JsonMember("data", MaplibreNative.JsonValue.object_value({
        new MaplibreNative.JsonMember("type", MaplibreNative.JsonValue.string_value("FeatureCollection")),
        new MaplibreNative.JsonMember("features", MaplibreNative.JsonValue.array_value({}))
      }))
    });
    map.add_style_source_json("json-source", source_json);
    assert(map.style_source_exists("json-source"));
    assert(map.get_style_source_type("json-source") == MaplibreNative.StyleSourceType.GEOJSON);
    assert(map.remove_style_source("json-source"));

    var vector_source_options = new MaplibreNative.StyleTileSourceOptions();
    vector_source_options.min_zoom = 0.0;
    vector_source_options.max_zoom = 22.0;
    vector_source_options.attribution = "test attribution";
    vector_source_options.scheme = MaplibreNative.StyleTileScheme.XYZ;
    vector_source_options.bounds = MaplibreNative.LatLngBounds(MaplibreNative.LatLng(-10.0, -10.0), MaplibreNative.LatLng(10.0, 10.0));
    vector_source_options.vector_encoding = MaplibreNative.StyleVectorTileEncoding.MVT;
    var raster_source_options = new MaplibreNative.StyleTileSourceOptions();
    raster_source_options.tile_size = 256;
    var dem_source_options = new MaplibreNative.StyleTileSourceOptions();
    dem_source_options.tile_size = 256;
    dem_source_options.raster_encoding = MaplibreNative.StyleRasterDemEncoding.MAPBOX;

    var nul_attribution = new MaplibreNative.Utf8String.from_bytes({ 'a', 0, 't' });
    var nul_attribution_options = new MaplibreNative.StyleTileSourceOptions();
    nul_attribution_options.set_attribution_utf8(nul_attribution);
    var nul_attribution_source = new MaplibreNative.Utf8String("vector-nul-attribution");
    map.add_vector_source_tiles_utf8(
      nul_attribution_source,
      { new MaplibreNative.Utf8String("https://example.invalid/{z}/{x}/{y}.pbf") },
      nul_attribution_options);
    assert(map.copy_style_source_attribution_utf8(nul_attribution_source).equal(nul_attribution));
    assert(map.remove_style_source_utf8(nul_attribution_source));

    map.add_vector_source_url("vector-url", "https://example.invalid/vector.json", vector_source_options);
    assert(map.get_style_source_type("vector-url") == MaplibreNative.StyleSourceType.VECTOR);
    assert(map.remove_style_source("vector-url"));

    map.add_vector_source_tiles("vector-tiles", { "https://example.invalid/{z}/{x}/{y}.pbf" }, vector_source_options);
    assert(map.get_style_source_type("vector-tiles") == MaplibreNative.StyleSourceType.VECTOR);
    assert(map.remove_style_source("vector-tiles"));

    map.add_raster_source_url("raster-url", "https://example.invalid/raster.json", raster_source_options);
    assert(map.get_style_source_type("raster-url") == MaplibreNative.StyleSourceType.RASTER);
    assert(map.remove_style_source("raster-url"));

    map.add_raster_source_tiles("raster-tiles", { "https://example.invalid/{z}/{x}/{y}.png" }, raster_source_options);
    assert(map.get_style_source_type("raster-tiles") == MaplibreNative.StyleSourceType.RASTER);
    assert(map.remove_style_source("raster-tiles"));

    map.add_raster_dem_source_url("dem-url", "https://example.invalid/dem.json", dem_source_options);
    assert(map.get_style_source_type("dem-url") == MaplibreNative.StyleSourceType.RASTER_DEM);
    assert(map.remove_style_source("dem-url"));

    map.add_raster_dem_source_tiles("dem-tiles", { "https://example.invalid/{z}/{x}/{y}.png" }, dem_source_options);
    assert(map.get_style_source_type("dem-tiles") == MaplibreNative.StyleSourceType.RASTER_DEM);
    if (GLib.Environment.get_variable("MLN_VALA_RUN_DEM_LAYER_SMOKE") == "1") {
      map.add_hillshade_layer("hillshade-layer", "dem-tiles");
      assert(map.style_layer_exists("hillshade-layer"));
      map.add_color_relief_layer("relief-layer", "dem-tiles");
      assert(map.style_layer_exists("relief-layer"));
      assert(map.remove_style_layer("hillshade-layer"));
      assert(map.remove_style_layer("relief-layer"));
    }
    assert(map.remove_style_source("dem-tiles"));

    MaplibreNative.LatLng[] image_coordinates = {
      MaplibreNative.LatLng(1.0, -1.0),
      MaplibreNative.LatLng(1.0, 1.0),
      MaplibreNative.LatLng(-1.0, 1.0),
      MaplibreNative.LatLng(-1.0, -1.0)
    };
    MaplibreNative.LatLng[] updated_image_coordinates = {
      MaplibreNative.LatLng(2.0, -2.0),
      MaplibreNative.LatLng(2.0, 2.0),
      MaplibreNative.LatLng(-2.0, 2.0),
      MaplibreNative.LatLng(-2.0, -2.0)
    };
    map.add_image_source_url("url-image-source", image_coordinates, "https://example.invalid/image.png");
    assert(map.get_style_source_type("url-image-source") == MaplibreNative.StyleSourceType.IMAGE);
    map.set_image_source_url("url-image-source", "https://example.invalid/updated-image.png");
    assert(map.remove_style_source("url-image-source"));

    map.add_image_source_image("inline-image-source", image_coordinates, image);
    assert(map.get_style_source_type("inline-image-source") == MaplibreNative.StyleSourceType.IMAGE);
    var copied_image_coordinates = map.get_image_source_coordinates("inline-image-source");
    assert(copied_image_coordinates != null && copied_image_coordinates.length == 4 && copied_image_coordinates[0].latitude == 1.0);
    map.set_image_source_coordinates("inline-image-source", updated_image_coordinates);
    copied_image_coordinates = map.get_image_source_coordinates("inline-image-source");
    assert(copied_image_coordinates != null && copied_image_coordinates[0].latitude == 2.0);
    map.set_image_source_image("inline-image-source", image);
    assert(map.remove_style_source("inline-image-source"));

    var circle_layer_json = MaplibreNative.JsonValue.object_value({
      new MaplibreNative.JsonMember("id", MaplibreNative.JsonValue.string_value("state-circle")),
      new MaplibreNative.JsonMember("type", MaplibreNative.JsonValue.string_value("circle")),
      new MaplibreNative.JsonMember("source", MaplibreNative.JsonValue.string_value("state-source"))
    });
    map.add_style_layer_json(circle_layer_json);
    assert(map.style_layer_exists("state-circle"));
    assert(map.get_style_layer_type("state-circle") == "circle");
    var copied_layer_json = map.get_style_layer_json("state-circle");
    assert(copied_layer_json != null && copied_layer_json.value_type == MaplibreNative.JsonValueType.OBJECT);
    map.set_layer_property("state-circle", "circle-radius", MaplibreNative.JsonValue.double_value(6.0));
    var copied_layer_property = map.get_layer_property("state-circle", "circle-radius");
    assert(copied_layer_property != null);
    map.set_layer_filter("state-circle", MaplibreNative.JsonValue.array_value({
      MaplibreNative.JsonValue.string_value("has"),
      MaplibreNative.JsonValue.string_value("rank")
    }));
    var copied_layer_filter = map.get_layer_filter("state-circle");
    assert(copied_layer_filter != null && copied_layer_filter.value_type == MaplibreNative.JsonValueType.ARRAY);
    map.set_layer_filter("state-circle");
    map.get_layer_filter("state-circle");
    map.move_style_layer("state-circle");
    assert(map.remove_style_layer("state-circle"));
    if (GLib.Environment.get_variable("MLN_VALA_RUN_STYLE_LIGHT_SMOKE") == "1") {
      compile_style_light_wrappers(map);
    }

    map.add_location_indicator_layer("location");
    assert(map.style_layer_exists("location"));
    assert(map.list_style_layer_ids().contains("location"));
    if (GLib.Environment.get_variable("MLN_VALA_RUN_LOCATION_INDICATOR_PROPERTY_SMOKE") == "1") {
      compile_location_indicator_property_wrappers(map);
    }
    assert(map.remove_style_layer("location"));
    assert(!map.remove_style_layer("location"));

    var operation_id = runtime.run_ambient_cache_operation_start(MaplibreNative.AmbientCacheOperation.INVALIDATE);
    runtime.discard_offline_operation(operation_id);
    operation_id.close();
    operation_id.close();
    compile_offline_region_wrappers(runtime, offline_merge_database_path);

    var projection = map.create_projection();
    var pixel = projection.pixel_for_lat_lng(MaplibreNative.LatLng(0.0, 0.0));
    var round_trip = projection.lat_lng_for_pixel(pixel);
    assert(round_trip.latitude > -90.0 && round_trip.latitude < 90.0);
    projection.set_camera(camera);
    projection.set_visible_coordinates({ MaplibreNative.LatLng(-1.0, -1.0), MaplibreNative.LatLng(1.0, 1.0) }, MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
    projection.set_visible_geometry(line_geometry, MaplibreNative.EdgeInsets(0.0, 0.0, 0.0, 0.0));
    exercise_projection_close_race(projection);
    bool closed_projection_failed = false;
    try {
      projection.get_camera();
    } catch (MaplibreNative.Error error) {
      closed_projection_failed = true;
    }
    assert(closed_projection_failed);

    var meters = MaplibreNative.projected_meters_for_lat_lng(MaplibreNative.LatLng(0.0, 0.0));
    meters.northing.to_string();
    meters.easting.to_string();
    var meters_coordinate = MaplibreNative.lat_lng_for_projected_meters(meters);
    assert(meters_coordinate.latitude > -1.0 && meters_coordinate.latitude < 1.0);
    var physical_size = MaplibreNative.render_target_extent_physical_size(65, 33, 1.5);
    assert(physical_size.width == 98);
    assert(physical_size.height == 50);

    if ((backends & MaplibreNative.RenderBackendFlags.VULKAN) != 0) {
      VulkanTestContext vulkan_context_storage;
      assert(vulkan_test_context_create(out vulkan_context_storage));
      try {
        var vulkan_context = new MaplibreNative.VulkanContextDescriptor(
          MaplibreNative.NativePointer.borrowed((size_t) vulkan_context_storage.instance),
          MaplibreNative.NativePointer.borrowed((size_t) vulkan_context_storage.physical_device),
          MaplibreNative.NativePointer.borrowed((size_t) vulkan_context_storage.device),
          MaplibreNative.NativePointer.borrowed((size_t) vulkan_context_storage.graphics_queue),
          vulkan_context_storage.graphics_queue_family_index);
        vulkan_context.get_instance_proc_addr = MaplibreNative.NativePointer.borrowed((size_t) vulkan_context_storage.get_instance_proc_addr);
        vulkan_context.get_device_proc_addr = MaplibreNative.NativePointer.borrowed((size_t) vulkan_context_storage.get_device_proc_addr);
        var vulkan_texture = new MaplibreNative.VulkanOwnedTextureDescriptor(vulkan_context);
        vulkan_texture.width = 32;
        vulkan_texture.height = 16;
        vulkan_texture.scale_factor = 1.0;
        var vulkan_session = map.attach_vulkan_owned_texture(vulkan_texture);
        vulkan_session.resize(32, 16, 1.0);
        assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE, 128));
        vulkan_session.render_update();
        assert(read_callback_count(ref custom_geometry_fetch_count) > 0);
        var vulkan_query_result = vulkan_session.query_rendered_features(MaplibreNative.RenderedQueryGeometry.point(MaplibreNative.ScreenPoint(0.0, 0.0)));
        vulkan_query_result.length.to_string();
        uint8[] vulkan_pixels = new uint8[32 * 16 * 4];
        var vulkan_info = vulkan_session.read_premultiplied_rgba8(vulkan_pixels);
        assert(vulkan_info.width == 32);
        assert(vulkan_info.height == 16);
        var vulkan_frame = vulkan_session.acquire_vulkan_owned_texture_frame();
        assert(vulkan_frame.get_width() == 32);
        var vulkan_frame_image = vulkan_frame.get_image();
        vulkan_frame.get_image_view();
        vulkan_frame.get_device();
        bool vulkan_render_while_acquired_failed = false;
        try {
          vulkan_session.render_update();
        } catch (MaplibreNative.Error error) {
          vulkan_render_while_acquired_failed = true;
        }
        assert(vulkan_render_while_acquired_failed);
        vulkan_frame.close();
        bool closed_vulkan_frame_image_failed = false;
        try {
          vulkan_frame_image.get_bits();
        } catch (MaplibreNative.Error error) {
          closed_vulkan_frame_image_failed = true;
        }
        assert(closed_vulkan_frame_image_failed);
        vulkan_session.detach();
        exercise_render_session_close_race(vulkan_session);

        VulkanBorrowedImage vulkan_borrowed_storage;
        assert(vulkan_test_borrowed_image_create(ref vulkan_context_storage, 32, 16, out vulkan_borrowed_storage));
        assert(vulkan_borrowed_storage.memory != null);
        try {
          var vulkan_borrowed_texture = new MaplibreNative.VulkanBorrowedTextureDescriptor(
            vulkan_context,
            MaplibreNative.NativePointer.borrowed((size_t) vulkan_borrowed_storage.image),
            MaplibreNative.NativePointer.borrowed((size_t) vulkan_borrowed_storage.image_view));
          vulkan_borrowed_texture.width = 32;
          vulkan_borrowed_texture.height = 16;
          vulkan_borrowed_texture.scale_factor = 1.0;
          vulkan_borrowed_texture.physical_width = 32;
          vulkan_borrowed_texture.physical_height = 16;
          vulkan_borrowed_texture.format = vulkan_borrowed_storage.format;
          vulkan_borrowed_texture.initial_layout = vulkan_borrowed_storage.initial_layout;
          vulkan_borrowed_texture.final_layout = vulkan_borrowed_storage.final_layout;
          var vulkan_borrowed_session = map.attach_vulkan_borrowed_texture(vulkan_borrowed_texture);
          map.request_repaint();
          assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE, 128));
          vulkan_borrowed_session.render_update();
          bool vulkan_borrowed_resize_failed = false;
          try {
            vulkan_borrowed_session.resize(16, 16, 1.0);
          } catch (MaplibreNative.Error error) {
            vulkan_borrowed_resize_failed = true;
          }
          assert(vulkan_borrowed_resize_failed);
          bool vulkan_borrowed_readback_failed = false;
          try {
            uint8[] borrowed_pixels = new uint8[32 * 16 * 4];
            vulkan_borrowed_session.read_premultiplied_rgba8(borrowed_pixels);
          } catch (MaplibreNative.Error error) {
            vulkan_borrowed_readback_failed = true;
          }
          assert(vulkan_borrowed_readback_failed);
          bool vulkan_borrowed_frame_failed = false;
          try {
            vulkan_borrowed_session.acquire_vulkan_owned_texture_frame();
          } catch (MaplibreNative.Error error) {
            vulkan_borrowed_frame_failed = true;
          }
          assert(vulkan_borrowed_frame_failed);
          vulkan_borrowed_session.detach();
          vulkan_borrowed_session.close();
        } finally {
          vulkan_test_borrowed_image_destroy(ref vulkan_context_storage, ref vulkan_borrowed_storage);
        }

        if (vulkan_test_surface_supported()) {
          MetalWindowLayer vulkan_surface_layer;
          assert(metal_test_window_layer_create(32, 16, out vulkan_surface_layer));
          assert(vulkan_surface_layer.window != null);
          void* vulkan_surface = null;
          try {
            assert(vulkan_test_surface_create(ref vulkan_context_storage, vulkan_surface_layer.layer, out vulkan_surface));
            var vulkan_surface_descriptor = new MaplibreNative.VulkanSurfaceDescriptor(vulkan_context, MaplibreNative.NativePointer.borrowed((size_t) vulkan_surface));
            vulkan_surface_descriptor.width = 32;
            vulkan_surface_descriptor.height = 16;
            vulkan_surface_descriptor.scale_factor = 1.0;
            var vulkan_surface_session = map.attach_vulkan_surface(vulkan_surface_descriptor);
            vulkan_surface_session.resize(32, 16, 1.0);
            vulkan_surface_session.detach();
            vulkan_surface_session.close();
          } finally {
            if (vulkan_surface != null) {
              vulkan_test_surface_destroy(ref vulkan_context_storage, vulkan_surface);
            }
            metal_test_window_layer_destroy(ref vulkan_surface_layer);
          }
        }
      } finally {
        vulkan_test_context_destroy(ref vulkan_context_storage);
      }
    }

    if ((backends & MaplibreNative.RenderBackendFlags.OPENGL) != 0) {
      assert(opengl_test_context_supported());
      OpenGLTestContext opengl_context_storage;
      assert(opengl_test_context_create(32, 16, out opengl_context_storage));
      assert(opengl_context_storage.surface != null);
      try {
        MaplibreNative.OpenGLContextDescriptor opengl_context;
        if (opengl_context_storage.platform == (uint32) MaplibreNative.OpenGLContextPlatform.WGL) {
          var wgl_context = new MaplibreNative.WglContextDescriptor(
            MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.display),
            MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.context));
          if (opengl_context_storage.get_proc_address != null) {
            wgl_context.get_proc_address = MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.get_proc_address);
          }
          opengl_context = wgl_context;
        } else {
          var egl_context = new MaplibreNative.EglContextDescriptor(
            MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.display),
            MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.config),
            MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.context));
          if (opengl_context_storage.get_proc_address != null) {
            egl_context.get_proc_address = MaplibreNative.NativePointer.borrowed((size_t) opengl_context_storage.get_proc_address);
          }
          opengl_context = egl_context;
        }
        var opengl_texture = new MaplibreNative.OpenGLOwnedTextureDescriptor(opengl_context);
        opengl_texture.width = 32;
        opengl_texture.height = 16;
        opengl_texture.scale_factor = 1.0;
        var opengl_session = map.attach_opengl_owned_texture(opengl_texture);
        opengl_session.resize(32, 16, 1.0);
        map.request_repaint();
        assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE, 128));
        assert(opengl_session.render_update());
        uint8[] opengl_pixels = new uint8[32 * 16 * 4];
        var opengl_info = opengl_session.read_premultiplied_rgba8(opengl_pixels);
        assert(opengl_info.width == 32);
        assert(opengl_info.height == 16);
        var opengl_frame = opengl_session.acquire_opengl_owned_texture_frame();
        assert(opengl_frame.get_width() == 32);
        assert(opengl_frame.get_height() == 16);
        assert(opengl_frame.get_texture().get() != 0);
        opengl_frame.close();
        opengl_session.detach();
        opengl_session.close();
      } finally {
        opengl_test_context_destroy(ref opengl_context_storage);
      }
    }

    if ((backends & MaplibreNative.RenderBackendFlags.METAL) != 0) {
      void* device = create_system_default_metal_device();
      if (device != null) {
        var texture = new MaplibreNative.MetalOwnedTextureDescriptor(MaplibreNative.NativePointer.borrowed((size_t) device));
        texture.width = 32;
        texture.height = 16;
        texture.scale_factor = 1.0;
        var session = map.attach_metal_owned_texture(texture);
        if (GLib.Environment.get_variable("MLN_VALA_RUN_TEXTURE_BACKEND_COMPILE_SMOKE") == "1") {
          compile_texture_backend_wrappers();
        }
        if (GLib.Environment.get_variable("MLN_VALA_RUN_UTF8_API_COMPILE_SMOKE") == "1") {
          compile_utf8_string_view_wrappers(map, session, feature_geojson, feature, image, custom_options);
        }
        session.resize(32, 16, 1.0);
        assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE, 64));
        session.render_update();
        assert(read_callback_count(ref custom_geometry_fetch_count) > 0);
        read_callback_count(ref custom_geometry_cancel_count).to_string();
        var feature_state_selector = new MaplibreNative.FeatureStateSelector("state-source");
        feature_state_selector.set_feature_id("feature-1");
        session.set_feature_state(feature_state_selector, MaplibreNative.JsonValue.object_value({ new MaplibreNative.JsonMember("selected", MaplibreNative.JsonValue.bool_value(true)) }));
        var copied_feature_state = session.get_feature_state(feature_state_selector);
        assert(copied_feature_state.value_type == MaplibreNative.JsonValueType.OBJECT);
        session.remove_feature_state(feature_state_selector);
        var query_result = session.query_rendered_features(MaplibreNative.RenderedQueryGeometry.point(MaplibreNative.ScreenPoint(0.0, 0.0)));
        query_result.length.to_string();
        var rendered_query_options = new MaplibreNative.RenderedFeatureQueryOptions();
        rendered_query_options.set_layer_ids({});
        rendered_query_options.set_filter(MaplibreNative.JsonValue.array_value({ MaplibreNative.JsonValue.string_value("has"), MaplibreNative.JsonValue.string_value("rank") }));
        var box_query_result = session.query_rendered_features(MaplibreNative.RenderedQueryGeometry.box(MaplibreNative.ScreenBox(MaplibreNative.ScreenPoint(0.0, 0.0), MaplibreNative.ScreenPoint(32.0, 16.0))), rendered_query_options);
        box_query_result.length.to_string();
        var line_query_result = session.query_rendered_features(MaplibreNative.RenderedQueryGeometry.line_string({ MaplibreNative.ScreenPoint(0.0, 0.0), MaplibreNative.ScreenPoint(32.0, 16.0) }));
        line_query_result.length.to_string();
        var source_query_options = new MaplibreNative.SourceFeatureQueryOptions();
        source_query_options.set_source_layer_ids({ "ignored-for-geojson" });
        var source_query_result = session.query_source_features("state-source", source_query_options);
        var source_query_count = source_query_result.length;
        if (source_query_count > 0) {
          var queried_feature = source_query_result[0];
          assert(queried_feature.feature.property_members.length > 0);
          queried_feature.get_source_id_utf8();
          queried_feature.get_source_layer_id_utf8();
          assert(queried_feature.equal(queried_feature.copy()));
        }
        if (GLib.Environment.get_variable("MLN_VALA_RUN_FEATURE_EXTENSION_SMOKE") == "1") {
          var extension_payload = session.query_feature_extensions("state-source", feature, "supercluster", "children");
          extension_payload.result_type.to_string();
          assert(extension_payload.equal(extension_payload.copy()));
        }
        uint8[] pixels = new uint8[32 * 16 * 4];
        var info = session.read_premultiplied_rgba8(pixels);
        assert(info.width == 32);
        assert(info.height == 16);
        assert(info.stride * info.height <= pixels.length);
        assert(info.equal(info.copy()));

        var frame = session.acquire_metal_owned_texture_frame();
        assert(frame.get_width() == 32);
        var frame_texture = frame.get_texture();
        bool render_while_acquired_failed = false;
        try {
          session.render_update();
        } catch (MaplibreNative.Error error) {
          render_while_acquired_failed = true;
        }
        assert(render_while_acquired_failed);
        bool resize_while_acquired_failed = false;
        try {
          session.resize(32, 16, 1.0);
        } catch (MaplibreNative.Error error) {
          resize_while_acquired_failed = true;
        }
        assert(resize_while_acquired_failed);
        bool detach_while_acquired_failed = false;
        try {
          session.detach();
        } catch (MaplibreNative.Error error) {
          detach_while_acquired_failed = true;
        }
        assert(detach_while_acquired_failed);
        frame.close();
        bool closed_frame_texture_failed = false;
        try {
          frame_texture.get_bits();
        } catch (MaplibreNative.Error error) {
          closed_frame_texture_failed = true;
        }
        assert(closed_frame_texture_failed);
        bool closed_frame_failed = false;
        try {
          frame.get_width();
        } catch (MaplibreNative.Error error) {
          closed_frame_failed = true;
        }
        assert(closed_frame_failed);
        session.dump_debug_logs();
        session.reduce_memory_use();
        session.clear_data();
        session.detach();
        assert(session.is_detached);
        bool render_after_detach_failed = false;
        try {
          session.render_update();
        } catch (MaplibreNative.Error error) {
          render_after_detach_failed = true;
        }
        assert(render_after_detach_failed);
        session.close();

        void* borrowed_texture = metal_test_texture_create(device, 32, 16);
        assert(borrowed_texture != null);
        try {
          var borrowed_descriptor = new MaplibreNative.MetalBorrowedTextureDescriptor(MaplibreNative.NativePointer.borrowed((size_t) borrowed_texture));
          borrowed_descriptor.width = 32;
          borrowed_descriptor.height = 16;
          borrowed_descriptor.scale_factor = 1.0;
          borrowed_descriptor.physical_width = 32;
          borrowed_descriptor.physical_height = 16;
          var borrowed_session = map.attach_metal_borrowed_texture(borrowed_descriptor);
          map.request_repaint();
          assert(wait_for_runtime_event(runtime, MaplibreNative.RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE, 64));
          borrowed_session.render_update();
          bool borrowed_resize_failed = false;
          try {
            borrowed_session.resize(16, 16, 1.0);
          } catch (MaplibreNative.Error error) {
            borrowed_resize_failed = true;
          }
          assert(borrowed_resize_failed);
          bool borrowed_readback_failed = false;
          try {
            uint8[] borrowed_pixels = new uint8[32 * 16 * 4];
            borrowed_session.read_premultiplied_rgba8(borrowed_pixels);
          } catch (MaplibreNative.Error error) {
            borrowed_readback_failed = true;
          }
          assert(borrowed_readback_failed);
          bool borrowed_frame_failed = false;
          try {
            borrowed_session.acquire_metal_owned_texture_frame();
          } catch (MaplibreNative.Error error) {
            borrowed_frame_failed = true;
          }
          assert(borrowed_frame_failed);
          borrowed_session.detach();
          borrowed_session.close();
        } finally {
          metal_test_object_release(borrowed_texture);
        }

        MetalWindowLayer metal_surface_layer;
        assert(metal_test_window_layer_create(32, 16, out metal_surface_layer));
        assert(metal_surface_layer.window != null);
        try {
          var surface_descriptor = new MaplibreNative.MetalSurfaceDescriptor(MaplibreNative.NativePointer.borrowed((size_t) metal_surface_layer.layer));
          surface_descriptor.device = MaplibreNative.NativePointer.borrowed((size_t) device);
          surface_descriptor.width = 32;
          surface_descriptor.height = 16;
          surface_descriptor.scale_factor = 1.0;
          var surface_session = map.attach_metal_surface(surface_descriptor);
          surface_session.resize(32, 16, 1.0);
          surface_session.detach();
          surface_session.close();
        } finally {
          metal_test_window_layer_destroy(ref metal_surface_layer);
        }
      }
    }

    map.close();
    map.close();
    runtime.close();
    runtime.close();
    exercise_runtime_close_race();
    var provider_history_runtime = new MaplibreNative.RuntimeHandle();
    provider_history_runtime.set_resource_provider(provide_resource);
    var provider_history_map = new MaplibreNative.MapHandle(provider_history_runtime);
    provider_history_map.close();
    provider_history_runtime.set_resource_provider(provide_resource);
    provider_history_runtime.close();
    if (offline_merge_database_path != null) {
      GLib.FileUtils.remove(offline_merge_database_path);
    }
    assert(map.closed);
    assert(runtime.closed);
    return 0;
  } catch (MaplibreNative.Error error) {
    stderr.printf("Vala binding smoke test failed: %s\n", error.message);
    return 1;
  }
}
