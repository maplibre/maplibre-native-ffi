/**
 * The normalized call, from the TypeScript side.
 *
 * Arguments are written into an eight-byte slot array the shim reads, and slot 0
 * carries the result — or the address of caller storage when the entrypoint
 * returns a struct by value. The diagnostic buffer travels with every call
 * because the C API's diagnostic is thread-local: reading it afterwards would
 * read whatever the next call left behind.
 */

import { ENTRYPOINTS } from "../raw/entrypoints.ts";
import { LAYOUTS, type RecordLayout } from "../raw/layouts.ts";
import type { Memory, Scope } from "./memory.ts";
import { AbiCallStatus, type Ptr, type Transport } from "./transport.ts";

const SLOT_BYTES = 8;
const DIAGNOSTIC_BYTES = 1024;

/** Thrown when a call could not be dispatched at all. */
export class AbiCallError extends Error {
  override name = "AbiCallError";

  constructor(
    readonly entrypoint: number,
    readonly entrypointName: string,
    readonly abiStatus: number,
  ) {
    super(`dispatching ${entrypointName} reported ABI status ${abiStatus}`);
  }
}

export interface CallResult {
  /** Slot 0 as the shim left it. */
  readonly raw: bigint;
  /**
   * The diagnostic the C API set for a failing call, copied inside the call.
   * Empty when the call reported success or set no diagnostic.
   */
  readonly diagnostic: string;
}

/** Encodes a signed 32-bit argument into its slot. */
export function i32Slot(value: number): bigint {
  return BigInt.asUintN(64, BigInt(value | 0));
}

/** Encodes an unsigned 32-bit argument into its slot. */
export function u32Slot(value: number): bigint {
  return BigInt(value >>> 0);
}

/** Encodes a double argument into its slot, which carries the bit pattern. */
export function f64Slot(value: number): bigint {
  const scratch = new DataView(new ArrayBuffer(8));
  scratch.setFloat64(0, value, true);
  return scratch.getBigUint64(0, true);
}

/** Decodes a double from a slot. */
export function f64FromSlot(raw: bigint): number {
  const scratch = new DataView(new ArrayBuffer(8));
  scratch.setBigUint64(0, raw, true);
  return scratch.getFloat64(0, true);
}

/** Decodes an `mln_status` from a slot. */
export function statusFromSlot(raw: bigint): number {
  return Number(BigInt.asIntN(32, raw));
}

export class Caller {
  readonly #transport: Transport;
  readonly #memory: Memory;
  readonly #layouts: Readonly<Record<string, RecordLayout>>;
  readonly #diagnostic: Ptr;
  readonly #diagnosticLength: Ptr;
  readonly #decoder = new TextDecoder();

  constructor(transport: Transport, memory: Memory) {
    this.#transport = transport;
    this.#memory = memory;
    this.#layouts = LAYOUTS[transport.abi];
    // One buffer per transport, reused by every call. A transport belongs to one
    // host execution context, and the C API's diagnostic is per thread, so no
    // two calls share it concurrently.
    this.#diagnostic = memory.allocate(DIAGNOSTIC_BYTES);
    this.#diagnosticLength = memory.allocate(SLOT_BYTES);
  }

  /**
   * Calls an entrypoint with slot values the caller already encoded.
   *
   * `returnStorage` is required for an entrypoint that returns a struct by
   * value, and is where the shim writes it.
   */
  invoke(
    scope: Scope,
    entrypoint: number,
    args: readonly bigint[],
    returnStorage?: Ptr,
  ): CallResult {
    const info = ENTRYPOINTS[entrypoint];
    if (info === undefined) {
      throw new AbiCallError(
        entrypoint,
        `entrypoint ${entrypoint}`,
        AbiCallStatus.unknownEntrypoint,
      );
    }
    // A struct return is stored through slot 0. Storage that is absent or too
    // small would be a write into whatever else lives at that address, so the
    // requirement is checked here rather than discovered afterwards.
    if (info.resultRecord !== undefined) {
      const required = this.#layouts[info.resultRecord]?.size ?? 0;
      const available =
        returnStorage === undefined
          ? 0
          : (this.#memory.allocationSize(returnStorage) ?? 0);
      if (available < required) {
        throw new AbiCallError(
          entrypoint,
          info.name,
          AbiCallStatus.badResultStorage,
        );
      }
    }
    if (args.length !== info.params.length) {
      throw new AbiCallError(
        entrypoint,
        info.name,
        AbiCallStatus.unknownEntrypoint,
      );
    }

    const slots = scope.allocateZeroed(SLOT_BYTES * (args.length + 1));
    const view = this.#memory.view(slots, SLOT_BYTES * (args.length + 1));
    if (returnStorage !== undefined) {
      view.setBigUint64(0, returnStorage, true);
    }
    for (let index = 0; index < args.length; index += 1) {
      view.setBigUint64(SLOT_BYTES * (index + 1), args[index]!, true);
    }

    this.#memory.view(this.#diagnosticLength, 4).setUint32(0, 0, true);
    const abiStatus = this.#transport.call(
      entrypoint,
      slots,
      this.#diagnostic,
      DIAGNOSTIC_BYTES,
      this.#diagnosticLength,
    );
    if (abiStatus !== AbiCallStatus.ok) {
      throw new AbiCallError(entrypoint, info.name, abiStatus);
    }

    const length = this.#memory
      .view(this.#diagnosticLength, 4)
      .getUint32(0, true);
    return {
      raw: this.#memory
        .view(slots, SLOT_BYTES * (args.length + 1))
        .getBigUint64(0, true),
      diagnostic:
        length === 0
          ? ""
          : this.#decoder.decode(this.#memory.bytes(this.#diagnostic, length)),
    };
  }
}
