// Sessions that present to a Win32 window with a graphics context that the
// library creates. Each window has a thread of its own that processes its
// messages, and a Vulkan build reads back what the session presents from the
// window's device context. Each case destroys its windows even after a failed
// assertion, once its session no longer uses them.

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>

#include "support/frames.h"
#include "support/test_support.h"

enum { window_size = 64 };

typedef struct test_window {
  HANDLE thread;
  HANDLE created;
  HWND window;
} test_window;

static const wchar_t window_class_name[] = L"MlnTestWin32Window";

static LRESULT CALLBACK
window_procedure(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
  if (message == WM_DESTROY) {
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(window, message, wparam, lparam);
}

// A pop-up window's client area is the whole window. Mesa presents through
// GDI, which draws only into a shown window.
static DWORD WINAPI window_thread(void* context) {
  test_window* window = context;
  const HINSTANCE instance = GetModuleHandleW(NULL);
  const WNDCLASSW window_class = {
    .lpfnWndProc = window_procedure,
    .hInstance = instance,
    .lpszClassName = window_class_name,
  };
  // A second registration fails with ERROR_CLASS_ALREADY_EXISTS, which leaves
  // the first one in place.
  RegisterClassW(&window_class);
  window->window = CreateWindowExW(
    WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW, window_class_name, window_class_name,
    WS_POPUP, 0, 0, window_size, window_size, NULL, NULL, instance, NULL
  );
  if (window->window != NULL) {
    ShowWindow(window->window, SW_SHOWNOACTIVATE);
    UpdateWindow(window->window);
  }
  SetEvent(window->created);
  if (window->window == NULL) return 1;
  MSG message;
  while (GetMessageW(&message, NULL, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  return 0;
}

static void window_create(test_window* out) {
  out->created = CreateEventW(NULL, TRUE, FALSE, NULL);
  TEST_ASSERT_NOT_NULL(out->created);
  out->thread = CreateThread(NULL, 0, window_thread, out, 0, NULL);
  TEST_ASSERT_NOT_NULL(out->thread);
  TEST_ASSERT_EQUAL_UINT32(
    WAIT_OBJECT_0, WaitForSingleObject(out->created, INFINITE)
  );
  TEST_ASSERT_NOT_NULL_MESSAGE(out->window, "CreateWindowExW failed");
}

static void window_destroy(test_window* window) {
  if (window->thread != NULL) {
    if (window->window != NULL) PostMessageW(window->window, WM_CLOSE, 0, 0);
    WaitForSingleObject(window->thread, INFINITE);
    CloseHandle(window->thread);
  }
  if (window->created != NULL) CloseHandle(window->created);
  *window = (test_window){0};
}

static void windows_destroy(test_window* windows, int count) {
  for (int index = 0; index < count; ++index) window_destroy(&windows[index]);
}

#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
typedef struct window_color {
  HWND window;
  COLORREF color;
  COLORREF seen;
} window_color;

static bool window_shows_color(void* context) {
  window_color* expected = context;
  const HDC device_context = GetDC(expected->window);
  expected->seen = GetPixel(device_context, window_size / 2, window_size / 2);
  ReleaseDC(expected->window, device_context);
  return expected->seen == expected->color;
}
#endif

// Waits for the style's red at the window's center, which a driver may draw
// after the present returns. On Windows, GDI can't read a window that has
// Mesa's OpenGL pixel format, so only a Vulkan build reads the window back.
static void expect_red_window(const test_window* window) {
#if defined(MLN_FFI_TEST_BACKEND_VULKAN)
  window_color expected = {
    .window = window->window, .color = RGB(0xff, 0x00, 0x00)
  };
  if (
    mln_test_await(
      window_shows_color, &expected, mln_test_deadline_default(),
      "the window to show red"
    )
  )
    return;
  TEST_ASSERT_EQUAL_HEX32_MESSAGE(
    expected.color, expected.seen, "the color at the window's center"
  );
#else
  (void)window;
#endif
}

static bool window_is_minimized(void* context) {
  return IsIconic(((const test_window*)context)->window) != FALSE;
}

static bool window_is_restored(void* context) {
  return !window_is_minimized(context);
}

static void minimize_window(test_window* window) {
  ShowWindow(window->window, SW_SHOWMINNOACTIVE);
  TEST_ASSERT_TRUE(mln_test_await(
    window_is_minimized, window, mln_test_deadline_default(),
    "the window to minimize"
  ));
}

static void restore_window(test_window* window) {
  ShowWindow(window->window, SW_SHOWNOACTIVATE);
  TEST_ASSERT_TRUE(mln_test_await(
    window_is_restored, window, mln_test_deadline_default(),
    "the window to restore"
  ));
}

// Submits an attach to `window` without waiting for it. The fixture holds the
// session once the call succeeds.
static mln_render_target window_target(HWND window) {
  mln_render_target target = {
    .size = sizeof(mln_render_target),
    .kind = MLN_RENDER_TARGET_KIND_WIN32_WINDOW,
  };
  target.data.win32_window = (mln_win32_window_target){
    .logical_size = {.width = window_size, .height = window_size},
    .window = window,
  };
  return target;
}

static mln_status start_attach(
  mln_map map, HWND window, mln_gpu_device device,
  mln_render_driver_kind driver, mln_test_completion* completion,
  mln_test_render_fixture* fixture
) {
  const mln_render_target target = window_target(window);
  mln_render_session_attach_options options = {
    .size = sizeof(mln_render_session_attach_options), .driver = driver
  };
  options.frame_wake = mln_test_pulse_wake();
  fixture->driver = driver;
  const mln_status status = mln_map_attach(
    map, &target, device, &options, &fixture->session, &completion->descriptor,
    MLN_TEST_DIAGNOSTIC
  );
  if (status != MLN_STATUS_OK) mln_test_completion_reject(completion);
  return status;
}

static void attach(
  mln_map map, const test_window* window, mln_test_render_fixture* fixture
) {
  mln_test_completion completion = mln_test_completion_default(0);
  MLN_TEST_OK(start_attach(
    map, window->window, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER,
    &completion, fixture
  ));
  MLN_TEST_OK(mln_test_render_fixture_finish_operation(fixture, &completion));
  mln_test_completion_destroy(&completion);
}

static mln_render_result render_frame(const mln_test_render_fixture* fixture) {
  mln_test_render_demand_forced(fixture, 0);
  const mln_render_frame_batch batch =
    mln_test_render_wait_for_results(fixture);
  const mln_render_frame_result result = mln_test_render_batch_result(batch, 0);
  mln_render_frame_batch_release(batch);
  return result.disposition;
}

static void set_window(
  const mln_test_render_fixture* fixture, const test_window* window
) {
  const mln_render_target target = window_target(window->window);
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_set_target(
      fixture->session, &target, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
}

static void detach(const mln_test_render_fixture* fixture) {
  MLN_TEST_RENDER_AWAIT(
    MLN_STATUS_OK, fixture,
    mln_render_session_detach(
      fixture->session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    )
  );
}

static void release_session(mln_test_render_fixture* fixture) {
  if (fixture->session != MLN_HANDLE_NULL) {
    (void)mln_test_render_session_release(fixture->session);
  }
  fixture->session = MLN_HANDLE_NULL;
}

// Unity aborts at every assertion after a failed one, back into the case's
// TEST_PROTECT, so a teardown that asserts would run again without end. After
// a failure, the harness reclaims the map and the runtime instead.
static void destroy_map_and_runtime(mln_map map, mln_runtime runtime) {
  if (Unity.CurrentTestFailed) return;
  mln_test_destroy_map(map);
  mln_test_destroy_runtime(runtime);
}

static mln_map create_red_map(mln_runtime runtime) {
  const mln_map map = mln_test_create_map(runtime);
  mln_test_load_style_and_wait(map, mln_test_red_background_style_json);
  return map;
}

// A session keeps its graphics context without a window, and presents to the
// next one, whether it gets that after a release or in place of a window.
static void a_session_presents_to_each_window_that_it_gets(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  test_window windows[3] = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    for (int index = 0; index < 3; ++index) window_create(&windows[index]);
    attach(map, &windows[0], &fixture);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture)
    );
    expect_red_window(&windows[0]);

    MLN_TEST_RENDER_AWAIT(
      MLN_STATUS_OK, &fixture,
      mln_render_session_release_target(
        fixture.session, &completion.descriptor, MLN_TEST_DIAGNOSTIC
      )
    );
    set_window(&fixture, &windows[1]);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture)
    );
    expect_red_window(&windows[1]);

    set_window(&fixture, &windows[2]);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture)
    );
    expect_red_window(&windows[2]);
  }
  release_session(&fixture);
  windows_destroy(windows, 3);
  destroy_map_and_runtime(map, runtime);
}

// A session skips frames while its window is minimized.
static void a_session_waits_out_a_minimized_window_and_detaches(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  test_window windows[1] = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    window_create(&windows[0]);
    attach(map, &windows[0], &fixture);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture)
    );

    minimize_window(&windows[0]);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_TARGET_NOT_READY, render_frame(&fixture)
    );
    restore_window(&windows[0]);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture)
    );
    minimize_window(&windows[0]);
    detach(&fixture);
  }
  release_session(&fixture);
  windows_destroy(windows, 1);
  destroy_map_and_runtime(map, runtime);
}

// The library owns the graphics context, so it drives the session itself.
// A minimized window's surface has no extent, which no swapchain can have.
static void a_minimized_window_is_rejected(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = create_red_map(runtime);
  test_window windows[2] = {0};
  mln_test_render_fixture fixture = {0};
  if (TEST_PROTECT()) {
    window_create(&windows[0]);
    window_create(&windows[1]);
    minimize_window(&windows[0]);
    mln_test_completion rejected = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_INVALID_STATE,
      start_attach(
        map, windows[0].window, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER,
        &rejected, &fixture
      )
    );
    mln_test_completion_destroy(&rejected);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);

    attach(map, &windows[1], &fixture);
    const mln_render_target replacement = window_target(windows[0].window);
    mln_test_completion completion = mln_test_completion_default(0);
    const mln_status replaced = mln_render_session_set_target(
      fixture.session, &replacement, &completion.descriptor, MLN_TEST_DIAGNOSTIC
    );
    if (replaced != MLN_STATUS_OK) mln_test_completion_reject(&completion);
    mln_test_completion_destroy(&completion);
    MLN_TEST_STATUS(MLN_STATUS_INVALID_STATE, replaced);
    TEST_ASSERT_EQUAL_UINT32(
      MLN_RENDER_RESULT_RENDERED, render_frame(&fixture)
    );
  }
  release_session(&fixture);
  windows_destroy(windows, 2);
  destroy_map_and_runtime(map, runtime);
}

static void a_win32_window_rejects_the_callers_graphics_thread(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  test_window windows[1] = {0};
  if (TEST_PROTECT()) {
    window_create(&windows[0]);
    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_UNSUPPORTED,
      start_attach(
        map, windows[0].window, MLN_HANDLE_NULL,
        MLN_RENDER_DRIVER_CALLER_GRAPHICS_THREAD, &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  windows_destroy(windows, 1);
  destroy_map_and_runtime(map, runtime);
}

// A device that presents to Win32 windows says so.
static void the_native_device_reports_win32_windows(void) {
  mln_gpu_device device = MLN_HANDLE_NULL;
  MLN_TEST_OK(mln_gpu_device_create(NULL, &device, MLN_TEST_DIAGNOSTIC));
  mln_gpu_device_capabilities capabilities = {.size = sizeof(capabilities)};
  MLN_TEST_OK(
    mln_gpu_device_get_capabilities(device, &capabilities, MLN_TEST_DIAGNOSTIC)
  );
  mln_gpu_device_release(device);
  TEST_ASSERT_NOT_EQUAL_UINT32(
    0, capabilities.target_kinds & MLN_RENDER_TARGET_KIND_FLAG_WIN32_WINDOW
  );
}

#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
// Windows lets a window set its pixel format only once, so a window that has
// another one can't take the library's.
static void a_window_with_another_pixel_format_is_rejected(void) {
  const mln_runtime runtime = mln_test_create_runtime();
  const mln_map map = mln_test_create_map(runtime);
  test_window windows[1] = {0};
  if (TEST_PROTECT()) {
    window_create(&windows[0]);
    const HDC device_context = GetDC(windows[0].window);
    PIXELFORMATDESCRIPTOR descriptor = {0};
    const int count =
      DescribePixelFormat(device_context, 1, sizeof(descriptor), &descriptor);
    int single_buffered = 0;
    for (int format = 1; format <= count && single_buffered == 0; ++format) {
      DescribePixelFormat(
        device_context, format, sizeof(descriptor), &descriptor
      );
      if (
        (descriptor.dwFlags & PFD_DRAW_TO_WINDOW) != 0 &&
        (descriptor.dwFlags & PFD_DOUBLEBUFFER) == 0
      ) {
        single_buffered = format;
      }
    }
    const BOOL set =
      single_buffered != 0 &&
      SetPixelFormat(device_context, single_buffered, &descriptor);
    ReleaseDC(windows[0].window, device_context);
    TEST_ASSERT_TRUE_MESSAGE(set, "no single-buffered pixel format to set");

    mln_test_render_fixture fixture = {0};
    mln_test_completion completion = mln_test_completion_default(0);
    MLN_TEST_STATUS(
      MLN_STATUS_INVALID_ARGUMENT,
      start_attach(
        map, windows[0].window, MLN_HANDLE_NULL, MLN_RENDER_DRIVER_CORE_WORKER,
        &completion, &fixture
      )
    );
    mln_test_completion_destroy(&completion);
    TEST_ASSERT_EQUAL_UINT64(MLN_HANDLE_NULL, fixture.session);
  }
  windows_destroy(windows, 1);
  destroy_map_and_runtime(map, runtime);
}
#endif

MLN_TEST_GROUP {
  RUN_TEST(a_session_presents_to_each_window_that_it_gets);
  RUN_TEST(a_session_waits_out_a_minimized_window_and_detaches);
  RUN_TEST(a_minimized_window_is_rejected);
  RUN_TEST(a_win32_window_rejects_the_callers_graphics_thread);
  RUN_TEST(the_native_device_reports_win32_windows);
#if defined(MLN_FFI_TEST_BACKEND_OPENGL)
  RUN_TEST(a_window_with_another_pixel_format_is_rejected);
#endif
}
