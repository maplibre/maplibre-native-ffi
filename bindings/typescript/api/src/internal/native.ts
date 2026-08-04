/**
 * One host execution context's view of the native library.
 *
 * A `Native` bundles the transport, the memory it addresses, and the call path.
 * Handles hold one, so every operation reaches the same library through the same
 * memory, and a handle carried into another context cannot silently use a
 * different one.
 */

import { MaplibreError, errorKindForStatus } from "../errors.ts";
import { ENTRYPOINTS } from "../raw/entrypoints.ts";
import { MLN_STATUS } from "../raw/enums.ts";
import { LAYOUTS, type RecordLayout } from "../raw/layouts.ts";
import { Caller, statusFromSlot } from "./call.ts";
import { Memory, type Scope } from "./memory.ts";
import { decodeUtf8 } from "./text.ts";
import type { Ptr, Transport } from "./transport.ts";

export class Native {
  readonly transport: Transport;
  readonly memory: Memory;
  readonly caller: Caller;
  readonly #layouts: Readonly<Record<string, RecordLayout>>;
  readonly #encoder = new TextEncoder();

  constructor(transport: Transport) {
    this.transport = transport;
    this.memory = new Memory(transport);
    this.caller = new Caller(transport, this.memory);
    this.#layouts = LAYOUTS[transport.abi];
  }

  /** The generated layout of one C struct for this transport's ABI class. */
  layout(name: string): RecordLayout {
    const layout = this.#layouts[name];
    if (layout === undefined) {
      throw new MaplibreError(
        "invalidInput",
        `${name} has no generated layout`,
      );
    }
    return layout;
  }

  /** Runs `body` with call-scoped native storage. */
  scope<T>(body: (scope: Scope) => T): T {
    return this.memory.scope(body);
  }

  /**
   * Calls a status-returning entrypoint, converting a non-OK status to an error
   * carrying the diagnostic the call itself produced.
   */
  checked(scope: Scope, entrypoint: number, args: readonly bigint[]): void {
    const result = this.caller.invoke(scope, entrypoint, args);
    const status = statusFromSlot(result.raw);
    if (status !== MLN_STATUS.MLN_STATUS_OK) {
      throw this.statusError(entrypoint, status, result.diagnostic);
    }
  }

  /** Calls an entrypoint whose result is not a status. */
  raw(
    scope: Scope,
    entrypoint: number,
    args: readonly bigint[],
    returnStorage?: Ptr,
  ): bigint {
    return this.caller.invoke(scope, entrypoint, args, returnStorage).raw;
  }

  /** Fills caller storage from an entrypoint that returns a struct by value. */
  structValue(scope: Scope, entrypoint: number, storage: Ptr): void {
    this.caller.invoke(scope, entrypoint, [], storage);
  }

  statusError(
    entrypoint: number,
    status: number,
    diagnostic: string,
  ): MaplibreError {
    const operation =
      ENTRYPOINTS[entrypoint]?.name ?? `entrypoint ${entrypoint}`;
    const kind = errorKindForStatus(status);
    const detail = diagnostic === "" ? "" : `: ${diagnostic}`;
    return new MaplibreError(kind, `${operation} reported ${kind}${detail}`, {
      nativeStatus: status,
      diagnostic,
      operation,
    });
  }

  /**
   * Writes a UTF-8 string into scoped storage as a null-terminated C string.
   *
   * An embedded NUL would end the string early at the C boundary, so it is
   * rejected here rather than silently truncating the caller's value.
   */
  cString(scope: Scope, value: string, what: string): Ptr {
    const bytes = this.#encoder.encode(value);
    if (bytes.includes(0)) {
      throw new MaplibreError(
        "invalidInput",
        `${what} contains an embedded NUL, which a null-terminated C string cannot carry`,
      );
    }
    const address = scope.allocate(bytes.length + 1);
    const target = this.memory.bytes(address, bytes.length + 1);
    target.set(bytes);
    target[bytes.length] = 0;
    return address;
  }

  /** Copies a library-owned string of known length. */
  foreignString(pointer: Ptr, length: number): string {
    if (pointer === 0n || length === 0) {
      return "";
    }
    return decodeUtf8(this.transport.readForeign(pointer, length));
  }

  /** Copies library-owned bytes. */
  foreignBytes(pointer: Ptr, length: number): Uint8Array {
    if (pointer === 0n || length === 0) {
      return new Uint8Array(0);
    }
    return this.transport.readForeign(pointer, length);
  }

  /** Reads a `size_t` field, whose width follows the ABI class. */
  readSize(address: Ptr): number {
    const view = this.memory.view(address, this.transport.pointerSize);
    return this.transport.pointerSize === 8
      ? Number(view.getBigUint64(0, true))
      : view.getUint32(0, true);
  }
}
