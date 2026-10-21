import { hold, unhold } from "./realm.ts";

interface Waiter {
  resolve(): void;
  reject(reason: unknown): void;
  cleanup(): void;
}

/**
 * The part of an `AbortSignal` that {@link AwaitableWake.next} reads. It is
 * declared here so that the package's types name no host global.
 */
export interface AwaitableWakeSignal {
  readonly aborted: boolean;
  readonly reason: unknown;
  addEventListener(
    type: "abort",
    listener: () => void,
    options?: { readonly once?: boolean },
  ): void;
  removeEventListener(type: "abort", listener: () => void): void;
}

/** Options for {@link AwaitableWake.next}. */
export interface AwaitableWakeOptions {
  /** Rejects the wait with the signal's reason when it aborts. */
  readonly signal?: AwaitableWakeSignal;
}

/**
 * Turns a wake callback into Promises.
 *
 * Pass {@link AwaitableWake.wake} as any wake field, such as a render
 * session's frame wake, or call it from a callback, such as a runtime's event
 * handler. {@link AwaitableWake.next} resolves at the next wake. A wake that
 * arrives while nothing waits is kept for the next call. A pending wait holds
 * the event loop open, as every pending binding Promise does, while the wake
 * callback alone keeps nothing alive. Draining stays explicit: a resolved wait
 * means the receiver has work, which the caller then drains.
 */
export class AwaitableWake {
  #latched = false;
  readonly #waiters = new Set<Waiter>();

  /** The wake callback, which the realm runs when native code wakes it. */
  readonly wake: () => void = () => {
    if (this.#waiters.size === 0) {
      this.#latched = true;
      return;
    }
    const waiters = [...this.#waiters];
    this.#waiters.clear();
    for (const waiter of waiters) {
      waiter.cleanup();
      waiter.resolve();
    }
  };

  /** Resolves at the next wake after the previous wait resolved. */
  next(options: AwaitableWakeOptions = {}): Promise<void> {
    const { signal } = options;
    if (signal?.aborted) {
      return Promise.reject(signal.reason);
    }
    if (this.#latched) {
      this.#latched = false;
      return Promise.resolve();
    }
    return new Promise<void>((resolve, reject) => {
      const abort = () => {
        this.#waiters.delete(waiter);
        waiter.cleanup();
        reject(signal!.reason);
      };
      let held = true;
      const waiter: Waiter = {
        resolve,
        reject,
        cleanup: () => {
          signal?.removeEventListener("abort", abort);
          if (held) {
            held = false;
            unhold();
          }
        },
      };
      hold();
      this.#waiters.add(waiter);
      signal?.addEventListener("abort", abort, { once: true });
    });
  }
}
