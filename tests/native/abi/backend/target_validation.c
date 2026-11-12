// Render target descriptors: what mln_map_attach() and
// mln_render_session_set_target() reject before they reach a device. Every
// build validates each kind's descriptor before it reports that it can't
// render to the kind, so these tables run on every preset.

#include "support/attach_table.h"
#include "support/test_support.h"

// Edits a kind's logical size: zero width, zero height, and a width that
// overflows 32 bits once the map that mln_test_create_target_map() creates
// scales it.
#define TARGET_SIZE_EDITS(member)                                      \
  static void member##_zero_width(void* call) {                        \
    ((mln_test_target_call*)call)                                      \
      ->descriptor.target.data.member.logical_size.width = 0;          \
  }                                                                    \
  static void member##_zero_height(void* call) {                       \
    ((mln_test_target_call*)call)                                      \
      ->descriptor.target.data.member.logical_size.height = 0;         \
  }                                                                    \
  static void member##_overflowing_size(void* call) {                  \
    ((mln_test_target_call*)call)                                      \
      ->descriptor.target.data.member.logical_size.width = UINT32_MAX; \
  }

#define TARGET_SIZE_CASES(member)                                              \
  {"zero logical width", member##_zero_width, MLN_STATUS_INVALID_ARGUMENT,     \
   "must be positive"},                                                        \
    {"zero logical height", member##_zero_height, MLN_STATUS_INVALID_ARGUMENT, \
     "must be positive"}

// An attach checks a scaled size against the map's scale factor. A
// replacement has the session's, which a stale session can't tell.
#define TARGET_OVERFLOW_CASE(member)                                     \
  {"logical size that overflows once scaled", member##_overflowing_size, \
   MLN_STATUS_INVALID_ARGUMENT, "too large"}

// Edits a kind's required pointer to null.
#define TARGET_NULL_EDIT(member, field)                                        \
  static void member##_without_##field(void* call) {                           \
    ((mln_test_target_call*)call)->descriptor.target.data.member.field = NULL; \
  }

#define TARGET_NULL_CASE(member, field)                  \
  {"null " #member " " #field, member##_without_##field, \
   MLN_STATUS_INVALID_ARGUMENT, "must not be null"}

TARGET_SIZE_EDITS(offscreen)
TARGET_SIZE_EDITS(metal_layer)
TARGET_SIZE_EDITS(win32_window)
TARGET_SIZE_EDITS(xlib_window)
TARGET_SIZE_EDITS(wayland_surface)
TARGET_SIZE_EDITS(android_window)
TARGET_SIZE_EDITS(ohos_window)
TARGET_SIZE_EDITS(web_gpu_surface)
TARGET_SIZE_EDITS(html_canvas)
TARGET_SIZE_EDITS(vulkan_surface)
TARGET_SIZE_EDITS(open_gl_surface)
TARGET_NULL_EDIT(metal_layer, layer)
TARGET_NULL_EDIT(win32_window, window)
TARGET_NULL_EDIT(xlib_window, display)
TARGET_NULL_EDIT(wayland_surface, display)
TARGET_NULL_EDIT(wayland_surface, surface)
TARGET_NULL_EDIT(android_window, window)
TARGET_NULL_EDIT(ohos_window, window)
TARGET_NULL_EDIT(web_gpu_surface, surface)
TARGET_NULL_EDIT(open_gl_surface, surface)

static void vulkan_surface_without_surface(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.data.vulkan_surface.surface =
    MLN_VULKAN_NON_DISPATCHABLE_HANDLE_NULL;
}

static void xlib_window_without_window(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.data.xlib_window.window = 0;
}
static void web_gpu_surface_without_format(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.data.web_gpu_surface.format =
    0;
}
static void html_canvas_without_selector(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.data.html_canvas.selector =
    (mln_buffer_view){0};
}
static void undersized_target(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.size =
    sizeof(mln_render_target) - 1;
}
static void unnamed_kind(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.kind =
    (mln_render_target_kind)0;
}
static void unknown_kind(void* call) {
  ((mln_test_target_call*)call)->descriptor.target.kind =
    (mln_render_target_kind)UINT32_MAX;
}
static void stale_device(void* call) {
  ((mln_test_target_call*)call)->device = UINT64_MAX;
}

static const char canvas_selector[] = "#map";

// A well-formed descriptor of `kind`, whose objects only validation reads.
static mln_render_target describe(mln_render_target_kind kind) {
  mln_render_target target = {.size = sizeof(mln_render_target), .kind = kind};
  const mln_logical_size size = mln_test_target_size();
  switch (kind) {
    case MLN_RENDER_TARGET_KIND_OFFSCREEN:
      target.data.offscreen = (mln_offscreen_target){.logical_size = size};
      break;
    case MLN_RENDER_TARGET_KIND_METAL_LAYER:
      target.data.metal_layer = (mln_metal_layer_target){
        .logical_size = size, .layer = MLN_TEST_FAKE_HANDLE
      };
      break;
    case MLN_RENDER_TARGET_KIND_WIN32_WINDOW:
      target.data.win32_window = (mln_win32_window_target){
        .logical_size = size, .window = MLN_TEST_FAKE_HANDLE
      };
      break;
    case MLN_RENDER_TARGET_KIND_XLIB_WINDOW:
      // A window with bits above the low 32, so a carrier that truncated it
      // would pass another window.
      target.data.xlib_window = (mln_xlib_window_target){
        .logical_size = size,
        .display = MLN_TEST_FAKE_HANDLE,
        .window = UINT64_C(0x8000000000000001),
      };
      break;
    case MLN_RENDER_TARGET_KIND_WAYLAND_SURFACE:
      target.data.wayland_surface = (mln_wayland_surface_target){
        .logical_size = size,
        .display = MLN_TEST_FAKE_HANDLE,
        .surface = MLN_TEST_FAKE_HANDLE,
      };
      break;
    case MLN_RENDER_TARGET_KIND_ANDROID_WINDOW:
      target.data.android_window = (mln_android_window_target){
        .logical_size = size, .window = MLN_TEST_FAKE_HANDLE
      };
      break;
    case MLN_RENDER_TARGET_KIND_OHOS_WINDOW:
      target.data.ohos_window = (mln_ohos_window_target){
        .logical_size = size, .window = MLN_TEST_FAKE_HANDLE
      };
      break;
    case MLN_RENDER_TARGET_KIND_WEB_GPU_SURFACE:
      // WGPUTextureFormat_BGRA8Unorm.
      target.data.web_gpu_surface = (mln_web_gpu_surface_target){
        .logical_size = size, .surface = MLN_TEST_FAKE_HANDLE, .format = 0x1B
      };
      break;
    case MLN_RENDER_TARGET_KIND_HTML_CANVAS:
      target.data.html_canvas = (mln_html_canvas_target){
        .logical_size = size,
        .selector = {canvas_selector, sizeof(canvas_selector) - 1},
      };
      break;
    case MLN_RENDER_TARGET_KIND_VULKAN_SURFACE:
      // A surface with bits above the low 32, so a carrier that truncated it
      // would pass another surface.
      target.data.vulkan_surface = (mln_vulkan_surface_target){
        .logical_size = size, .surface = UINT64_C(0x8000000000000001)
      };
      break;
    case MLN_RENDER_TARGET_KIND_OPEN_GL_SURFACE:
      target.data.open_gl_surface = (mln_open_gl_surface_target){
        .logical_size = size, .surface = MLN_TEST_FAKE_HANDLE
      };
      break;
    default:
      break;
  }
  return target;
}

// Each kind's rows: its size rules and its required objects, and the row
// that only an attach checks.
typedef struct kind_rows {
  mln_render_target_kind kind;
  mln_render_target_kind_flag flag;
  mln_test_validation_case rows[4];
  size_t row_count;
  mln_test_validation_case overflow;
} kind_rows;

static const kind_rows every_kind[] = {
  {MLN_RENDER_TARGET_KIND_OFFSCREEN,
   MLN_RENDER_TARGET_KIND_FLAG_OFFSCREEN,
   {TARGET_SIZE_CASES(offscreen)},
   2,
   TARGET_OVERFLOW_CASE(offscreen)},
  {MLN_RENDER_TARGET_KIND_METAL_LAYER,
   MLN_RENDER_TARGET_KIND_FLAG_METAL_LAYER,
   {TARGET_SIZE_CASES(metal_layer), TARGET_NULL_CASE(metal_layer, layer)},
   3,
   TARGET_OVERFLOW_CASE(metal_layer)},
  {MLN_RENDER_TARGET_KIND_WIN32_WINDOW,
   MLN_RENDER_TARGET_KIND_FLAG_WIN32_WINDOW,
   {TARGET_SIZE_CASES(win32_window), TARGET_NULL_CASE(win32_window, window)},
   3,
   TARGET_OVERFLOW_CASE(win32_window)},
  {MLN_RENDER_TARGET_KIND_XLIB_WINDOW,
   MLN_RENDER_TARGET_KIND_FLAG_XLIB_WINDOW,
   {TARGET_SIZE_CASES(xlib_window),
    TARGET_NULL_CASE(xlib_window, display),
    {"zero xlib window", xlib_window_without_window,
     MLN_STATUS_INVALID_ARGUMENT, "must not be null"}},
   4,
   TARGET_OVERFLOW_CASE(xlib_window)},
  {MLN_RENDER_TARGET_KIND_WAYLAND_SURFACE,
   MLN_RENDER_TARGET_KIND_FLAG_WAYLAND_SURFACE,
   {TARGET_SIZE_CASES(wayland_surface),
    TARGET_NULL_CASE(wayland_surface, display),
    TARGET_NULL_CASE(wayland_surface, surface)},
   4,
   TARGET_OVERFLOW_CASE(wayland_surface)},
  {MLN_RENDER_TARGET_KIND_ANDROID_WINDOW,
   MLN_RENDER_TARGET_KIND_FLAG_ANDROID_WINDOW,
   {TARGET_SIZE_CASES(android_window),
    TARGET_NULL_CASE(android_window, window)},
   3,
   TARGET_OVERFLOW_CASE(android_window)},
  {MLN_RENDER_TARGET_KIND_OHOS_WINDOW,
   MLN_RENDER_TARGET_KIND_FLAG_OHOS_WINDOW,
   {TARGET_SIZE_CASES(ohos_window), TARGET_NULL_CASE(ohos_window, window)},
   3,
   TARGET_OVERFLOW_CASE(ohos_window)},
  {MLN_RENDER_TARGET_KIND_WEB_GPU_SURFACE,
   MLN_RENDER_TARGET_KIND_FLAG_WEB_GPU_SURFACE,
   {TARGET_SIZE_CASES(web_gpu_surface),
    TARGET_NULL_CASE(web_gpu_surface, surface),
    {"zero surface format", web_gpu_surface_without_format,
     MLN_STATUS_INVALID_ARGUMENT, "format"}},
   4,
   TARGET_OVERFLOW_CASE(web_gpu_surface)},
  {MLN_RENDER_TARGET_KIND_HTML_CANVAS,
   MLN_RENDER_TARGET_KIND_FLAG_HTML_CANVAS,
   {TARGET_SIZE_CASES(html_canvas),
    {"empty canvas selector", html_canvas_without_selector,
     MLN_STATUS_INVALID_ARGUMENT, "selector"}},
   3,
   TARGET_OVERFLOW_CASE(html_canvas)},
  {MLN_RENDER_TARGET_KIND_VULKAN_SURFACE,
   MLN_RENDER_TARGET_KIND_FLAG_VULKAN_SURFACE,
   {TARGET_SIZE_CASES(vulkan_surface),
    {"null vulkan_surface surface", vulkan_surface_without_surface,
     MLN_STATUS_INVALID_ARGUMENT, "must not be null"}},
   3,
   TARGET_OVERFLOW_CASE(vulkan_surface)},
  {MLN_RENDER_TARGET_KIND_OPEN_GL_SURFACE,
   MLN_RENDER_TARGET_KIND_FLAG_OPEN_GL_SURFACE,
   {TARGET_SIZE_CASES(open_gl_surface),
    TARGET_NULL_CASE(open_gl_surface, surface)},
   3,
   TARGET_OVERFLOW_CASE(open_gl_surface)},
};

enum { kind_count = sizeof(every_kind) / sizeof(every_kind[0]) };

// The rows that every target checks before its kind's.
static const mln_test_validation_case target_rows[] = {
  {"undersized target", undersized_target, MLN_STATUS_INVALID_ARGUMENT,
   "size is too small"},
  {"unnamed kind", unnamed_kind, MLN_STATUS_INVALID_ARGUMENT, "kind"},
  {"unknown kind", unknown_kind, MLN_STATUS_INVALID_ARGUMENT, "kind"},
};

// Each attach checks every pointer and handle it reads, and each kind's
// descriptor, before it reaches a device.
static void attach_rejects_malformed_targets(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);
  for (size_t index = 0; index < kind_count; index += 1) {
    mln_test_target_call call =
      mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
    call.descriptor.target = describe(every_kind[index].kind);
    if (index == 0) {
      mln_test_run_attach_table(
        mln_test_submit_target_attach, &call, target_rows,
        sizeof(target_rows) / sizeof(target_rows[0])
      );
      static const mln_test_validation_case attach_rows[] = {
        {"stale device", stale_device, MLN_STATUS_INVALID_ARGUMENT, NULL},
        {"zero driver", mln_test_call_with_zero_driver,
         MLN_STATUS_INVALID_ARGUMENT, "render driver kind"},
      };
      mln_test_run_validation_table(
        attach_rows, sizeof(attach_rows) / sizeof(attach_rows[0]), &call,
        sizeof(call), mln_test_submit_target_attach, NULL
      );
    }
    mln_test_run_validation_table(
      every_kind[index].rows, every_kind[index].row_count, &call, sizeof(call),
      mln_test_submit_target_attach, NULL
    );
    mln_test_run_validation_table(
      &every_kind[index].overflow, 1, &call, sizeof(call),
      mln_test_submit_target_attach, NULL
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

// set_target checks its target before the session, so each row gets its
// edit's status from a disposed session. A well-formed window then reaches
// the session, and an offscreen target is never a replacement.
static void set_target_rejects_malformed_targets(void) {
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);
  const mln_render_session disposed = mln_test_create_disposed_session(map);
  for (size_t index = 0; index < kind_count; index += 1) {
    mln_test_target_call call =
      mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
    call.session = disposed;
    call.descriptor.target = describe(every_kind[index].kind);
    if (index == 0) {
      static const mln_test_validation_case null_target = {
        "null target", mln_test_call_without_descriptor,
        MLN_STATUS_INVALID_ARGUMENT, NULL
      };
      mln_test_run_validation_table(
        &null_target, 1, &call, sizeof(call), mln_test_submit_target_set_target,
        NULL
      );
      mln_test_run_validation_table(
        target_rows, sizeof(target_rows) / sizeof(target_rows[0]), &call,
        sizeof(call), mln_test_submit_target_set_target, NULL
      );
    }
    mln_test_run_validation_table(
      every_kind[index].rows, every_kind[index].row_count, &call, sizeof(call),
      mln_test_submit_target_set_target, NULL
    );
    const mln_test_validation_case well_formed =
      every_kind[index].kind == MLN_RENDER_TARGET_KIND_OFFSCREEN
        ? (
            mln_test_validation_case
          ){"an offscreen target", NULL, MLN_STATUS_UNSUPPORTED, NULL}
        : (mln_test_validation_case){
            "a well-formed target", NULL, MLN_STATUS_INVALID_STATE, NULL
          };
    mln_test_run_validation_table(
      &well_formed, 1, &call, sizeof(call), mln_test_submit_target_set_target,
      NULL
    );
  }
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
// A device renders to the kinds that its capabilities name, and refuses a
// well-formed target of any other kind before it uses the target's objects.
static void attach_refuses_the_kinds_that_the_device_lacks(void) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  mln_gpu_device_capabilities capabilities = {.size = sizeof(capabilities)};
  MLN_TEST_OK(
    mln_gpu_device_get_capabilities(device, &capabilities, MLN_TEST_DIAGNOSTIC)
  );
  // No library device presents to a WebGPU surface, which only a wrapped
  // device can.
  TEST_ASSERT_EQUAL_UINT32(
    0, capabilities.target_kinds & MLN_RENDER_TARGET_KIND_FLAG_WEB_GPU_SURFACE
  );
  mln_runtime runtime = mln_test_create_runtime();
  mln_map map = mln_test_create_target_map(runtime);
  for (size_t index = 0; index < kind_count; index += 1) {
    if ((capabilities.target_kinds & every_kind[index].flag) != 0) continue;
    mln_test_target_call call =
      mln_test_target_call_default(map, MLN_RENDER_DRIVER_CORE_WORKER);
    call.device = device;
    call.descriptor.target = describe(every_kind[index].kind);
    const mln_test_validation_case refused = {
      "a kind that the device lacks", NULL, MLN_STATUS_UNSUPPORTED, NULL
    };
    mln_test_run_validation_table(
      &refused, 1, &call, sizeof(call), mln_test_submit_target_attach, NULL
    );
  }
  mln_gpu_device_release(device);
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(attach_rejects_malformed_targets);
  RUN_TEST(set_target_rejects_malformed_targets);
#if !defined(MLN_FFI_TEST_BACKEND_WEBGPU)
  RUN_TEST(attach_refuses_the_kinds_that_the_device_lacks);
#endif
}
