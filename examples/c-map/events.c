#include <stdio.h>

#include "events.h"

static Uint32 app_event_type;

app_error app_events_init(void) {
  app_event_type = SDL_RegisterEvents(1);
  return app_event_type == 0 ? APP_ERROR_EVENT_DRAIN_FAILED : APP_OK;
}

bool app_event_code_of(const SDL_Event* event, app_event_code* out_code) {
  if (event->type != app_event_type) return false;
  *out_code = (app_event_code)event->user.code;
  return true;
}

static void push_app_event(app_event_code code) {
  SDL_Event event = {.user = {.type = app_event_type, .code = code}};
  // A native queue wakes again only after its next drain, so a lost push
  // stalls the loop. SDL's queue holds far more events than these wakes post.
  if (!SDL_PushEvent(&event)) {
    fprintf(stderr, "SDL_PushEvent failed: %s\n", SDL_GetError());
  }
}

static void wake_render_loop(void* user_data) {
  push_app_event((app_event_code)(intptr_t)user_data);
}

mln_wake app_event_wake(app_event_code code) {
  return (mln_wake){
    .size = sizeof(mln_wake),
    .callback = wake_render_loop,
    .user_data = (void*)(intptr_t)code,
  };
}

static Uint32 push_timed_app_event(
  void* user_data, [[maybe_unused]] SDL_TimerID timer,
  [[maybe_unused]] Uint32 interval
) {
  push_app_event((app_event_code)(intptr_t)user_data);
  return 0;
}

void app_event_push_after(app_event_code code, Uint32 delay_ms) {
  if (
    SDL_AddTimer(delay_ms, push_timed_app_event, (void*)(intptr_t)code) == 0
  ) {
    fprintf(stderr, "SDL_AddTimer failed: %s\n", SDL_GetError());
  }
}

static void store_completion(
  void* user_data, const mln_completion_result* result
) {
  awaited_completion* completion = user_data;
  completion->status = result->status;
  // Only map creation completes with a value in this example.
  if (result->value != nullptr && result->value_count == 1) {
    completion->map = *(const mln_map*)result->value;
  }
}

/// Signals from the release callback, which runs once native code is done with
/// the completion, so the waiter may destroy it as soon as it wakes.
static void signal_completion(void* user_data) {
  awaited_completion* completion = user_data;
  SDL_SignalSemaphore(completion->signal);
}

app_error awaited_completion_init(
  awaited_completion* completion, mln_completion* out_descriptor
) {
  *completion = (awaited_completion){.signal = SDL_CreateSemaphore(0)};
  if (completion->signal == nullptr) {
    fprintf(stderr, "SDL_CreateSemaphore failed: %s\n", SDL_GetError());
    return APP_ERROR_BACKEND_SETUP_FAILED;
  }
  *out_descriptor = (mln_completion){
    .size = sizeof(mln_completion),
    .callback = store_completion,
    .user_data = completion,
    .release_user_data = signal_completion,
  };
  return APP_OK;
}

void awaited_completion_deinit(awaited_completion* completion) {
  SDL_DestroySemaphore(completion->signal);
  completion->signal = nullptr;
}

bool awaited_completion_wait(
  awaited_completion* completion, Sint32 timeout_ms
) {
  return SDL_WaitSemaphoreTimeout(completion->signal, timeout_ms);
}
