/**
 * The runtime, which owns MapLibre's owner-thread scheduler state.
 *
 * A runtime is thread-affine: the host context that creates it pumps it, polls
 * its events, and closes it. Pumping with a non-zero timeout parks that context,
 * so a host that cannot block runs the runtime on a worker and pumps with a zero
 * timeout elsewhere.
 */

import { MaplibreError } from "./errors.ts";
import type { RuntimeEvent } from "./events.ts";
import { decodeEvent } from "./internal/event-decode.ts";
import { HandleState } from "./internal/handle.ts";
import type { Native } from "./internal/native.ts";
import type { Ptr } from "./internal/transport.ts";
import { EP } from "./raw/entrypoints.ts";

/** How a runtime is configured at creation. */
export interface RuntimeOptions {
  /** Filesystem root for `asset://` URLs. */
  readonly assetPath?: string;
  /** Cache database path. */
  readonly cachePath?: string;
}

/**
 * How long a pump may park the calling context.
 *
 * Zero drains and returns. A positive number of milliseconds parks for up to
 * that long, then drains. `"untilWake"` parks until a wake arrives.
 */
export type PumpTimeout = number | "untilWake";

/**
 * Releases a parked runtime.
 *
 * A wake source holds its own reference to the runtime's wake state, so it stays
 * usable after the runtime closes and the two are released in either order. It
 * is safe to move to another host context through {@link transfer}.
 */
export class WakeSource {
  readonly #state: HandleState;

  /** @internal */
  constructor(native: Native, id: bigint) {
    this.#state = new HandleState(native, "WakeSource", id);
    this.#state.watchForLeaks(this);
  }

  /**
   * Sets the runtime's wake flag, releasing a parked pump.
   *
   * Signalling after the runtime closes succeeds and does nothing.
   */
  signal(): void {
    const id = this.#state.use("WakeSource.signal");
    this.#state.native.scope((scope) => {
      this.#state.native.checked(scope, EP.mln_wake_source_signal, [id]);
    });
  }

  /**
   * Hands this wake source to another host context.
   *
   * The carrier holds a token the native library issues, not the handle id: a
   * handle id is copyable data, so a receiver that copied it would become a
   * second owner. Exactly one `adopt` succeeds, whatever a host does with the
   * carrier, and this wrapper is closed for further use as soon as the token is
   * issued.
   */
  transfer(): ArrayBuffer {
    const id = this.#state.use("WakeSource.transfer");
    const token = this.#state.native.transport.transferIssue(id);
    if (token === 0n) {
      throw new MaplibreError(
        "invalidState",
        "no transfer token was available; a host may hold at most 256 unclaimed transfers",
      );
    }
    // The wrapper stops being an owner here rather than when the receiver
    // adopts, so the sending context cannot keep using it in the meantime.
    this.#state.close(() => {});
    const carrier = new ArrayBuffer(8);
    new DataView(carrier).setBigUint64(0, token, true);
    return carrier;
  }

  /** Adopts a wake source another context transferred. */
  static adopt(native: Native, carrier: ArrayBuffer): WakeSource {
    if (carrier.byteLength !== 8) {
      throw new MaplibreError(
        "invalidInput",
        "a wake source carrier is eight bytes",
      );
    }
    const token = new DataView(carrier).getBigUint64(0, true);
    const id = native.transport.transferClaim(token);
    if (id === 0n) {
      throw new MaplibreError(
        "invalidArgument",
        "this carrier names no unclaimed transfer, so it was already adopted or discarded",
      );
    }
    return new WakeSource(native, id);
  }

  /** Releases the wake source. Closing twice succeeds. */
  close(): void {
    this.#state.close((id) => {
      this.#state.native.scope((scope) => {
        this.#state.native.raw(scope, EP.mln_wake_source_destroy, [id]);
      });
    });
  }

  get isClosed(): boolean {
    return this.#state.isClosed;
  }
}

export class Runtime {
  readonly #state: HandleState;
  readonly #eventStorage: Ptr;
  readonly #hasEventStorage: Ptr;

  /** @internal */
  constructor(native: Native, id: bigint) {
    this.#state = new HandleState(native, "Runtime", id);
    this.#state.watchForLeaks(this);
    // Poll storage belongs to the runtime rather than to each call: the C API
    // fills the same shape every time, and a per-call allocation would churn
    // through the allocator for the hottest call in the binding.
    const layout = native.layout("mln_runtime_event");
    this.#eventStorage = native.memory.allocate(layout.size);
    this.#hasEventStorage = native.memory.allocate(1);
  }

  /** @internal */
  static create(native: Native, options: RuntimeOptions = {}): Runtime {
    const id = native.scope((scope) => {
      const layout = native.layout("mln_runtime_options");
      const storage = scope.allocateZeroed(layout.size);
      native.structValue(scope, EP.mln_runtime_options_default, storage);
      if (options.assetPath !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.asset_path!.offset)) as Ptr,
          native.cString(scope, options.assetPath, "assetPath"),
        );
      }
      if (options.cachePath !== undefined) {
        native.memory.writePointer(
          (storage + BigInt(layout.fields.cache_path!.offset)) as Ptr,
          native.cString(scope, options.cachePath, "cachePath"),
        );
      }
      const outRuntime = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_runtime_create, [storage, outRuntime]);
      return native.memory.view(outRuntime, 8).getBigUint64(0, true);
    });
    return new Runtime(native, id);
  }

  /**
   * Advances the runtime, then returns.
   *
   * A pump drains every task queued when it begins plus the work those tasks
   * enqueue, so its duration follows the work it finds. Poll events after every
   * return.
   */
  pump(timeout: PumpTimeout = 0): void {
    if (timeout !== "untilWake" && (!Number.isFinite(timeout) || timeout < 0)) {
      throw new MaplibreError(
        "invalidInput",
        `a pump timeout is zero, a positive number of milliseconds, or "untilWake", not ${timeout}`,
      );
    }
    const id = this.#state.use("Runtime.pump");
    const milliseconds =
      timeout === "untilWake" ? -1n : BigInt(Math.trunc(timeout));
    this.#state.native.scope((scope) => {
      this.#state.native.checked(scope, EP.mln_runtime_pump, [
        id,
        BigInt.asUintN(64, milliseconds),
      ]);
    });
  }

  /** Takes one queued event, or `undefined` when the queue is empty. */
  pollEvent(): RuntimeEvent | undefined {
    const id = this.#state.use("Runtime.pollEvent");
    const native = this.#state.native;
    const layout = native.layout("mln_runtime_event");
    native.memory.bytes(this.#eventStorage, layout.size).fill(0);
    native.memory
      .view(this.#eventStorage, layout.size)
      .setUint32(layout.fields.size!.offset, layout.size, true);
    native.memory.bytes(this.#hasEventStorage, 1).fill(0);
    native.scope((scope) => {
      native.checked(scope, EP.mln_runtime_poll_event, [
        id,
        this.#eventStorage,
        this.#hasEventStorage,
      ]);
    });
    if (native.memory.bytes(this.#hasEventStorage, 1)[0] === 0) {
      return undefined;
    }
    return decodeEvent(native, this.#eventStorage);
  }

  /** Acquires a wake source that releases this runtime's parked owner thread. */
  acquireWakeSource(): WakeSource {
    const id = this.#state.use("Runtime.acquireWakeSource");
    const native = this.#state.native;
    const source = native.scope((scope) => {
      const outSource = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_runtime_wake_source_acquire, [
        id,
        outSource,
      ]);
      return native.memory.view(outSource, 8).getBigUint64(0, true);
    });
    return new WakeSource(native, source);
  }

  /** Releases the runtime. Closing twice succeeds. */
  close(): void {
    if (this.#state.isClosed) {
      return;
    }
    const native = this.#state.native;
    this.#state.close((id) => {
      native.scope((scope) => {
        native.checked(scope, EP.mln_runtime_destroy, [id]);
      });
    });
    // Poll storage is returned only once the native handle is gone, so a failed
    // destroy leaves the runtime usable for a retry.
    native.memory.free(this.#eventStorage);
    native.memory.free(this.#hasEventStorage);
  }

  get isClosed(): boolean {
    return this.#state.isClosed;
  }

  /** @internal The state child handles retain. */
  get state(): HandleState {
    return this.#state;
  }
}
