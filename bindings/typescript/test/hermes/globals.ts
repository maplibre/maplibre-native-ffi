// Host facilities used by the shared suite, outside the engine's ECMAScript
// builtins. The binding itself needs only the host scheduler.
const scope = globalThis as typeof globalThis & {
  __mlnPrint: (message: string) => void;
  __mlnTimeout: (callback: () => void, milliseconds: number) => number;
  __mlnCancelTimeout: (id: number) => void;
  __mlnUtf8: (text: string) => number[];
};

const print = (...values: unknown[]) =>
  scope.__mlnPrint(values.map(String).join(" "));
scope.console = {
  log: print,
  info: print,
  warn: print,
  error: print,
} as Console;
scope.setTimeout = ((callback: () => void, milliseconds = 0) =>
  scope.__mlnTimeout(callback, milliseconds)) as unknown as typeof setTimeout;
scope.clearTimeout = ((id: number) =>
  scope.__mlnCancelTimeout(id)) as typeof clearTimeout;
scope.queueMicrotask = (callback: () => void) => {
  void Promise.resolve().then(callback);
};

class Signal {
  aborted = false;
  reason: unknown;
  readonly listeners = new Set<() => void>();
  addEventListener(_type: string, listener: () => void): void {
    this.listeners.add(listener);
  }
  removeEventListener(_type: string, listener: () => void): void {
    this.listeners.delete(listener);
  }
}
class Controller {
  readonly signal = new Signal();
  abort(reason: unknown = new Error("This operation was aborted")): void {
    if (this.signal.aborted) return;
    this.signal.aborted = true;
    this.signal.reason = reason;
    const listeners = [...this.signal.listeners];
    this.signal.listeners.clear();
    for (const listener of listeners) listener();
  }
}
class Encoder {
  encode(text = ""): Uint8Array {
    return Uint8Array.from(scope.__mlnUtf8(text));
  }
}
scope.AbortController ??= Controller as unknown as typeof AbortController;
scope.TextEncoder ??= Encoder as unknown as typeof TextEncoder;
scope.performance ??= { now: () => Date.now() } as Performance;
