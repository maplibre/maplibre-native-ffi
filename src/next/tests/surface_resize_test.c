// Browser only: a continuous map's WebGPU surface session
// (MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD) on an OffscreenCanvas of this
// thread, resized while it renders.
//
// Every rendered frame must find the canvas at the frame's extent times the
// scale factor. The session sizes the canvas from the frame it draws, in the
// same task as the draw. It used to resize the canvas as soon as the session
// resized, so the frames rendered before the map's resized update arrived
// drew the old map size into the new canvas size, which the page showed
// stretched.

#include <maplibre_native_c.h>

#include <emscripten.h>
#include <webgpu/webgpu.h>

#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int failures = 0;

#define CHECK(condition)                                                   \
  do {                                                                     \
    if (!(condition)) {                                                    \
      fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__,     \
              #condition);                                                 \
      failures += 1;                                                       \
    }                                                                      \
  } while (0)

#define CHECK_STATUS(call)                                                   \
  do {                                                                       \
    mln_diagnostic diagnostic_ = {.size = sizeof(mln_diagnostic)};           \
    const mln_status status_ = (call);                                       \
    if (status_ != MLN_STATUS_OK) {                                          \
      fprintf(stderr, "%s:%d: %s returned %d: %s\n", __FILE__, __LINE__,     \
              #call, (int)status_, diagnostic_.message);                     \
      exit(1);                                                               \
    }                                                                        \
  } while (0)

#define DIAG (&diagnostic_)

// MARK: - The canvas

// An OffscreenCanvas of this thread, registered where the
// "#next-surface-resize" selector resolves under -sOFFSCREENCANVAS_SUPPORT
// (see tests/native/support/render_webgl.c).
EM_JS(void, register_canvas, (void), {
  const canvas = new OffscreenCanvas(1, 1);
  Module["GL"].offscreenCanvases["next-surface-resize"] = {
    canvas : canvas,
    offscreenCanvas : canvas,
    id : "next-surface-resize",
  };
});
EM_JS(int, canvas_width, (void), {
  return Module["GL"].offscreenCanvases["next-surface-resize"].offscreenCanvas.width;
});
EM_JS(int, canvas_height, (void), {
  return Module["GL"].offscreenCanvases["next-surface-resize"].offscreenCanvas.height;
});

// MARK: - WebGPU

static WGPUInstance instance;
static WGPUAdapter adapter;
static WGPUDevice device;
static WGPUSurface surface;
static WGPUTextureFormat format;

static void on_adapter(WGPURequestAdapterStatus status, WGPUAdapter result, WGPUStringView message,
                       void* out, void* unused) {
  (void)unused;
  if (status != WGPURequestAdapterStatus_Success) {
    fprintf(stderr, "requestAdapter: %.*s\n", (int)message.length, message.data);
  }
  *(WGPUAdapter*)out = result;
}

static void on_device(WGPURequestDeviceStatus status, WGPUDevice result, WGPUStringView message,
                      void* out, void* unused) {
  (void)unused;
  if (status != WGPURequestDeviceStatus_Success) {
    fprintf(stderr, "requestDevice: %.*s\n", (int)message.length, message.data);
  }
  *(WGPUDevice*)out = result;
}

static void on_uncaptured_error(const WGPUDevice* unused_device, WGPUErrorType type,
                                WGPUStringView message, void* a, void* b) {
  (void)unused_device, (void)a, (void)b;
  fprintf(stderr, "WebGPU error %d: %.*s\n", (int)type, (int)message.length, message.data);
  failures += 1;
}

static void wait_future(WGPUFuture future) {
  WGPUFutureWaitInfo info = {.future = future};
  wgpuInstanceWaitAny(instance, 1, &info, UINT64_MAX);
}

static void create_gpu(void) {
  const WGPUInstanceFeatureName features[] = {WGPUInstanceFeatureName_TimedWaitAny};
  const WGPUInstanceDescriptor instance_descriptor = {
    .requiredFeatureCount = 1, .requiredFeatures = features
  };
  instance = wgpuCreateInstance(&instance_descriptor);
  CHECK(instance != NULL);
  wait_future(wgpuInstanceRequestAdapter(
    instance, NULL,
    (WGPURequestAdapterCallbackInfo){
      .mode = WGPUCallbackMode_WaitAnyOnly, .callback = on_adapter, .userdata1 = &adapter
    }
  ));
  if (adapter == NULL) {
    fprintf(stderr, "no WebGPU adapter\n");
    exit(1);
  }
  const WGPUDeviceDescriptor device_descriptor = {
    .uncapturedErrorCallbackInfo = {.callback = on_uncaptured_error},
  };
  wait_future(wgpuAdapterRequestDevice(
    adapter, &device_descriptor,
    (WGPURequestDeviceCallbackInfo){
      .mode = WGPUCallbackMode_WaitAnyOnly, .callback = on_device, .userdata1 = &device
    }
  ));
  if (device == NULL) {
    fprintf(stderr, "no WebGPU device\n");
    exit(1);
  }
  WGPUEmscriptenSurfaceSourceCanvasHTMLSelector canvas = {
    .chain = {.sType = WGPUSType_EmscriptenSurfaceSourceCanvasHTMLSelector},
    .selector = {.data = "#next-surface-resize", .length = WGPU_STRLEN},
  };
  surface = wgpuInstanceCreateSurface(
    instance, &(WGPUSurfaceDescriptor){.nextInChain = &canvas.chain}
  );
  CHECK(surface != NULL);
  WGPUSurfaceCapabilities capabilities = {0};
  wgpuSurfaceGetCapabilities(surface, adapter, &capabilities);
  format = capabilities.formatCount > 0 ? capabilities.formats[0] : WGPUTextureFormat_BGRA8Unorm;
  wgpuSurfaceCapabilitiesFreeMembers(capabilities);
}

// MARK: - Completions

typedef struct waiter {
  atomic_int done;
  atomic_int released;
  mln_status status;
  char diagnostic[256];
} waiter;

static void on_complete(void* user_data, const mln_completion_result* result) {
  waiter* w = user_data;
  w->status = result->status;
  snprintf(w->diagnostic, sizeof(w->diagnostic), "%.*s", (int)result->diagnostic.size,
           (const char*)result->diagnostic.data);
  atomic_store(&w->done, 1);
}

static void on_release(void* user_data) { atomic_store(&((waiter*)user_data)->released, 1); }

static mln_completion completion_for(waiter* w) {
  memset(w, 0, sizeof(*w));
  return (mln_completion){
    .size = sizeof(mln_completion),
    .callback = on_complete,
    .user_data = w,
    .release_user_data = on_release,
  };
}

// The session's graphics work runs on this thread, so waits service it, and
// suspend (JSPI) so that the browser runs this thread's events.
static mln_render_session session = MLN_HANDLE_NULL;

static void service(void) {
  if (session != MLN_HANDLE_NULL) {
    CHECK_STATUS(mln_render_session_service_driver_work(session, DIAG));
  }
}

static void wait_for(waiter* w, const char* what) {
  for (int i = 0; i < 60000; i++) {
    if (atomic_load(&w->done) && atomic_load(&w->released)) {
      if (w->status != MLN_STATUS_OK) {
        fprintf(stderr, "%s failed (%d): %s\n", what, (int)w->status, w->diagnostic);
        exit(1);
      }
      return;
    }
    service();
    emscripten_sleep(1);
  }
  fprintf(stderr, "%s never completed\n", what);
  exit(1);
}

// MARK: - Frames

static const double scale_factor = 2.0;

// Renders frames until one shows `width` x `height`, checking that every
// rendered frame found the canvas at its own extent. Returns how many
// rendered frames showed another extent first.
static int render_until(uint32_t width, uint32_t height) {
  static uint64_t token = 0;
  int other_extent = 0;
  for (int attempt = 0; attempt < 600; attempt++) {
    const mln_frame_demand demand = {
      .size = sizeof(demand), .token = ++token, .flags = MLN_FRAME_DEMAND_FORCE
    };
    CHECK_STATUS(mln_render_session_demand_frame(session, &demand, DIAG));
    service();
    mln_render_frame_batch batch = MLN_HANDLE_NULL;
    mln_render_frame_batch_view view = {.size = sizeof(view)};
    CHECK_STATUS(mln_render_session_drain_frame_results(session, &batch, &view, DIAG));
    bool shown = false;
    for (size_t i = 0; i < view.result_count; i++) {
      const mln_render_frame_result result =
        *MLN_STRIDED_AT(mln_render_frame_result, view.results, view.result_size, i);
      CHECK(result.disposition != MLN_RENDER_RESULT_FAILED);
      if (result.disposition != MLN_RENDER_RESULT_RENDERED) continue;
      const mln_logical_extent extent = result.view.extent;
      const int expected_width = (int)lround(extent.width * extent.scale_factor);
      const int expected_height = (int)lround(extent.height * extent.scale_factor);
      if (canvas_width() != expected_width || canvas_height() != expected_height) {
        fprintf(stderr, "a frame of %ux%u at %.1fx found the canvas at %dx%d\n", extent.width,
                extent.height, extent.scale_factor, canvas_width(), canvas_height());
        failures += 1;
      }
      if (extent.width == width && extent.height == height) {
        shown = true;
      } else {
        other_extent += 1;
      }
    }
    if (batch != MLN_HANDLE_NULL) mln_render_frame_batch_release(batch);
    if (shown) return other_extent;
    emscripten_sleep(1);
  }
  fprintf(stderr, "no frame showed %ux%u\n", width, height);
  failures += 1;
  return other_extent;
}

// MARK: - Test

static const char* const background_style =
  "{\"version\":8,\"sources\":{},\"layers\":[{\"id\":\"background\","
  "\"type\":\"background\",\"paint\":{\"background-color\":\"#336699\"}}]}";

int main(void) {
  register_canvas();
  create_gpu();

  mln_runtime runtime = MLN_HANDLE_NULL;
  const mln_runtime_options runtime_options = {.size = sizeof(runtime_options)};
  CHECK_STATUS(mln_runtime_create(&runtime_options, &runtime, DIAG));

  waiter w;
  mln_completion done = completion_for(&w);
  const mln_map_options map_options = {
    .size = sizeof(map_options),
    .initial_extent = {.width = 160, .height = 120, .scale_factor = scale_factor},
    .map_mode = MLN_MAP_MODE_CONTINUOUS,
  };
  mln_map map = MLN_HANDLE_NULL;
  CHECK_STATUS(mln_runtime_create_map(runtime, &map_options, &map, &done, DIAG));
  wait_for(&w, "mln_runtime_create_map");
  done = completion_for(&w);
  CHECK_STATUS(mln_map_set_style_json(map, mln_buffer_view_from_cstr(background_style), &done, DIAG));
  wait_for(&w, "mln_map_set_style_json");

  done = completion_for(&w);
  const mln_web_gpu_surface_descriptor descriptor = {
    .size = sizeof(descriptor),
    .logical_size = {.width = 160, .height = 120},
    .context = {.instance = instance, .device = device},
    .surface = surface,
    .format = (uint32_t)format,
  };
  const mln_render_session_attach_options attach_options = {
    .size = sizeof(attach_options), .driver = MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD
  };
  CHECK_STATUS(
    mln_map_attach_web_gpu_surface(map, &descriptor, &attach_options, &session, &done, DIAG)
  );
  wait_for(&w, "mln_map_attach_web_gpu_surface");

  render_until(160, 120);
  CHECK(canvas_width() == 320 && canvas_height() == 240);

  // Resize, growing and shrinking, and render straight away, before the
  // resize completes: the map's resized update comes from the runtime thread,
  // so the first frames after a resize usually still show the earlier extent.
  // Those frames are the ones the bug stretched.
  const mln_logical_size sizes[] = {
    {.width = 240, .height = 100}, {.width = 90, .height = 150}, {.width = 200, .height = 200},
    {.width = 120, .height = 60},  {.width = 260, .height = 90}, {.width = 100, .height = 180},
  };
  int earlier_total = 0;
  for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); i++) {
    done = completion_for(&w);
    CHECK_STATUS(mln_render_session_resize(session, sizes[i], &done, DIAG));
    const int earlier = render_until(sizes[i].width, sizes[i].height);
    wait_for(&w, "mln_render_session_resize");
    printf("resize to %ux%u: %d frame(s) of the earlier extent first\n", sizes[i].width,
           sizes[i].height, earlier);
    earlier_total += earlier;
    CHECK(canvas_width() == (int)(sizes[i].width * scale_factor));
    CHECK(canvas_height() == (int)(sizes[i].height * scale_factor));
  }
  // Otherwise no frame rendered between a resize and the map's resized
  // update, and this test checked nothing.
  CHECK(earlier_total > 0);

  done = completion_for(&w);
  CHECK_STATUS(mln_render_session_detach(session, &done, DIAG));
  wait_for(&w, "mln_render_session_detach");

  if (failures != 0) {
    fprintf(stderr, "%d check(s) failed\n", failures);
    return 1;
  }
  printf("all checks passed\n");
  return 0;
}
