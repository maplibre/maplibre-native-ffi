// Attaching each kind of OpenGL render target to a live map, replacing the
// target of an attached session, and closing the two in order.

#include <maplibre_native_c.h>

static mln_status drive_operation(
  mln_render_session session, mln_operation operation
) {
  bool completed = false;
  while (!completed) {
    size_t serviced = 0;
    mln_status status =
      mln_render_session_service_driver_work(session, 64, &serviced);
    if (status != MLN_STATUS_OK) return status;
    status = mln_operation_wait(operation, serviced == 0 ? 1 : 0, &completed);
    if (status != MLN_STATUS_OK) return status;
  }
  mln_status terminal = MLN_STATUS_NATIVE_ERROR;
  return mln_operation_get_status(operation, &terminal) == MLN_STATUS_OK
           ? terminal
           : MLN_STATUS_NATIVE_ERROR;
}

static mln_render_session_attach_options caller_driver(void) {
  mln_render_session_attach_options options =
    mln_render_session_attach_options_default();
  options.driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD;
  return options;
}

mln_render_session attach_to_window(
  mln_map map, const mln_opengl_context_descriptor* context, void* egl_surface,
  uint32_t width, uint32_t height, double scale_factor
) {
  // #region surface
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  descriptor.surface = egl_surface;
  const mln_render_session_attach_options options = caller_driver();

  mln_render_session session = MLN_HANDLE_NULL;
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status = mln_opengl_surface_attach_start(
    map, &descriptor, &options, &session, &operation
  );
  if (status == MLN_STATUS_OK) status = drive_operation(session, operation);
  mln_operation_release(operation);
  // #endregion surface
  if (status != MLN_STATUS_OK && session != MLN_HANDLE_NULL) {
    mln_render_abandon_result abandoned = {
      .size = sizeof(mln_render_abandon_result)
    };
    mln_render_session_abandon(session, &abandoned);
    mln_render_session_destroy(session);
    return MLN_HANDLE_NULL;
  }
  return session;
}

mln_render_session attach_to_own_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t width,
  uint32_t height, double scale_factor
) {
  // #region owned
  mln_opengl_owned_texture_descriptor descriptor =
    mln_opengl_owned_texture_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  const mln_render_session_attach_options options = caller_driver();

  mln_render_session session = MLN_HANDLE_NULL;
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status = mln_opengl_owned_texture_attach_start(
    map, &descriptor, &options, &session, &operation
  );
  if (status == MLN_STATUS_OK) status = drive_operation(session, operation);
  mln_operation_release(operation);
  // #endregion owned
  if (status != MLN_STATUS_OK && session != MLN_HANDLE_NULL) {
    mln_render_abandon_result abandoned = {
      .size = sizeof(mln_render_abandon_result)
    };
    mln_render_session_abandon(session, &abandoned);
    mln_render_session_destroy(session);
    return MLN_HANDLE_NULL;
  }
  return session;
}

mln_render_session attach_to_host_texture(
  mln_map map, const mln_opengl_context_descriptor* context, uint32_t texture,
  uint32_t texture_target, uint32_t logical_width, uint32_t logical_height,
  double scale_factor
) {
  // #region borrowed
  mln_opengl_borrowed_texture_descriptor descriptor =
    mln_opengl_borrowed_texture_descriptor_default();
  descriptor.extent.width = logical_width;
  descriptor.extent.height = logical_height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.physical_width = (uint32_t)(logical_width * scale_factor);
  descriptor.physical_height = (uint32_t)(logical_height * scale_factor);
  descriptor.context = *context;
  descriptor.texture = texture;
  descriptor.target = texture_target;
  const mln_render_session_attach_options options = caller_driver();

  mln_render_session session = MLN_HANDLE_NULL;
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status = mln_opengl_borrowed_texture_attach_start(
    map, &descriptor, &options, &session, &operation
  );
  if (status == MLN_STATUS_OK) status = drive_operation(session, operation);
  mln_operation_release(operation);
  // #endregion borrowed
  if (status != MLN_STATUS_OK && session != MLN_HANDLE_NULL) {
    mln_render_abandon_result abandoned = {
      .size = sizeof(mln_render_abandon_result)
    };
    mln_render_session_abandon(session, &abandoned);
    mln_render_session_destroy(session);
    return MLN_HANDLE_NULL;
  }
  return session;
}

mln_status resize_session(
  mln_render_session session, uint32_t width, uint32_t height,
  double scale_factor
) {
  // #region resize
  const mln_render_target_extent extent = {
    .width = width, .height = height, .scale_factor = scale_factor
  };
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status =
    mln_render_session_resize_start(session, &extent, &operation);
  if (status == MLN_STATUS_OK) status = drive_operation(session, operation);
  mln_operation_release(operation);
  return status;
  // #endregion resize
}

mln_status resize_window_target(
  mln_render_session session, const mln_opengl_context_descriptor* context,
  void* egl_surface, uint32_t width, uint32_t height, double scale_factor
) {
  // #region set-target
  mln_opengl_surface_descriptor descriptor =
    mln_opengl_surface_descriptor_default();
  descriptor.extent.width = width;
  descriptor.extent.height = height;
  descriptor.extent.scale_factor = scale_factor;
  descriptor.context = *context;
  descriptor.surface = egl_surface;
  mln_operation operation = MLN_HANDLE_NULL;
  mln_status status =
    mln_opengl_surface_set_target_start(session, &descriptor, &operation);
  if (status == MLN_STATUS_OK) status = drive_operation(session, operation);
  mln_operation_release(operation);
  return status;
  // #endregion set-target
}

void release_map(mln_map map, mln_render_session session) {
  // #region teardown
  mln_operation operation = MLN_HANDLE_NULL;
  if (mln_render_session_detach_start(session, &operation) == MLN_STATUS_OK) {
    (void)drive_operation(session, operation);
  }
  mln_operation_release(operation);
  mln_render_session_destroy(session);

  operation = MLN_HANDLE_NULL;
  if (mln_map_close_start(map, &operation) == MLN_STATUS_OK) {
    bool completed = false;
    (void)mln_operation_wait(operation, -1, &completed);
  }
  mln_operation_release(operation);
  // #endregion teardown
}
