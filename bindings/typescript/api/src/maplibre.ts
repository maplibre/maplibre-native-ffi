/**
 * The loaded library, and the process-global operations that belong to it.
 *
 * An application declares this package and one runtime payload package. The
 * payload carries a compiled artifact for one target and render backend; this
 * package owns every public name.
 */

import { MaplibreError } from "./errors.ts";
import { NamedValue } from "./events.ts";
import type { LatLng, ProjectedMeters } from "./geo.ts";
import { CallbackRegistry, LogRegistration } from "./internal/callbacks.ts";
import { Native } from "./internal/native.ts";
import {
  type NodeApiAddon,
  nodeApiTransport,
} from "./internal/node-transport.ts";
import { asRawEnum, asUint32 } from "./internal/numbers.ts";
import { attachCallbackRegistry, attachNative } from "./internal/private.ts";
import type { Ptr, Transport } from "./internal/transport.ts";
import {
  LogEvent,
  LogSeverityMask,
  type LogCallbackOptions,
  type LogRecord,
  LogSeverity,
} from "./logging.ts";
import { EP } from "./raw/entrypoints.ts";
import { MLN_NETWORK_STATUS, MLN_RENDER_BACKEND_FLAG } from "./raw/enums.ts";
import {
  Runtime,
  type RuntimeOptions,
  WakeSource,
  WakeSourceTransfer,
} from "./runtime.ts";

/** Whether MapLibre may start network requests. */
export class NetworkStatus extends NamedValue {
  static readonly online = new NetworkStatus(
    MLN_NETWORK_STATUS.MLN_NETWORK_STATUS_ONLINE,
    "online",
  );
  static readonly offline = new NetworkStatus(
    MLN_NETWORK_STATUS.MLN_NETWORK_STATUS_OFFLINE,
    "offline",
  );

  static fromRawValue(rawValue: number): NetworkStatus {
    return (
      [NetworkStatus.online, NetworkStatus.offline].find(
        (value) => value.rawValue === rawValue,
      ) ?? new NetworkStatus(rawValue, `unknown(${rawValue})`)
    );
  }
}

/** The render backends a native build compiled in. */
export interface RenderBackends {
  readonly metal: boolean;
  readonly vulkan: boolean;
  readonly opengl: boolean;
  readonly webgpu: boolean;
}

/**
 * The runtime payload packages this facade knows how to load.
 *
 * A payload is named for the target and the public render backend it carries,
 * because MapLibre Native compiles exactly one renderer per build. Discovery
 * tries each in turn; a checkout's locally staged payload comes last, so an
 * installed one wins over a development one.
 */
const KNOWN_RUNTIME_PACKAGES = [
  "@maplibre/native-ffi-runtime-linux-x64-vulkan",
  "@maplibre/native-ffi-runtime-linux-x64-opengl",
  "@maplibre/native-ffi-runtime-linux-arm64-vulkan",
  "@maplibre/native-ffi-runtime-linux-arm64-opengl",
  "@maplibre/native-ffi-runtime-macos-arm64-metal",
  "@maplibre/native-ffi-runtime-macos-arm64-vulkan",
  "@maplibre/native-ffi-runtime-macos-arm64-opengl",
  "@maplibre/native-ffi-runtime-windows-x64-vulkan",
  "@maplibre/native-ffi-runtime-windows-x64-opengl",
  "@maplibre/native-ffi-runtime-windows-arm64-vulkan",
  "@maplibre/native-ffi-runtime-windows-arm64-opengl",
  "@maplibre/native-ffi-runtime-android-arm64-vulkan",
  "@maplibre/native-ffi-runtime-android-arm64-opengl",
  "@maplibre/native-ffi-runtime-arkts",
  "@maplibre/native-ffi-runtime-ohos-arm64-vulkan",
  "@maplibre/native-ffi-runtime-ohos-arm64-opengl",
  "@maplibre/native-ffi-runtime-node",
] as const;

export interface LoadOptions {
  /**
   * Loads this runtime payload package instead of discovering one.
   *
   * Required when more than one compatible payload is installed: the binding
   * never picks between them, and never changes transports after a requested
   * payload fails to load.
   */
  readonly runtimePackage?: string;
}

export class Maplibre {
  readonly #native: Native;
  readonly #callbacks: CallbackRegistry;
  #logRegistration: LogRegistration | undefined;

  private constructor(native: Native) {
    this.#native = native;
    attachNative(this, native);
    this.#callbacks = new CallbackRegistry(native);
    attachCallbackRegistry(this, this.#callbacks);
    // Records arrive on MapLibre's threads and wait until this context can run
    // them. The signal is installed once and drains everything queued for this
    // context, leaving every other context in the process still able to be
    // woken for its own.
    this.#callbacks.startNotifications();
  }

  /**
   * Installs the process-global log callback.
   *
   * Log records are copied and delivered on this execution context, so the
   * callback runs after MapLibre has already logged. `consume` is the answer
   * this registration reports for every record, because a decision that arrives
   * later is no decision at all.
   *
   * Installing a second callback replaces the first. The old registration keeps
   * receiving nothing new, and its state is released once native code can no
   * longer reach it.
   */
  setLogCallback(
    callback: (record: LogRecord) => void,
    options: LogCallbackOptions = {},
  ): void {
    const native = this.#native;
    const registration = new LogRegistration(
      native,
      this.#callbacks,
      (record) => {
        callback(readLogRecord(native, record));
      },
      options.consume ?? false,
    );
    try {
      native.scope((scope) => {
        native.checked(scope, EP.mln_adapter_log_set_callback, [
          registration.statePointer,
        ]);
      });
    } catch (error) {
      // Installation failed, so the previous callback is still the active one
      // and this registration's state goes back now.
      registration.abandon();
      throw error;
    }
    this.#logRegistration = registration;
  }

  /**
   * Controls which log severities MapLibre may dispatch asynchronously.
   *
   * An asynchronous record reaches the callback after the code that logged it
   * has moved on, so errors stay synchronous by default.
   */
  setAsyncLogSeverities(mask: LogSeverityMask): void {
    this.#native.scope((scope) => {
      this.#native.checked(scope, EP.mln_log_set_async_severity_mask, [
        BigInt(asUint32(mask.rawValue, "log severity mask")),
      ]);
    });
  }

  /** Clears the process-global log callback. */
  clearLogCallback(): void {
    if (this.#logRegistration === undefined) {
      return;
    }
    this.#native.scope((scope) => {
      this.#native.checked(scope, EP.mln_adapter_log_set_callback, [0n]);
    });
    this.#logRegistration = undefined;
  }

  /**
   * Delivers every callback record waiting for this context.
   *
   * Delivery is normally driven by the transport's own signal. A host that
   * blocks its context — inside a pump, for instance — calls this to see what
   * arrived while it was blocked.
   */
  deliverCallbacks(): void {
    this.#callbacks.drain();
  }

  /** Reports how many callback records are waiting for this context. */
  get pendingCallbackCount(): number {
    return this.#callbacks.pendingCount;
  }

  /**
   * Releases what this context holds in the loaded library.
   *
   * The library itself stays loaded, because a process shares one copy of it
   * and another host context may still be using it. What goes is this
   * context's own place in it: its log callback, the signal that wakes it, and
   * the queue its records were waiting in. Records still queued for it are
   * released rather than left outstanding, since nothing will drain them.
   *
   * Closing twice succeeds. A runtime this context created is closed on its
   * own, before this: its registrations name native state that this call
   * cannot prove is unreachable.
   */
  close(): void {
    if (this.#callbacks.isClosed) {
      return;
    }
    // The log callback is this facade's own registration, so it goes here. The
    // clear queues the registration's retirement, and the drain delivers it, so
    // the state it owns is released rather than left behind.
    this.clearLogCallback();
    this.#callbacks.drain();
    this.#callbacks.close();
  }

  /** Reports whether this context has released its place in the library. */
  get isClosed(): boolean {
    return this.#callbacks.isClosed;
  }

  /**
   * Loads the installed runtime payload.
   *
   * Discovery imports the payload packages this facade knows. One compatible
   * payload loads; several require an explicit choice.
   */
  static async load(options: LoadOptions = {}): Promise<Maplibre> {
    const candidates =
      options.runtimePackage === undefined
        ? [...KNOWN_RUNTIME_PACKAGES]
        : [options.runtimePackage];

    const found: { name: string; addon: NodeApiAddon }[] = [];
    const failures: string[] = [];
    for (const name of candidates) {
      try {
        const payload = (await import(/* @vite-ignore */ name)) as {
          addon: NodeApiAddon;
        };
        found.push({ name, addon: payload.addon });
      } catch (error) {
        failures.push(
          `${name}: ${error instanceof Error ? error.message : String(error)}`,
        );
      }
    }

    if (found.length === 0) {
      throw new MaplibreError(
        "invalidState",
        `no MapLibre Native runtime payload could be loaded (${failures.join("; ")})`,
      );
    }
    if (found.length > 1) {
      throw new MaplibreError(
        "invalidState",
        `several runtime payloads are installed (${found.map((entry) => entry.name).join(", ")}); ` +
          "name one through loadOptions.runtimePackage",
      );
    }
    return Maplibre.fromTransport(nodeApiTransport(found[0]!.addon));
  }

  /** @internal Wraps a transport a test or another loader built. */
  static fromTransport(transport: Transport): Maplibre {
    const native = new Native(transport);
    checkAbiVersion(native);
    return new Maplibre(native);
  }

  /** The C ABI contract version this library reports. */
  get cVersion(): number {
    return this.#native.scope((scope) =>
      Number(this.#native.raw(scope, EP.mln_c_version, []) & 0xffff_ffffn),
    );
  }

  /** The render backends this native build compiled in. */
  get renderBackends(): RenderBackends {
    const mask = this.#native.scope((scope) =>
      Number(
        this.#native.raw(scope, EP.mln_supported_render_backend_mask, []) &
          0xffff_ffffn,
      ),
    );
    return {
      metal:
        (mask & MLN_RENDER_BACKEND_FLAG.MLN_RENDER_BACKEND_FLAG_METAL) !== 0,
      vulkan:
        (mask & MLN_RENDER_BACKEND_FLAG.MLN_RENDER_BACKEND_FLAG_VULKAN) !== 0,
      opengl:
        (mask & MLN_RENDER_BACKEND_FLAG.MLN_RENDER_BACKEND_FLAG_OPENGL) !== 0,
      webgpu:
        (mask & MLN_RENDER_BACKEND_FLAG.MLN_RENDER_BACKEND_FLAG_WEBGPU) !== 0,
    };
  }

  /** Reports whether MapLibre may start network requests. */
  getNetworkStatus(): NetworkStatus {
    return this.#native.scope((scope) => {
      const out = scope.allocateZeroed(4);
      this.#native.checked(scope, EP.mln_network_status_get, [out]);
      return NetworkStatus.fromRawValue(
        this.#native.memory.view(out, 4).getUint32(0, true),
      );
    });
  }

  /** Sets whether MapLibre may start network requests. */
  setNetworkStatus(status: NetworkStatus): void {
    this.#native.scope((scope) => {
      this.#native.checked(scope, EP.mln_network_status_set, [
        BigInt(asRawEnum(status.rawValue, "network status")),
      ]);
    });
  }

  /**
   * Creates a runtime owned by the calling host context.
   *
   * Each owner thread may hold one live runtime.
   */
  createRuntime(options?: RuntimeOptions): Runtime {
    return Runtime.create(this.#native, this.#callbacks, options);
  }

  /**
   * Where a coordinate lands in Web Mercator meters.
   *
   * This conversion depends on no map, so it needs none.
   */
  projectedMetersForLatLng(coordinate: LatLng): ProjectedMeters {
    const native = this.#native;
    return native.scope((scope) => {
      const input = native.layout("mln_lat_lng");
      const coordinateStorage = scope.allocateZeroed(input.size, input.align);
      const coordinateView = native.memory.view(coordinateStorage, input.size);
      coordinateView.setFloat64(
        input.fields.latitude!.offset,
        coordinate.latitude,
        true,
      );
      coordinateView.setFloat64(
        input.fields.longitude!.offset,
        coordinate.longitude,
        true,
      );
      const output = native.layout("mln_projected_meters");
      const metersStorage = scope.allocateZeroed(output.size, output.align);
      native.checked(scope, EP.mln_projected_meters_for_lat_lng, [
        coordinateStorage,
        metersStorage,
      ]);
      const view = native.memory.view(metersStorage, output.size);
      return {
        northing: view.getFloat64(output.fields.northing!.offset, true),
        easting: view.getFloat64(output.fields.easting!.offset, true),
      };
    });
  }

  /** Which coordinate a position in Web Mercator meters names. */
  latLngForProjectedMeters(meters: ProjectedMeters): LatLng {
    const native = this.#native;
    return native.scope((scope) => {
      const input = native.layout("mln_projected_meters");
      const metersStorage = scope.allocateZeroed(input.size, input.align);
      const metersView = native.memory.view(metersStorage, input.size);
      metersView.setFloat64(
        input.fields.northing!.offset,
        meters.northing,
        true,
      );
      metersView.setFloat64(input.fields.easting!.offset, meters.easting, true);
      const output = native.layout("mln_lat_lng");
      const coordinateStorage = scope.allocateZeroed(output.size, output.align);
      native.checked(scope, EP.mln_lat_lng_for_projected_meters, [
        metersStorage,
        coordinateStorage,
      ]);
      const view = native.memory.view(coordinateStorage, output.size);
      return {
        latitude: view.getFloat64(output.fields.latitude!.offset, true),
        longitude: view.getFloat64(output.fields.longitude!.offset, true),
      };
    });
  }

  /** Adopts a wake source another host context transferred. */
  adoptWakeSource(carrier: WakeSourceTransfer | ArrayBuffer): WakeSource {
    return WakeSource.adopt(this.#native, carrier);
  }

  /**
   * The C API's diagnostic for the last failing call on this thread.
   *
   * Errors already carry the diagnostic that belonged to them, so this is for
   * host code that called native functions another way.
   */
  get lastErrorMessage(): string {
    return this.#native.scope((scope) => {
      const pointer = this.#native.raw(
        scope,
        EP.mln_thread_last_error_message,
        [],
      );
      return this.#native.transport.readForeignCString(pointer) ?? "";
    });
  }
}

/**
 * The C ABI contract version this binding was generated against.
 *
 * The value is zero while the ABI is unstable and increments on each SemVer
 * major release, so a payload reporting another one describes a library this
 * package cannot call.
 */
const EXPECTED_C_VERSION = 0;

function checkAbiVersion(native: Native): void {
  const reported = native.scope((scope) =>
    Number(native.raw(scope, EP.mln_c_version, []) & 0xffff_ffffn),
  );
  if (reported !== EXPECTED_C_VERSION) {
    throw new MaplibreError(
      "abiMismatch",
      `the installed runtime reports C ABI version ${reported}, and this package ` +
        `was built against ${EXPECTED_C_VERSION}`,
    );
  }
}

/** Copies one adapter-owned log record. */
function readLogRecord(native: Native, record: Ptr): LogRecord {
  const layout = native.layout("mln_adapter_log_record");
  const bytes = native.transport.readForeign(record, layout.size);
  const view = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  const fields = layout.fields;
  const messagePointer =
    native.transport.pointerSize === 8
      ? view.getBigUint64(fields.message!.offset, true)
      : BigInt(view.getUint32(fields.message!.offset, true));
  return {
    severity: LogSeverity.fromRawValue(
      view.getUint32(fields.severity!.offset, true),
    ),
    event: LogEvent.fromRawValue(view.getUint32(fields.event!.offset, true)),
    code: view.getBigInt64(fields.code!.offset, true),
    message: native.transport.readForeignCString(messagePointer as Ptr) ?? "",
  };
}
