// App shell: command-line parsing, the SDL window, and the render loop.

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diagnostics.h"
#include "input.h"
#include "map_state.h"
#include "render/render.h"
#include "render_request.h"
#include "types.h"
#include "util.h"
#include "viewport.h"

typedef struct notification_receiver {
  atomic_bool scheduled;
  Uint32 event_type;
} notification_receiver;

/// Native callbacks only schedule receiver work. The SDL event loop performs
/// every C API drain later on the render-loop thread.
static void schedule_notification_drain(void* user_data) {
  notification_receiver* receiver = user_data;
  if (
    atomic_exchange_explicit(&receiver->scheduled, true, memory_order_acq_rel)
  ) {
    return;
  }
  SDL_Event event = {.type = receiver->event_type};
  if (!SDL_PushEvent(&event)) {
    atomic_store_explicit(&receiver->scheduled, false, memory_order_release);
  }
}

static app_error render_loop_iteration(
  SDL_Window* window, render_target* target, viewport* current_viewport,
  map_state* state, render_request* request, notification_receiver* receiver,
  input_controller* controller, bool* running
) {
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    if (event.type == receiver->event_type) {
      atomic_store_explicit(&receiver->scheduled, false, memory_order_release);
      bool render_update = false;
      MAP_TRY(map_state_drain_notifications(state, &render_update));
      if (render_update) {
        render_request_set(request);
      }
      continue;
    }

    switch (event.type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        *running = false;
        break;
      case SDL_EVENT_WINDOW_RESIZED:
      case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
      case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
        *current_viewport = viewport_get(window);
        viewport_log("resized viewport", *current_viewport);
        MAP_TRY(map_state_resize(state, *current_viewport));
        MAP_TRY(render_target_resize(target, *current_viewport));
        render_request_set(request);
        break;
      default: {
        const input_result result = input_controller_handle_event(
          controller, &event, state, *current_viewport
        );
        if (result.error != APP_OK) {
          return result.error;
        }
        if (result.camera_changed) {
          render_request_set(request);
        }
        break;
      }
    }
  }

  // Consume before rendering, so a request published during the render call is
  // not discarded.
  if (render_request_consume(request)) {
    bool rendered = false;
    MAP_TRY(render_target_render_update(target, *current_viewport, &rendered));
    if (!rendered) {
      render_request_set(request);
    }
  }
  MAP_TRY(render_target_finish_frame(target));

  // Stand-in for a display-refresh subscription.
  sleep_milliseconds(8);
  return APP_OK;
}

static app_error render_loop(
  SDL_Window* window, render_target_mode mode, render_target* target,
  viewport* current_viewport, map_state* state, render_request* request,
  notification_receiver* receiver
) {
  app_error error = render_target_attach(target, state->map, *current_viewport);
  if (error != APP_OK) {
    return error;
  }

  printf("render target: %s\n", render_target_mode_label(mode));
  printf("render target status: %s\n", render_target_mode_status_line(mode));
  input_log_controls();
  render_request_set(request);

  bool running = true;
  input_controller controller = {};
  while (running) {
    void* frame_scope = render_target_frame_scope_open();
    error = render_loop_iteration(
      window, target, current_viewport, state, request, receiver, &controller,
      &running
    );
    render_target_frame_scope_close(frame_scope);
    if (error != APP_OK) {
      return error;
    }
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
  if (error != APP_OK) {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
    return EXIT_FAILURE;
  }

  mln_log_set_callback(diagnostics_log_record, nullptr, nullptr);
  int exit_code = EXIT_FAILURE;
  render_target_apply_sdl_hints();
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    goto out_log_callback;
  }
  error = render_target_configure_video();
  if (error != APP_OK) {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
    goto out_sdl;
  }

  const SDL_WindowFlags window_flags = render_target_window_flags() |
                                       SDL_WINDOW_RESIZABLE |
                                       SDL_WINDOW_HIGH_PIXEL_DENSITY;
  SDL_Window* window = SDL_CreateWindow(
    "MapLibre SDL3 Map", viewport_window_width, viewport_window_height,
    window_flags
  );
  if (window == nullptr) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    goto out_sdl;
  }
  SDL_RaiseWindow(window);

  viewport current_viewport = viewport_get(window);
  viewport_log("initial viewport", current_viewport);
  render_target* target = nullptr;
  error = render_target_init(&target, window, current_viewport, mode);
  if (error != APP_OK) {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
    goto out_window;
  }

  notification_receiver receiver = {
    .event_type = SDL_RegisterEvents(1),
  };
  atomic_init(&receiver.scheduled, false);
  if (receiver.event_type == 0) {
    error = APP_ERROR_EVENT_DRAIN_FAILED;
    goto out_target;
  }

  map_state state;
  error = map_state_init(
    &state, current_viewport, schedule_notification_drain, &receiver
  );
  if (error != APP_OK) {
    goto out_target;
  }

  render_request request;
  render_request_init(&request);
  error = render_loop(
    window, mode, target, &current_viewport, &state, &request, &receiver
  );

  // The graphics-thread-affine session closes before its map and runtime.
  render_target_deinit(target);
  target = nullptr;
  map_state_deinit(&state);

  if (error == APP_OK) {
    exit_code = EXIT_SUCCESS;
  } else {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
  }
  goto out_window;

out_target:
  render_target_deinit(target);
out_window:
  SDL_DestroyWindow(window);
out_sdl:
  SDL_Quit();
out_log_callback:
  mln_log_clear_callback();
  return exit_code;
}
