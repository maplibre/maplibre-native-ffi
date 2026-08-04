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
import {
  type CallbackRegistry,
  ProviderRegistration,
  RuleTable,
} from "./internal/callbacks.ts";
import { decodeEvent } from "./internal/event-decode.ts";
import { HandleState } from "./internal/handle.ts";
import type { Native } from "./internal/native.ts";
import { asInt64, asRawEnum, asUint64 } from "./internal/numbers.ts";
import { attachHandleState, mapForId } from "./internal/private.ts";
import { readQueuedRequest } from "./internal/queued-request.ts";
import type { Ptr } from "./internal/transport.ts";
import { Map, type MapOptions } from "./map.ts";
import {
  type AmbientCacheOperation,
  type OfflineOperationId,
  type OfflineRegion,
  OfflineRegionDefinitionType,
} from "./offline.ts";
import { EP } from "./raw/entrypoints.ts";
import type { ResourceRequest } from "./resource-request.ts";
import {
  ANY_RESOURCE_KIND,
  type ResourceRewriteRule,
  type ResourceRoute,
  routeFlags,
} from "./resources.ts";

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

  private constructor(native: Native, id: bigint) {
    this.#state = new HandleState(native, "WakeSource", id);
    this.#state.watchForLeaks(this);
    attachHandleState(this, this.#state);
  }

  /** Builds a wrapper for an id the caller just acquired. */
  static own(native: Native, id: bigint): WakeSource {
    return new WakeSource(native, id);
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
  transfer(): WakeSourceTransfer {
    const native = this.#state.native;
    const id = this.#state.use("WakeSource.transfer");
    const token = native.transport.transferIssue(id);
    if (token === 0n) {
      throw new MaplibreError(
        "invalidState",
        "no transfer token was available; a host may hold at most 256 unclaimed transfers",
      );
    }
    try {
      // The wrapper stops being an owner here rather than when the receiver
      // adopts, so the sending context cannot keep using it meanwhile.
      this.#state.close(() => {});
    } catch (error) {
      // Nothing owns the handle if this fails, so the token goes back before
      // the failure surfaces.
      native.transport.transferDiscard(token);
      throw error;
    }
    return WakeSourceTransfer.issue(native, token);
  }

  /** Adopts a wake source another context transferred. */
  static adopt(
    native: Native,
    carrier: WakeSourceTransfer | ArrayBuffer,
  ): WakeSource {
    const token =
      carrier instanceof WakeSourceTransfer
        ? carrier.take()
        : readCarrierToken(carrier);
    const id = native.transport.transferClaim(token);
    if (id === 0n) {
      throw new MaplibreError(
        "invalidArgument",
        "this carrier names no unclaimed transfer, so it was already adopted or discarded",
      );
    }
    try {
      return new WakeSource(native, id);
    } catch (error) {
      // The claim already consumed the token, so a wrapper that cannot be built
      // would leave the handle with no owner at all.
      native.scope((scope) => {
        native.raw(scope, EP.mln_wake_source_destroy, [id]);
      });
      throw error;
    }
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

/**
 * A wake source in transit between host contexts.
 *
 * The carrier holds a token the native library issued rather than the handle
 * id, so a host that copies its bytes still cannot produce a second owner. It is
 * owned: a transfer nobody adopts is discarded rather than leaving the wake
 * source with no owner, and a carrier the host drops reports the loss.
 */
export class WakeSourceTransfer {
  readonly #native: Native;
  #token: bigint;

  private constructor(native: Native, token: bigint) {
    this.#native = native;
    this.#token = token;
  }

  /** @internal */
  static issue(native: Native, token: bigint): WakeSourceTransfer {
    const transfer = new WakeSourceTransfer(native, token);
    abandonedTransfers.register(transfer, { native, token }, transfer);
    return transfer;
  }

  /** The bytes a host posts to the receiving context. */
  get bytes(): ArrayBuffer {
    if (this.#token === 0n) {
      throw new MaplibreError(
        "closedHandle",
        "this transfer was already consumed",
      );
    }
    const carrier = new ArrayBuffer(8);
    new DataView(carrier).setBigUint64(0, this.#token, true);
    return carrier;
  }

  /** @internal Consumes the token, so exactly one adoption can use it. */
  take(): bigint {
    const token = this.#token;
    if (token === 0n) {
      throw new MaplibreError(
        "closedHandle",
        "this transfer was already consumed",
      );
    }
    this.#token = 0n;
    abandonedTransfers.unregister(this);
    return token;
  }

  /**
   * Cancels a transfer nobody adopted, releasing the wake source.
   *
   * A transfer that is never posted, or whose receiver never adopts it, would
   * otherwise hold the handle with no owner to close it.
   */
  discard(): void {
    if (this.#token === 0n) {
      return;
    }
    const token = this.take();
    releaseTransferredWakeSource(this.#native, token);
  }
}

/** Releases a wake source whose transfer nobody claimed. */
function releaseTransferredWakeSource(native: Native, token: bigint): void {
  const id = native.transport.transferDiscard(token);
  if (id === 0n) {
    return;
  }
  native.scope((scope) => {
    native.raw(scope, EP.mln_wake_source_destroy, [id]);
  });
}

/**
 * Recovers a transfer the host dropped.
 *
 * The handle has no owner at this point, and the wake source is not
 * thread-affine, so releasing it here is safe where releasing a runtime would
 * not be.
 */
const abandonedTransfers = new FinalizationRegistry<{
  native: Native;
  token: bigint;
}>(({ native, token }) => {
  releaseTransferredWakeSource(native, token);
});

function readCarrierToken(carrier: ArrayBuffer): bigint {
  if (carrier.byteLength !== 8) {
    throw new MaplibreError(
      "invalidInput",
      "a wake source carrier is eight bytes",
    );
  }
  return new DataView(carrier).getBigUint64(0, true);
}

export class Runtime {
  readonly #state: HandleState;
  readonly #eventStorage: Ptr;
  #rewriteRules: RuleTable | undefined;
  #provider: ProviderRegistration | undefined;
  readonly #hasEventStorage: Ptr;

  readonly #callbacks: CallbackRegistry;

  private constructor(native: Native, id: bigint, callbacks: CallbackRegistry) {
    this.#state = new HandleState(native, "Runtime", id);
    this.#state.watchForLeaks(this);
    this.#callbacks = callbacks;
    attachHandleState(this, this.#state);
    // Poll storage belongs to the runtime rather than to each call: the C API
    // fills the same shape every time, and a per-call allocation would churn
    // through the allocator for the hottest call in the binding.
    const layout = native.layout("mln_runtime_event");
    this.#eventStorage = native.memory.allocate(layout.size);
    this.#hasEventStorage = native.memory.allocate(1);
  }

  /** @internal */
  static create(
    native: Native,
    callbacks: CallbackRegistry,
    options: RuntimeOptions = {},
  ): Runtime {
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
    try {
      return new Runtime(native, id, callbacks);
    } catch (error) {
      // The native handle exists and nothing owns it, so it is released here
      // rather than left for a leak report that names no wrapper.
      native.scope((scope) => {
        native.raw(scope, EP.mln_runtime_destroy, [id]);
      });
      throw error;
    }
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
      timeout === "untilWake"
        ? -1n
        : asInt64(BigInt(Math.trunc(timeout)), "pump timeout");
    this.#state.native.scope((scope) => {
      this.#state.native.checked(scope, EP.mln_runtime_pump, [
        id,
        BigInt.asUintN(64, milliseconds),
      ]);
    });
    // A pump is where a host returns from waiting, so it is also where records
    // MapLibre's threads queued while it waited become deliveries. A transport
    // that can wake this context has already scheduled a drain, and draining
    // again finds nothing; one that cannot — WebAssembly, whose workers reach
    // no other agent — delivers here or not at all.
    this.#callbacks.drain();
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
    return decodeEvent(native, this.#eventStorage, (id) => mapForId(this, id));
  }

  /**
   * Creates a map owned by this runtime.
   *
   * The map retains the runtime, so closing a runtime with live maps fails
   * rather than leaving them naming a released object.
   */
  createMap(options: MapOptions): Map {
    this.#state.use("Runtime.createMap");
    return Map.create(this, this.#state.native, this.#callbacks, options);
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
    return WakeSource.own(native, source);
  }

  /**
   * Rewrites resource URLs through a table of rules.
   *
   * MapLibre asks what a URL should become on its own threads, with the answer
   * due at once, so the rules are evaluated in native code rather than handed to
   * JavaScript. The first matching rule replaces the URL; a request that matches
   * nothing is unchanged.
   *
   * Installing a table replaces the previous one.
   */
  setResourceRewriteRules(rules: readonly ResourceRewriteRule[]): void {
    const id = this.#state.use("Runtime.setResourceRewriteRules");
    const native = this.#state.native;
    const table = RuleTable.rewriteRules(
      native,
      rules.map((rule) => ({
        kind: rule.kind?.rawValue ?? ANY_RESOURCE_KIND,
        url: rule.url,
        ...(rule.replacementUrl !== undefined && {
          replacementUrl: rule.replacementUrl,
        }),
      })),
    );
    try {
      native.scope((scope) => {
        const layout = native.layout("mln_resource_transform");
        const transform = scope.allocateZeroed(layout.size, layout.align);
        native.memory
          .view(transform, layout.size)
          .setUint32(layout.fields.size!.offset, layout.size, true);
        native.memory.writePointer(
          (transform + BigInt(layout.fields.callback!.offset)) as Ptr,
          native.transport.symbol(
            EP.mln_adapter_resource_transform_rewrite_callback,
          ),
        );
        native.memory.writePointer(
          (transform + BigInt(layout.fields.user_data!.offset)) as Ptr,
          table.table,
        );
        native.checked(scope, EP.mln_runtime_set_resource_transform, [
          id,
          transform,
        ]);
      });
    } catch (error) {
      table.release();
      throw error;
    }
    // The C call has returned, so native code no longer reads the old table.
    this.#rewriteRules?.release();
    this.#rewriteRules = table;
  }

  /**
   * Serves resources this host claims.
   *
   * The routes are evaluated in native code the moment MapLibre asks, so a
   * request no route matches passes straight through to the native loader. A
   * claimed request is copied and handed to the handler on this execution
   * context, and the handler answers it whenever it can.
   *
   * Installing a provider replaces the previous one.
   */
  setResourceProvider(
    routes: readonly ResourceRoute[],
    handler: (request: ResourceRequest) => void,
  ): void {
    const id = this.#state.use("Runtime.setResourceProvider");
    const native = this.#state.native;
    const registration = new ProviderRegistration(
      native,
      this.#callbacks,
      routes.map((route) => ({
        kind: route.kind?.rawValue ?? ANY_RESOURCE_KIND,
        flags: routeFlags(route),
        url: route.url,
      })),
      (record) => {
        handler(readQueuedRequest(native, record));
      },
    );
    try {
      native.scope((scope) => {
        const layout = native.layout("mln_resource_provider");
        const provider = scope.allocateZeroed(layout.size, layout.align);
        native.memory
          .view(provider, layout.size)
          .setUint32(layout.fields.size!.offset, layout.size, true);
        native.memory.writePointer(
          (provider + BigInt(layout.fields.callback!.offset)) as Ptr,
          native.transport.symbol(
            EP.mln_adapter_queued_resource_provider_callback,
          ),
        );
        native.memory.writePointer(
          (provider + BigInt(layout.fields.user_data!.offset)) as Ptr,
          registration.provider,
        );
        native.checked(scope, EP.mln_runtime_set_resource_provider, [
          id,
          provider,
        ]);
      });
    } catch (error) {
      registration.retire();
      throw error;
    }
    this.#retireProvider();
    this.#provider = registration;
  }

  /** Stops serving resources from this host. */
  clearResourceProvider(): void {
    const id = this.#state.use("Runtime.clearResourceProvider");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_runtime_clear_resource_provider, [id]);
    });
    this.#retireProvider();
  }

  /**
   * Retires the installed provider.
   *
   * The C call that replaced or cleared it has returned, so native code can no
   * longer reach the registration. Asking the adapter to deliver its retirement
   * record is what releases the host state behind it, after the records already
   * queued for it drain.
   */
  #retireProvider(): void {
    const provider = this.#provider;
    if (provider === undefined) {
      return;
    }
    this.#provider = undefined;
    const native = this.#state.native;
    native.scope((scope) => {
      native.raw(scope, EP.mln_adapter_queued_resource_provider_retire, [
        provider.provider,
      ]);
    });
  }

  /** Stops rewriting resource URLs. */
  clearResourceRewriteRules(): void {
    const id = this.#state.use("Runtime.clearResourceRewriteRules");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_runtime_clear_resource_transform, [id]);
    });
    this.#rewriteRules?.release();
    this.#rewriteRules = undefined;
  }

  /**
   * Starts listing the offline regions in this runtime's database.
   *
   * Offline work runs against a database, so it is a command: this reports that
   * the operation was accepted, its completion arrives as an
   * `offlineOperationCompleted` event naming the id, and the result is taken
   * afterwards.
   */
  startOfflineRegionList(): OfflineOperationId {
    const id = this.#state.use("Runtime.startOfflineRegionList");
    const native = this.#state.native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_runtime_offline_regions_list_start, [
        id,
        out,
      ]);
      return native.memory.view(out, 8).getBigUint64(0, true);
    });
  }

  /**
   * Takes the result of a completed region list.
   *
   * Ownership transfers once. A take that fails leaves the operation there, so
   * a caller may retry it.
   */
  takeOfflineRegionList(
    operation: OfflineOperationId,
  ): readonly OfflineRegion[] {
    const id = this.#state.use("Runtime.takeOfflineRegionList");
    const native = this.#state.native;
    return native.scope((scope) => {
      const outList = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_runtime_offline_regions_list_take_result, [
        id,
        asUint64(operation, "operation id"),
        outList,
      ]);
      const list = native.memory.view(outList, 8).getBigUint64(0, true);
      try {
        return readOfflineRegions(native, list);
      } finally {
        // The list handle is this call's to release, on every path.
        native.scope((inner) => {
          native.raw(inner, EP.mln_offline_region_list_destroy, [list]);
        });
      }
    });
  }

  /** Starts an ambient cache operation, reporting its id. */
  startAmbientCacheOperation(
    operation: AmbientCacheOperation,
  ): OfflineOperationId {
    const id = this.#state.use("Runtime.startAmbientCacheOperation");
    const native = this.#state.native;
    return native.scope((scope) => {
      const out = scope.allocateZeroed(8);
      native.checked(scope, EP.mln_runtime_run_ambient_cache_operation_start, [
        id,
        BigInt(asRawEnum(operation.rawValue, "ambient cache operation")),
        out,
      ]);
      return native.memory.view(out, 8).getBigUint64(0, true);
    });
  }

  /** Discards an operation whose result the host will not take. */
  discardOfflineOperation(operation: OfflineOperationId): void {
    const id = this.#state.use("Runtime.discardOfflineOperation");
    const native = this.#state.native;
    native.scope((scope) => {
      native.checked(scope, EP.mln_runtime_offline_operation_discard, [
        id,
        asUint64(operation, "operation id"),
      ]);
    });
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
    // Storage is returned only once the native handle is gone, so a failed
    // destroy leaves the runtime usable for a retry.
    this.#retireProvider();
    this.#rewriteRules?.release();
    this.#rewriteRules = undefined;
    native.memory.free(this.#eventStorage);
    native.memory.free(this.#hasEventStorage);
  }

  get isClosed(): boolean {
    return this.#state.isClosed;
  }
}

/** Copies every region out of a list the C API handed back. */
function readOfflineRegions(
  native: Native,
  list: bigint,
): readonly OfflineRegion[] {
  return native.scope((scope) => {
    const outCount = scope.allocateZeroed(8);
    native.checked(scope, EP.mln_offline_region_list_count, [list, outCount]);
    const count = native.readSize(outCount);

    const layout = native.layout("mln_offline_region_info");
    const definition = native.layout("mln_offline_region_definition");
    const info = scope.allocateZeroed(layout.size, layout.align);
    const regions: OfflineRegion[] = [];
    for (let index = 0; index < count; index += 1) {
      native.memory.bytes(info, layout.size).fill(0);
      native.memory
        .view(info, layout.size)
        .setUint32(layout.fields.size!.offset, layout.size, true);
      native.checked(scope, EP.mln_offline_region_list_get, [
        list,
        BigInt(index),
        info,
      ]);
      const view = native.memory.view(info, layout.size);
      const metadataPointer = native.memory.readPointer(
        (info + BigInt(layout.fields.metadata!.offset)) as Ptr,
      );
      const metadataSize = native.readSize(
        (info + BigInt(layout.fields.metadata_size!.offset)) as Ptr,
      );
      regions.push({
        id: view.getBigInt64(layout.fields.id!.offset, true),
        definitionType: OfflineRegionDefinitionType.fromRawValue(
          view.getUint32(
            layout.fields.definition!.offset + definition.fields.type!.offset,
            true,
          ),
        ),
        // The bytes belong to the list, which this call releases, so they are
        // copied before it does.
        metadata: native.foreignBytes(metadataPointer, metadataSize),
      });
    }
    return regions;
  });
}
