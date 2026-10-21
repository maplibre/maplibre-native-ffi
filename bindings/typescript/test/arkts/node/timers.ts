// node:timers for the ArkTS bundle. ArkTS runs native finalizers and
// threadsafe-function calls from work that the event loop polls for, and a
// zero-delay timer can run before that poll, so a turn waits on a 1 ms timer.

export function setImmediate(callback: () => void): unknown {
  return setTimeout(callback, 1);
}
