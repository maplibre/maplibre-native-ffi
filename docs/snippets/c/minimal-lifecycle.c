// Creates a runtime and a map, loads a style, pumps until the style resolves,
// and tears everything down. One thread does all of it: the thread that creates
// the runtime owns the runtime and every map under it.

#include <maplibre_native_c.h>
#include <stdio.h>
#include <time.h>

int main(void) {
  mln_runtime_options runtime_options = mln_runtime_options_default();
  runtime_options.cache_path = ":memory:";

  mln_runtime* runtime = NULL;
  if (mln_runtime_create(&runtime_options, &runtime) != MLN_STATUS_OK) {
    return 1;
  }

  mln_map_options map_options = mln_map_options_default();
  map_options.width = 512;
  map_options.height = 512;
  map_options.scale_factor = 1.0;

  mln_map* map = NULL;
  if (mln_map_create(runtime, &map_options, &map) != MLN_STATUS_OK) {
    mln_runtime_destroy(runtime);
    return 1;
  }

  // Accepting the request is not loading the style. The outcome arrives later,
  // as an event.
  const mln_status requested =
    mln_map_set_style_url(map, "https://tiles.openfreemap.org/styles/bright");
  bool loaded = false;
  bool settled = requested != MLN_STATUS_OK;

  const time_t deadline = time(NULL) + 30;
  while (!settled && time(NULL) < deadline) {
    mln_runtime_run_once(runtime);

    mln_runtime_event event = {.size = sizeof(event)};
    bool has_event = false;
    while (mln_runtime_poll_event(runtime, &event, &has_event) ==
             MLN_STATUS_OK &&
           has_event) {
      if (event.source != map) continue;
      if (event.type == MLN_RUNTIME_EVENT_MAP_STYLE_LOADED) {
        loaded = true;
        settled = true;
      } else if (event.type == MLN_RUNTIME_EVENT_MAP_LOADING_FAILED) {
        if (event.message_size > 0) {
          fprintf(
            stderr, "style failed: %.*s\n", (int)event.message_size,
            event.message
          );
        }
        settled = true;
      }
    }
    // A host with its own event loop waits in it here rather than spinning.
  }

  // Children first: a runtime refuses to close while its maps are live.
  mln_map_destroy(map);
  mln_runtime_destroy(runtime);
  return loaded ? 0 : 1;
}
