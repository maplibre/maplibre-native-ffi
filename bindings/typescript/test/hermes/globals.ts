// The standard globals that the suite uses and Hermes lacks. The binding
// itself needs none of them: strings convert in C, and AwaitableWake.next()
// takes a signal only when one is passed.

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
  encode(text = ""): Uint8Array {
    const bytes: number[] = [];
    for (const character of text) {
      const point = character.codePointAt(0)!;
      if (point < 0x80) {
        bytes.push(point);
      } else if (point < 0x800) {
        bytes.push(0xc0 | (point >> 6), 0x80 | (point & 0x3f));
      } else if (point < 0x10000) {
        bytes.push(
          0xe0 | (point >> 12),
          0x80 | ((point >> 6) & 0x3f),
          0x80 | (point & 0x3f),
        );
      } else {
        bytes.push(
          0xf0 | (point >> 18),
          0x80 | ((point >> 12) & 0x3f),
          0x80 | ((point >> 6) & 0x3f),
          0x80 | (point & 0x3f),
        );
      }
    }
    return new Uint8Array(bytes);
  }
}

scope.AbortController ??= Controller;
scope.TextEncoder ??= Encoder;
