// App shell: command-line parsing, the SDL window, and the render loop.
//
// The render loop only reacts to SDL events. Input submits camera commands,
// and native wakes post app events: a runtime event drain demands a frame for
// each map update, and a frame-result drain shows what the session rendered.
// A caller-driver session also gets driver wakes, which service it.

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"
#include "events.h"
#include "input.h"
#include "map_state.h"
#include "render/render.h"
#include "types.h"
#include "util.h"
#include "viewport.h"

/// How long a smoke run waits for its first rendered frame.
static const Uint32 smoke_timeout_milliseconds = 60000;

/// How long the loop waits before it retries a frame that did not reach the
/// window, about one display refresh. No map-update event prompts that retry.
static const Uint32 frame_retry_milliseconds = 16;

/// How long the render loop waits before SDL checks for a quit signal.
static const Sint32 signal_check_milliseconds = 250;

typedef struct app {
  SDL_Window* window;
  render_target* target;
  map_state map;
  viewport viewport;
  input_controller input;
  bool smoke;
  bool running;
} app;

/// Smoke mode, selected by MLN_EXAMPLE_SMOKE=1, renders one frame of an inline
/// style into a hidden window and exits, so CI can run the example without a
/// display or network.
static bool smoke_mode(void) {
  const char* value = getenv("MLN_EXAMPLE_SMOKE");
  return value != nullptr && strcmp(value, "1") == 0;
}

static app_error show_frame_results(app* app) {
  render_session* session = render_target_session(app->target);
  frame_results results;
  MAP_TRY(render_session_drain_results(session, &results));
  bool presented = false;
  if (results.rendered) {
    MAP_TRY(render_target_present(app->target, app->viewport, &presented));
  }
  if (presented && app->smoke) {
    puts("smoke: rendered a frame");
    app->running = false;
    return APP_OK;
  }
  if (results.target_not_ready || (results.rendered && !presented)) {
    // Neither a target that was not ready nor a frame that missed the window
    // causes a map-update event, so the retry waits about one display
    // refresh. It forces the frame, because a frame that missed the window
    // consumed its update.
    app_event_push_after(APP_EVENT_RETRY_FRAME, frame_retry_milliseconds);
  } else if (results.needs_repaint) {
    MAP_TRY(render_session_request_frame(session, false));
  }
  return APP_OK;
}

static app_error handle_app_event(app* app, app_event_code code) {
  render_session* session = render_target_session(app->target);
  switch (code) {
    case APP_EVENT_RUNTIME_EVENTS: {
      bool render_update = false;
      MAP_TRY(map_state_drain_events(&app->map, &render_update));
      return render_update ? render_session_request_frame(session, false)
                           : APP_OK;
    }
    case APP_EVENT_DRIVER_WORK:
      return render_session_service(session);
    case APP_EVENT_TARGET_REPLACED:
      return render_target_retire_replaced(app->target);
    case APP_EVENT_FRAME_RESULTS:
      return show_frame_results(app);
    case APP_EVENT_RETRY_FRAME:
      return render_session_request_frame(session, true);
    case APP_EVENT_SMOKE_TIMEOUT:
      return APP_ERROR_SMOKE_FRAME_TIMED_OUT;
  }
  return APP_OK;
}

static app_error handle_event(app* app, const SDL_Event* event) {
  app_event_code code;
  if (app_event_code_of(event, &code)) {
    return handle_app_event(app, code);
  }
  switch (event->type) {
    case SDL_EVENT_QUIT:
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
      app->running = false;
      return APP_OK;
    case SDL_EVENT_WINDOW_RESIZED:
    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
    case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED: {
      // One window change raises several of these events.
      const viewport resized = viewport_get(app->window);
      if (viewport_equal(resized, app->viewport)) return APP_OK;
      app->viewport = resized;
      viewport_log("resized viewport", resized);
      // The session resize carries the new logical extent to the map, and a
      // later resize supersedes an earlier one that has not applied yet.
      return render_target_resize(app->target, resized);
    }
    default:
      return input_controller_handle_event(
        &app->input, event, &app->map, app->viewport
      );
  }
}

static app_error render_loop(app* app, render_target_mode mode) {
  MAP_TRY(render_target_attach(app->target, app->map.map, app->viewport));

  render_session* session = render_target_session(app->target);
  printf("render target: %s\n", render_target_mode_label(mode));
  printf("render target status: %s\n", render_target_mode_status_line(mode));
  printf("render driver: %s\n", render_driver_label(session->driver));
  input_log_controls();

  if (app->smoke) {
    app_event_push_after(APP_EVENT_SMOKE_TIMEOUT, smoke_timeout_milliseconds);
  }
  // Updates the map published before attachment have no event left to demand
  // their frame.
  MAP_TRY(render_session_request_frame(session, false));
  app->running = true;
  while (app->running) {
    SDL_Event event;
    // SDL turns SIGINT and SIGTERM into a quit event only when it next pumps
    // events, so the wait wakes now and then to let it.
    if (!SDL_WaitEventTimeout(&event, signal_check_milliseconds)) continue;
    void* frame_scope = render_target_frame_scope_open();
    const app_error error = handle_event(app, &event);
    render_target_frame_scope_close(frame_scope);
    MAP_TRY(error);
  }
  return APP_OK;
}

static app_error validate_native_render_backend(void) {
  static const struct {
    uint32_t flag;
    const char* name;
  } backends[] = {
    {MLN_RENDER_BACKEND_FLAG_METAL, "metal"},
    {MLN_RENDER_BACKEND_FLAG_OPENGL, "opengl"},
    {MLN_RENDER_BACKEND_FLAG_VULKAN, "vulkan"},
    {MLN_RENDER_BACKEND_FLAG_WEBGPU, "webgpu"},
  };

  const uint32_t support = mln_supported_render_backend_mask();
  printf("native render backends:");
  bool has_backend = false;
  for (size_t i = 0; i < sizeof(backends) / sizeof(backends[0]); i += 1) {
    if ((support & backends[i].flag) != 0) {
      printf(has_backend ? ",%s" : " %s", backends[i].name);
      has_backend = true;
    }
  }
  puts(has_backend ? "" : " none");

  if ((support & render_target_backend_flag()) == 0) {
    return APP_ERROR_RENDER_BACKEND_MISMATCH;
  }
  return APP_OK;
}

static void print_usage(FILE* stream) {
  fputs(
    "Usage: c-map <mode>\n"
    "\n"
    "Modes:\n"
    "  owned-texture     session-owned texture render target\n"
    "  borrowed-texture  caller-owned texture render target\n"
    "  native-surface    native surface render target\n",
    stream
  );
}

/// Runs the example once SDL is up: the window, the map, and the render loop,
/// then tears them down in reverse order.
static app_error run(render_target_mode mode) {
  MAP_TRY(app_events_init());
  MAP_TRY(render_target_configure_video());

  app app = {.smoke = smoke_mode()};
  const SDL_WindowFlags window_flags =
    render_target_window_flags() | SDL_WINDOW_RESIZABLE |
    SDL_WINDOW_HIGH_PIXEL_DENSITY | (app.smoke ? SDL_WINDOW_HIDDEN : 0);
  app.window = SDL_CreateWindow(
    "MapLibre SDL3 Map", viewport_window_width, viewport_window_height,
    window_flags
  );
  if (app.window == nullptr) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  if (!app.smoke) {
    SDL_RaiseWindow(app.window);
  }

  app.viewport = viewport_get(app.window);
  viewport_log("initial viewport", app.viewport);
  app_error error =
    render_target_init(&app.target, app.window, app.viewport, mode);
  if (error == APP_OK) {
    error = map_state_init(&app.map, app.viewport, app.smoke);
    if (error == APP_OK) {
      error = render_loop(&app, mode);
      // The session detaches before its map and runtime close.
      render_target_deinit(app.target);
      map_state_deinit(&app.map);
    } else {
      render_target_deinit(app.target);
    }
  }
  SDL_DestroyWindow(app.window);
  return error;
}

int main(int argc, char** argv) {
  if (argc == 2 && strcmp(argv[1], "--help") == 0) {
    print_usage(stdout);
    return EXIT_SUCCESS;
  }
  render_target_mode mode;
  if (
    argc != 2 || argv[1][0] == '-' || !render_target_mode_parse(argv[1], &mode)
  ) {
    print_usage(stderr);
    return EXIT_FAILURE;
  }

  app_error error = validate_native_render_backend();
  if (error == APP_OK) {
    const mln_log_handler log_handler = {
      .size = sizeof(log_handler),
      .callback = diagnostics_log_record,
    };
    mln_log_set_callback(&log_handler, NULL);
    render_target_apply_sdl_hints();
    if (SDL_Init(SDL_INIT_VIDEO)) {
      error = run(mode);
      SDL_Quit();
    } else {
      fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      error = APP_ERROR_BACKEND_SETUP_FAILED;
    }
    mln_log_clear_callback(NULL);
  }
  if (error != APP_OK) {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
