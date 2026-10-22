// node:timers for the Hermes bundle, from the host's globals.

export function setImmediate(callback: () => void): unknown {
  return globalThis.setImmediate(callback);
}
