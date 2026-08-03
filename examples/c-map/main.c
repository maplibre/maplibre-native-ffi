// The app shell: command-line parsing, the SDL window, the two loops on their
// two threads, and the shutdown ordering between them.

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "channel.h"
#include "diagnostics.h"
#include "input.h"
#include "map_state.h"
#include "render/render.h"
#include "types.h"
#include "util.h"
#include "viewport.h"

/// Backstop for the runtime loop's park. The render loop's wake source is what
/// normally releases it, so this only bounds a pump that nothing signals.
static constexpr int64_t park_timeout_milliseconds = 100;

typedef struct runtime_loop_args {
  viewport initial_viewport;
  command_queue* commands;
  render_request* request;
  map_channel* channel;
} runtime_loop_args;

static app_error runtime_loop_body(runtime_loop_args* args, map_state* state) {
  // The render loop signals this to release the parked pump, so a camera
  // command or a shutdown request lands without waiting out the bound below.
  mln_wake_source wake = MLN_HANDLE_NULL;
  const mln_status wake_status =
    mln_runtime_wake_source_acquire(state->runtime, &wake);
  if (wake_status != MLN_STATUS_OK) {
    diagnostics_log_status("wake source acquire failed", wake_status);
    return APP_ERROR_WAKE_SOURCE_FAILED;
  }

  // Reused across drains, so applying a batch allocates nothing.
  command_list batch = {};

  map_channel_publish(args->channel, state->map, wake);

  app_error error = APP_OK;
  app_error failure = APP_OK;
  while (!map_channel_shutdown_requested(args->channel) &&
         !map_channel_failure(args->channel, &failure)) {
    error = map_state_apply_commands(state, args->commands, &batch);
    if (error != APP_OK) {
      break;
    }
    // This thread has no display to pace it, so it takes its cadence from the
    // runtime's own work and parks in between. The bound is a backstop for
    // work that queues nothing on the owner thread, not the cadence.
    const mln_status pump_status =
      mln_runtime_pump(state->runtime, park_timeout_milliseconds);
    if (pump_status != MLN_STATUS_OK) {
      diagnostics_log_status("runtime pump failed", pump_status);
      error = APP_ERROR_RUNTIME_PUMP_FAILED;
      break;
    }
    bool render_update = false;
    error = map_state_drain_events(state, &render_update);
    if (error != APP_OK) {
      break;
    }
    if (render_update) {
      render_request_set(args->request);
    }
  }

  command_list_deinit(&batch);
  mln_wake_source_destroy(wake);
  return error;
}

/// Owns the runtime and the map for their whole lifetime, on a thread that is
/// not the one presenting. It never touches the render session: the render
/// loop attaches its own against the map published here.
static int runtime_loop(void* userdata) {
  runtime_loop_args* args = userdata;

  map_state state;
  app_error error = map_state_init(&state, args->initial_viewport);
  if (error != APP_OK) {
    map_channel_fail(args->channel, error);
    return 0;
  }

  error = runtime_loop_body(args, &state);
  if (error != APP_OK) {
    map_channel_fail(args->channel, error);
  }

  // However this loop exits, the render loop still owns the session, and a
  // map with an attached session cannot be destroyed. The body publishes any
  // failure before this point, so the render loop stops, closes its session,
  // and requests shutdown; only then is the map destroyed.
  map_channel_await_shutdown(args->channel);
  map_state_deinit(&state);
  return 0;
}

/// The display-paced render loop. Owns the window, input, and the render
/// session once it adopts it.
static app_error render_loop(
  SDL_Window* window, render_target_mode mode, render_target* target,
  viewport* current_viewport, command_queue* commands, render_request* request,
  map_channel* channel
) {
  // The runtime loop creates the map; this loop attaches its own session
  // against it and owns that session for the rest of the run.
  mln_map map = MLN_HANDLE_NULL;
  while (!map_channel_try_map(channel, &map)) {
    app_error failure;
    if (map_channel_failure(channel, &failure)) {
      return failure;
    }
    sleep_milliseconds(1);
  }
  app_error error = render_target_attach(target, map, *current_viewport);
  if (error != APP_OK) {
    return error;
  }

  printf("render target: %s\n", render_target_mode_label(mode));
  printf("render target status: %s\n", render_target_mode_status_line(mode));
  input_log_controls();

  bool running = true;
  input_controller controller = {};
  while (running) {
    app_error failure;
    if (map_channel_failure(channel, &failure)) {
      return failure;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_EVENT_QUIT:
        case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
          running = false;
          break;
        case SDL_EVENT_WINDOW_RESIZED:
        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        case SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED:
          *current_viewport = viewport_get(window);
          viewport_log("resized viewport", *current_viewport);
          // Every mode follows a resize without losing its session: the ones
          // the session sizes resize in place, and a caller-owned target
          // allocates a replacement and hands it over.
          error = render_target_resize(target, *current_viewport);
          if (error != APP_OK) {
            return error;
          }
          render_request_set(request);
          break;
        default: {
          const input_result result = input_controller_handle_event(
            &controller, &event, commands, *current_viewport
          );
          if (result.handled) {
            // Release the runtime loop's parked pump so the command just
            // queued is applied on this frame rather than after the parking
            // bound.
            map_channel_wake_runtime_loop(channel);
          }
          if (result.camera_changed) {
            render_request_set(request);
          }
          break;
        }
      }
    }

    error = render_target_finish_frame(target);
    if (error != APP_OK) {
      return error;
    }

    // Consume before rendering, so a request the runtime loop publishes
    // during the render call is not discarded.
    if (render_request_consume(request)) {
      bool rendered = false;
      error = render_target_render_update(target, *current_viewport, &rendered);
      if (error != APP_OK) {
        return error;
      }
      if (!rendered) {
        render_request_set(request);
      }
    }

    // Stand-in for a display-refresh subscription until the host loop is
    // display-paced; see the frame loop section of the example spec.
    sleep_milliseconds(8);
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

  mln_log_set_callback(diagnostics_log_record, nullptr);

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

  // The graphics context, the render session, and every presentation resource
  // belong to this thread, which owns the window and its display callbacks.
  render_target* target = nullptr;
  error = render_target_init(&target, window, current_viewport, mode);
  if (error != APP_OK) {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
    goto out_window;
  }

  command_queue commands;
  command_queue_init(&commands);
  render_request request;
  render_request_init(&request);
  map_channel channel;
  map_channel_init(&channel);

  runtime_loop_args args = {
    .initial_viewport = current_viewport,
    .commands = &commands,
    .request = &request,
    .channel = &channel,
  };
  thrd_t runtime_thread;
  if (thrd_create(&runtime_thread, runtime_loop, &args) != thrd_success) {
    fprintf(
      stderr, "c-map failed: %s\n",
      app_error_name(APP_ERROR_THREAD_SPAWN_FAILED)
    );
    render_target_deinit(target);
    goto out_channels;
  }

  error = render_loop(
    window, mode, target, &current_viewport, &commands, &request, &channel
  );

  // Destroy the session before the runtime loop destroys the map: a map with
  // an attached session cannot be destroyed.
  render_target_deinit(target);
  map_channel_request_shutdown(&channel);
  thrd_join(runtime_thread, nullptr);

  app_error failure = APP_OK;
  if (error == APP_OK && map_channel_failure(&channel, &failure)) {
    error = failure;
  }
  if (error == APP_OK) {
    exit_code = EXIT_SUCCESS;
  } else {
    fprintf(stderr, "c-map failed: %s\n", app_error_name(error));
  }

out_channels:
  map_channel_deinit(&channel);
  command_queue_deinit(&commands);
out_window:
  SDL_DestroyWindow(window);
out_sdl:
  SDL_Quit();
out_log_callback:
  mln_log_clear_callback();
  return exit_code;
}
