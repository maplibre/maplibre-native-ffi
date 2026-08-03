/**
 * The loaded library, and the process-global operations that belong to it.
 *
 * An application declares this package and one runtime payload package. The
 * payload carries a compiled artifact for one target and render backend; this
 * package owns every public name.
 */

import { MaplibreError } from "./errors.ts";
import { NamedValue } from "./events.ts";
import { Native } from "./internal/native.ts";
import {
  type NodeApiAddon,
  nodeApiTransport,
} from "./internal/node-transport.ts";
import type { Transport } from "./internal/transport.ts";
import { EP } from "./raw/entrypoints.ts";
import { MLN_NETWORK_STATUS, MLN_RENDER_BACKEND_FLAG } from "./raw/enums.ts";
import { Runtime, type RuntimeOptions, WakeSource } from "./runtime.ts";

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

/** Which runtime payload packages the facade knows how to load. */
const KNOWN_RUNTIME_PACKAGES = ["@maplibre/native-ffi-runtime-node"] as const;

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

  private constructor(native: Native) {
    this.#native = native;
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
    return new Maplibre(new Native(transport));
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
        BigInt(status.rawValue),
      ]);
    });
  }

  /**
   * Creates a runtime owned by the calling host context.
   *
   * Each owner thread may hold one live runtime.
   */
  createRuntime(options?: RuntimeOptions): Runtime {
    return Runtime.create(this.#native, options);
  }

  /** Adopts a wake source another host context transferred. */
  adoptWakeSource(carrier: ArrayBuffer): WakeSource {
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

  /** @internal */
  get native(): Native {
    return this.#native;
  }
}
