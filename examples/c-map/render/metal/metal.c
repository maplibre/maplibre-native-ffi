// The Metal render target: an SDL Metal view bridged to the C API through a
// CAMetalLayer and an MTLDevice, a fullscreen-triangle compositor whose MSL
// source compiles at startup, and the three render-target modes behind the
// uniform interface in render.h.
//
// The example stays plain C, so every Metal and Cocoa call goes through typed
// objc_msgSend casts rather than an Objective-C translation unit.

#include <SDL3/SDL.h>
#include <SDL3/SDL_metal.h>
#include <maplibre_native_c.h>
#include <objc/message.h>
#include <objc/runtime.h>
#include <stdlib.h>

#include "../../diagnostics.h"
#include "../../render_target.h"
#include "../../types.h"
#include "../../util.h"
#include "../render.h"

extern id MTLCreateSystemDefaultDevice(void);
extern void* objc_autoreleasePoolPush(void);
extern void objc_autoreleasePoolPop(void* pool);

// Metal enum values stated directly, so the example needs no Objective-C
// headers. Each matches its MTL* counterpart by name.
static constexpr unsigned long mtl_pixel_format_bgra8_unorm = 80;
static constexpr unsigned long mtl_pixel_format_rgba8_unorm = 70;
static constexpr unsigned long mtl_load_action_clear = 2;
static constexpr unsigned long mtl_store_action_store = 1;
static constexpr unsigned long mtl_primitive_type_triangle = 3;
static constexpr unsigned long mtl_texture_usage_shader_read = 1;
static constexpr unsigned long mtl_texture_usage_render_target = 4;

/// CGSize stated directly: two CGFloat fields.
typedef struct metal_cg_size {
  double width;
  double height;
} metal_cg_size;

/// MTLClearColor stated directly: four double fields.
typedef struct metal_clear_color {
  double red;
  double green;
  double blue;
  double alpha;
} metal_clear_color;

static const char* const metal_shader_source =
  "#include <metal_stdlib>\n"
  "using namespace metal;\n"
  "struct VertexOut {\n"
  "  float4 position [[position]];\n"
  "  float2 uv;\n"
  "};\n"
  "vertex VertexOut vertex_main(uint vertex_id [[vertex_id]]) {\n"
  "  float2 positions[3] = {\n"
  "    float2(-1.0, 1.0), float2(3.0, 1.0), float2(-1.0, -3.0),\n"
  "  };\n"
  "  float2 uvs[3] = {\n"
  "    float2(0.0, 0.0), float2(2.0, 0.0), float2(0.0, 2.0),\n"
  "  };\n"
  "  VertexOut out;\n"
  "  out.position = float4(positions[vertex_id], 0.0, 1.0);\n"
  "  out.uv = uvs[vertex_id];\n"
  "  return out;\n"
  "}\n"
  "fragment float4 fragment_main(\n"
  "  VertexOut in [[stage_in]],\n"
  "  texture2d<float> map_texture [[texture(0)]]\n"
  ") {\n"
  "  constexpr sampler map_sampler(address::clamp_to_edge, filter::linear);\n"
  "  return map_texture.sample(map_sampler, in.uv);\n"
  "}\n";

// Typed objc_msgSend wrappers for the common message shapes. Less common
// shapes cast objc_msgSend at the call site instead.

static id msg_id(id receiver, const char* selector) {
  return ((id (*)(id, SEL))objc_msgSend)(receiver, sel_registerName(selector));
}

static void msg_void(id receiver, const char* selector) {
  ((void (*)(id, SEL))objc_msgSend)(receiver, sel_registerName(selector));
}

static void msg_set_id(id receiver, const char* selector, id value) {
  ((void (*)(id, SEL, id))objc_msgSend)(
    receiver, sel_registerName(selector), value
  );
}

static void msg_set_ulong(
  id receiver, const char* selector, unsigned long value
) {
  ((void (*)(id, SEL, unsigned long))objc_msgSend)(
    receiver, sel_registerName(selector), value
  );
}

static id msg_id_with_id(id receiver, const char* selector, id value) {
  return ((id (*)(id, SEL, id))objc_msgSend)(
    receiver, sel_registerName(selector), value
  );
}

static id msg_id_at_index(
  id receiver, const char* selector, unsigned long index
) {
  return ((id (*)(id, SEL, unsigned long))objc_msgSend)(
    receiver, sel_registerName(selector), index
  );
}

static id class_object(const char* name) { return (id)objc_getClass(name); }

static id nsstring_from_utf8(const char* value) {
  return ((id (*)(id, SEL, const char*))objc_msgSend)(
    class_object("NSString"), sel_registerName("stringWithUTF8String:"), value
  );
}

static void release_object(id* object) {
  if (*object != nullptr) {
    msg_void(*object, "release");
    *object = nullptr;
  }
}

static metal_cg_size drawable_size(viewport current_viewport) {
  return (metal_cg_size){
    .width = (double)current_viewport.physical_width,
    .height = (double)current_viewport.physical_height,
  };
}

/// The SDL Metal view plus the device and CAMetalLayer behind it. The layer
/// belongs to the view; the device is the one object this struct releases.
typedef struct metal_view {
  SDL_MetalView view;
  id device;
  id layer;
} metal_view;

static void metal_view_resize(metal_view* view, viewport current_viewport) {
  ((void (*)(id, SEL, metal_cg_size))objc_msgSend)(
    view->layer, sel_registerName("setDrawableSize:"),
    drawable_size(current_viewport)
  );
}

static void metal_view_deinit(metal_view* view) {
  release_object(&view->device);
  if (view->view != nullptr) {
    SDL_Metal_DestroyView(view->view);
    view->view = nullptr;
  }
}

static app_error metal_view_init(
  metal_view* view, SDL_Window* window, viewport current_viewport
) {
  *view = (metal_view){};
  view->view = SDL_Metal_CreateView(window);
  if (view->view == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  view->device = MTLCreateSystemDefaultDevice();
  void* layer = SDL_Metal_GetLayer(view->view);
  if (view->device == nullptr || layer == nullptr) {
    metal_view_deinit(view);
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  view->layer = (id)layer;
  msg_set_id(view->layer, "setDevice:", view->device);
  msg_set_ulong(view->layer, "setPixelFormat:", mtl_pixel_format_bgra8_unorm);
  metal_view_resize(view, current_viewport);
  return APP_OK;
}

static app_error create_pipeline(id device, id* out_pipeline) {
  const id source = nsstring_from_utf8(metal_shader_source);
  if (source == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }

  id library_error = nullptr;
  id library = ((id (*)(id, SEL, id, id, id*))objc_msgSend)(
    device, sel_registerName("newLibraryWithSource:options:error:"), source,
    nullptr, &library_error
  );
  if (library == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }

  id vertex = msg_id_with_id(
    library, "newFunctionWithName:", nsstring_from_utf8("vertex_main")
  );
  id fragment = msg_id_with_id(
    library, "newFunctionWithName:", nsstring_from_utf8("fragment_main")
  );
  id descriptor = nullptr;
  id pipeline = nullptr;
  if (vertex != nullptr && fragment != nullptr) {
    descriptor = msg_id(
      msg_id(class_object("MTLRenderPipelineDescriptor"), "alloc"), "init"
    );
  }
  if (descriptor != nullptr) {
    msg_set_id(descriptor, "setVertexFunction:", vertex);
    msg_set_id(descriptor, "setFragmentFunction:", fragment);
    const id attachment = msg_id_at_index(
      msg_id(descriptor, "colorAttachments"), "objectAtIndexedSubscript:", 0
    );
    msg_set_ulong(attachment, "setPixelFormat:", mtl_pixel_format_bgra8_unorm);

    id pipeline_error = nullptr;
    pipeline = ((id (*)(id, SEL, id, id*))objc_msgSend)(
      device, sel_registerName("newRenderPipelineStateWithDescriptor:error:"),
      descriptor, &pipeline_error
    );
  }
  release_object(&descriptor);
  release_object(&fragment);
  release_object(&vertex);
  release_object(&library);
  if (pipeline == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  *out_pipeline = pipeline;
  return APP_OK;
}

/// The compositor: a fullscreen-triangle pass that samples the map texture
/// into the layer's next drawable.
typedef struct metal_compositor {
  metal_view view;
  id queue;
  id pipeline;
} metal_compositor;

static void metal_compositor_deinit(metal_compositor* compositor) {
  release_object(&compositor->pipeline);
  release_object(&compositor->queue);
  metal_view_deinit(&compositor->view);
}

static app_error metal_compositor_init(
  metal_compositor* compositor, SDL_Window* window, viewport current_viewport
) {
  *compositor = (metal_compositor){};
  MAP_TRY(metal_view_init(&compositor->view, window, current_viewport));
  compositor->queue = msg_id(compositor->view.device, "newCommandQueue");
  app_error error = APP_OK;
  if (compositor->queue == nullptr) {
    error = APP_ERROR_BACKEND_SETUP_FAILED;
  } else {
    error = create_pipeline(compositor->view.device, &compositor->pipeline);
  }
  if (error != APP_OK) {
    metal_compositor_deinit(compositor);
  }
  return error;
}

static void metal_compositor_resize(
  metal_compositor* compositor, viewport current_viewport
) {
  metal_view_resize(&compositor->view, current_viewport);
}

static app_error metal_compositor_draw_texture(
  metal_compositor* compositor, id texture
) {
  const id drawable = msg_id(compositor->view.layer, "nextDrawable");
  if (drawable == nullptr) {
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }

  const id drawable_texture = msg_id(drawable, "texture");
  const id pass_descriptor =
    msg_id(class_object("MTLRenderPassDescriptor"), "renderPassDescriptor");
  const id attachment = msg_id_at_index(
    msg_id(pass_descriptor, "colorAttachments"), "objectAtIndexedSubscript:", 0
  );
  msg_set_id(attachment, "setTexture:", drawable_texture);
  msg_set_ulong(attachment, "setLoadAction:", mtl_load_action_clear);
  msg_set_ulong(attachment, "setStoreAction:", mtl_store_action_store);
  ((void (*)(id, SEL, metal_clear_color))objc_msgSend)(
    attachment, sel_registerName("setClearColor:"),
    (metal_clear_color){.red = 0.08, .green = 0.09, .blue = 0.11, .alpha = 1.0}
  );

  const id command_buffer = msg_id(compositor->queue, "commandBuffer");
  if (command_buffer == nullptr) {
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }
  const id encoder = msg_id_with_id(
    command_buffer, "renderCommandEncoderWithDescriptor:", pass_descriptor
  );
  if (encoder == nullptr) {
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }

  msg_set_id(encoder, "setRenderPipelineState:", compositor->pipeline);
  ((void (*)(id, SEL, id, unsigned long))objc_msgSend)(
    encoder, sel_registerName("setFragmentTexture:atIndex:"), texture, 0
  );
  ((void (*)(id, SEL, unsigned long, unsigned long, unsigned long))
     objc_msgSend)(
    encoder, sel_registerName("drawPrimitives:vertexStart:vertexCount:"),
    mtl_primitive_type_triangle, 0, 3
  );
  msg_void(encoder, "endEncoding");
  msg_set_id(command_buffer, "presentDrawable:", drawable);
  msg_void(command_buffer, "commit");
  msg_void(command_buffer, "waitUntilCompleted");
  return APP_OK;
}

static app_error borrowed_texture_create(
  id device, viewport current_viewport, id* out_texture
) {
  const id descriptor = ((
    id (*)(id, SEL, unsigned long, unsigned long, unsigned long, bool)
  )objc_msgSend)(
    class_object("MTLTextureDescriptor"),
    sel_registerName(
      "texture2DDescriptorWithPixelFormat:width:height:mipmapped:"
    ),
    mtl_pixel_format_rgba8_unorm, current_viewport.physical_width,
    current_viewport.physical_height, false
  );
  if (descriptor == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  msg_set_ulong(
    descriptor,
    "setUsage:", mtl_texture_usage_shader_read | mtl_texture_usage_render_target
  );
  const id texture =
    msg_id_with_id(device, "newTextureWithDescriptor:", descriptor);
  if (texture == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  *out_texture = texture;
  return APP_OK;
}

struct render_target {
  render_target_mode mode;
  render_session session;
  union {
    struct {
      metal_compositor compositor;
    } owned;
    struct {
      metal_compositor compositor;
      id texture;
    } borrowed;
    struct {
      metal_view view;
    } surface;
  } as;
};

uint32_t render_target_backend_flag(void) {
  return MLN_RENDER_BACKEND_FLAG_METAL;
}

void render_target_apply_sdl_hints(void) {}

app_error render_target_configure_video(void) { return APP_OK; }

SDL_WindowFlags render_target_window_flags(void) { return SDL_WINDOW_METAL; }

void* render_target_frame_scope_open(void) {
  return objc_autoreleasePoolPush();
}

void render_target_frame_scope_close(void* scope) {
  objc_autoreleasePoolPop(scope);
}

app_error render_target_init(
  render_target** out_target, SDL_Window* window, viewport current_viewport,
  render_target_mode mode
) {
  render_target* target = calloc(1, sizeof(render_target));
  if (target == nullptr) {
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  target->mode = mode;

  app_error error = APP_OK;
  switch (mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      error = metal_compositor_init(
        &target->as.owned.compositor, window, current_viewport
      );
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      error = metal_compositor_init(
        &target->as.borrowed.compositor, window, current_viewport
      );
      if (error == APP_OK) {
        error = borrowed_texture_create(
          target->as.borrowed.compositor.view.device, current_viewport,
          &target->as.borrowed.texture
        );
        if (error != APP_OK) {
          metal_compositor_deinit(&target->as.borrowed.compositor);
        }
      }
      break;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      error =
        metal_view_init(&target->as.surface.view, window, current_viewport);
      break;
  }
  if (error != APP_OK) {
    free(target);
    return error;
  }
  *out_target = target;
  return APP_OK;
}

static mln_metal_context_descriptor metal_context_descriptor(id device) {
  return (mln_metal_context_descriptor){
    .size = sizeof(mln_metal_context_descriptor),
    .device = device,
  };
}

static mln_metal_borrowed_texture_descriptor borrowed_texture_descriptor(
  render_target* target, viewport current_viewport
) {
  mln_metal_borrowed_texture_descriptor descriptor =
    mln_metal_borrowed_texture_descriptor_default();
  descriptor.extent = render_target_extent(current_viewport);
  descriptor.physical_width = current_viewport.physical_width;
  descriptor.physical_height = current_viewport.physical_height;
  descriptor.texture = target->as.borrowed.texture;
  return descriptor;
}

app_error render_target_attach(
  render_target* target, mln_map map, viewport current_viewport
) {
  mln_render_session session = MLN_HANDLE_NULL;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE: {
      mln_metal_owned_texture_descriptor descriptor =
        mln_metal_owned_texture_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        metal_context_descriptor(target->as.owned.compositor.view.device);
      const mln_status status =
        mln_metal_owned_texture_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("Metal texture attach failed", status);
        return APP_ERROR_TEXTURE_ATTACH_FAILED;
      }
      target->session =
        (render_session){.kind = RENDER_SESSION_TEXTURE, .handle = session};
      return APP_OK;
    }
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      const mln_metal_borrowed_texture_descriptor descriptor =
        borrowed_texture_descriptor(target, current_viewport);
      const mln_status status =
        mln_metal_borrowed_texture_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("Metal borrowed texture attach failed", status);
        return APP_ERROR_TEXTURE_ATTACH_FAILED;
      }
      target->session =
        (render_session){.kind = RENDER_SESSION_TEXTURE, .handle = session};
      return APP_OK;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE: {
      mln_metal_surface_descriptor descriptor =
        mln_metal_surface_descriptor_default();
      descriptor.extent = render_target_extent(current_viewport);
      descriptor.context =
        metal_context_descriptor(target->as.surface.view.device);
      descriptor.layer = target->as.surface.view.layer;
      const mln_status status =
        mln_metal_surface_attach(map, &descriptor, &session);
      if (status != MLN_STATUS_OK) {
        diagnostics_log_status("Metal surface attach failed", status);
        return APP_ERROR_SURFACE_ATTACH_FAILED;
      }
      target->session =
        (render_session){.kind = RENDER_SESSION_SURFACE, .handle = session};
      return APP_OK;
    }
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

void render_target_deinit(render_target* target) {
  if (target == nullptr) {
    return;
  }
  render_session_close(&target->session);
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      metal_compositor_deinit(&target->as.owned.compositor);
      break;
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      release_object(&target->as.borrowed.texture);
      metal_compositor_deinit(&target->as.borrowed.compositor);
      break;
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      metal_view_deinit(&target->as.surface.view);
      break;
  }
  free(target);
}

/// Follows a resized window in borrowed-texture mode.
///
/// This example sizes the borrowed texture, not the session, so following a
/// resize means allocating one at the new size and handing it to the live
/// session. The session stays live, which is what keeps the map from going
/// cold on every resize.
static app_error resize_borrowed(
  render_target* target, viewport current_viewport
) {
  if (target->session.kind != RENDER_SESSION_TEXTURE) {
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  metal_compositor_resize(&target->as.borrowed.compositor, current_viewport);

  id previous = target->as.borrowed.texture;
  id replacement = nullptr;
  MAP_TRY(borrowed_texture_create(
    target->as.borrowed.compositor.view.device, current_viewport, &replacement
  ));
  target->as.borrowed.texture = replacement;
  const mln_metal_borrowed_texture_descriptor descriptor =
    borrowed_texture_descriptor(target, current_viewport);
  const mln_status status =
    mln_metal_borrowed_texture_set_target(target->session.handle, &descriptor);
  if (status != MLN_STATUS_OK) {
    // A native error may mean the session took the replacement before
    // failing, and nothing here can tell that apart from a rejection that
    // came first, so detach before either target is released.
    mln_render_session_detach(target->session.handle);
    diagnostics_log_status("Metal borrowed texture set target failed", status);
    target->as.borrowed.texture = previous;
    release_object(&replacement);
    return APP_ERROR_TEXTURE_RESIZE_FAILED;
  }
  // Only once the session has taken the replacement, so a rejected one leaves
  // this target on the texture it already had.
  release_object(&previous);
  return APP_OK;
}

app_error render_target_resize(
  render_target* target, viewport current_viewport
) {
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      metal_compositor_resize(&target->as.owned.compositor, current_viewport);
      return render_session_resize(&target->session, current_viewport);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE:
      return resize_borrowed(target, current_viewport);
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      metal_view_resize(&target->as.surface.view, current_viewport);
      return render_session_resize(&target->session, current_viewport);
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}

app_error render_target_finish_frame(render_target* target) {
  (void)target;
  return APP_OK;
}

static app_error render_update_owned(
  render_target* target, bool* out_rendered
) {
  bool rendered = false;
  MAP_TRY(render_session_render_update(&target->session, &rendered));
  if (!rendered) {
    return APP_OK;
  }

  mln_metal_owned_texture_frame frame = {.size = sizeof(frame)};
  const mln_status status =
    mln_metal_owned_texture_acquire_frame(target->session.handle, &frame);
  if (status == MLN_STATUS_INVALID_STATE) {
    return APP_OK;
  }
  if (status != MLN_STATUS_OK) {
    diagnostics_log_status("Metal texture acquire failed", status);
    return APP_ERROR_BACKEND_DRAW_FAILED;
  }

  const app_error error = metal_compositor_draw_texture(
    &target->as.owned.compositor, (id)frame.texture
  );
  const mln_status release_status =
    mln_metal_owned_texture_release_frame(target->session.handle, &frame);
  if (release_status != MLN_STATUS_OK) {
    diagnostics_log_status("Metal texture release failed", release_status);
  }
  MAP_TRY(error);
  *out_rendered = true;
  return APP_OK;
}

app_error render_target_render_update(
  render_target* target, viewport current_viewport, bool* out_rendered
) {
  (void)current_viewport;
  *out_rendered = false;
  switch (target->mode) {
    case RENDER_TARGET_MODE_OWNED_TEXTURE:
      return render_update_owned(target, out_rendered);
    case RENDER_TARGET_MODE_BORROWED_TEXTURE: {
      bool rendered = false;
      MAP_TRY(render_session_render_update(&target->session, &rendered));
      if (!rendered) {
        return APP_OK;
      }
      MAP_TRY(metal_compositor_draw_texture(
        &target->as.borrowed.compositor, target->as.borrowed.texture
      ));
      *out_rendered = true;
      return APP_OK;
    }
    case RENDER_TARGET_MODE_NATIVE_SURFACE:
      return render_session_render_update(&target->session, out_rendered);
  }
  return APP_ERROR_BACKEND_SETUP_FAILED;
}
