// node:timers for the browser bundle. A zero-delay timer runs after the tasks
// that the page has queued, such as the module's posts from its threads.

export function setImmediate(callback: () => void): unknown {
  return setTimeout(callback, 0);
}
