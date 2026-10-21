// The standard globals that the suite uses and ArkTS lacks. The binding itself needs none of them: strings convert in
// C, and AwaitableWake.next() takes a signal only when one is passed.

import util from "@ohos.util";

const scope = globalThis as Record<string, unknown>;

class Signal {
  aborted = false;
  reason: unknown = undefined;
  readonly listeners = new Set<() => void>();

  addEventListener(_type: "abort", listener: () => void): void {
    this.listeners.add(listener);
  }

  removeEventListener(_type: "abort", listener: () => void): void {
    this.listeners.delete(listener);
  }
}

class Controller {
  readonly signal = new Signal();

  abort(reason: unknown = new Error("This operation was aborted")): void {
    if (this.signal.aborted) {
      return;
    }
    this.signal.aborted = true;
    this.signal.reason = reason;
    const listeners = [...this.signal.listeners];
    this.signal.listeners.clear();
    for (const listener of listeners) {
      listener();
    }
  }
}

class Encoder {
  readonly #encoder = new util.TextEncoder();

  encode(text = ""): Uint8Array {
    return this.#encoder.encodeInto(text);
  }
}

scope.AbortController ??= Controller;
scope.TextEncoder ??= Encoder;
scope.performance ??= { now: () => Date.now() };
