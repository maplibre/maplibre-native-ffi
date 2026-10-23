// node:timers for the ArkTS bundle. ArkTS runs native finalizers and
// threadsafe-function calls from work that the event loop polls for, and a
// zero-delay timer can run before that poll. A turn instead waits for an empty
// async work item, whose completion the loop also polls for.
//
// A turn must not wait on a timer. The application's main thread runs its
// event loop from a timer task that OpenHarmony skips reposting when the next
// due time equals the last one it posted. A task that fires before the loop's
// coarse clock reaches that time leaves no task behind, and no timer runs
// again until something else wakes the loop. A turn's own work wakes it.

import testGraphics from "libmaplibre_native_ffi_test_graphics.so";

const addon = testGraphics as { turn(callback: () => void): void };

export function setImmediate(callback: () => void): unknown {
  addon.turn(callback);
  return undefined;
}
