// Carries native wakes to the SDL event loop. Every native callback only
// pushes an SDL event; the render loop does the work when it dispatches it.

#ifndef C_MAP_EVENTS_H
#define C_MAP_EVENTS_H

#include <SDL3/SDL.h>
#include <maplibre_native_c.h>

#include "types.h"

/// The work an app event asks the render loop to do.
typedef enum app_event_code : int32_t {
  /// The runtime event queue has events to drain.
  APP_EVENT_RUNTIME_EVENTS,
  /// The render session has frame results to drain.
  APP_EVENT_FRAME_RESULTS,
  /// The render session has caller-driver work to service.
  APP_EVENT_DRIVER_WORK,
  /// A paced retry after a frame that could not reach the screen.
  APP_EVENT_RETRY_FRAME,
  /// A smoke run waited too long for its first frame.
  APP_EVENT_SMOKE_TIMEOUT,
} app_event_code;

/// Registers the SDL event type app events use. Call once after SDL_Init.
[[nodiscard]] app_error app_events_init(void);

/// Returns whether event is an app event, writing its code when it is.
bool app_event_code_of(const SDL_Event* event, app_event_code* out_code);

/// A wake that pushes an app event with code from any native thread.
mln_wake app_event_wake(app_event_code code);

/// Pushes an app event with code after delay_ms.
void app_event_push_after(app_event_code code, Uint32 delay_ms);

/// One submission's completion that the submitting thread blocks on, for
/// startup and shutdown steps that cannot continue without it.
typedef struct awaited_completion {
  SDL_Semaphore* signal;
  mln_status status;
  /// The map a map creation completes with.
  mln_map map;
} awaited_completion;

/// Prepares completion and returns the descriptor to submit with it.
[[nodiscard]] app_error awaited_completion_init(
  awaited_completion* completion, mln_completion* out_descriptor
);
void awaited_completion_deinit(awaited_completion* completion);

/// Blocks for at most timeout_ms, or without limit for -1, and reports whether
/// the completion ran.
bool awaited_completion_wait(awaited_completion* completion, Sint32 timeout_ms);

#endif  // C_MAP_EVENTS_H
