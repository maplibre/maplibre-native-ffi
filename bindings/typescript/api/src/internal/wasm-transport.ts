/**
 * The WebAssembly transport.
 *
 * The same shared layer runs over this and over the Node-API addon. What changes
 * is only what a pointer is: here every address is an offset in the module's one
 * linear memory, so a slab is a region of that memory rather than a buffer of
 * its own, and library-owned memory is reachable through the same views instead
 * of through a copy call.
 *
 * Nothing here touches `window`, `document`, or a canvas, so a browser, Node,
 * Bun, and Deno all instantiate the same artifact. What is not portable is the
 * module's own default resource loading: MapLibre's Emscripten HTTP source is
 * Emscripten Fetch, which is XHR, so a non-browser host serves resources
 * through a resource provider until a host-injected network adapter exists.
 */

import { MaplibreError } from "../errors.ts";
import { ABI_FINGERPRINT, ABI_HEADER_DIGEST } from "../raw/fingerprint.ts";
import { AbiMismatchError, verifyPayload } from "./handshake.ts";
import { decodeUtf8 } from "./text.ts";
import type { Ptr, Slab, Transport } from "./transport.ts";

/**
 * What the Emscripten module exposes.
 *
 * These are the exported functions and runtime helpers the link step names, and
 * nothing else: the transport reaches the C API through the same normalized
 * dispatch the Node-API addon does.
 */
export interface WasmModule {
  HEAPU8: Uint8Array;
  /**
   * The module's memory.
   *
   * Emscripten refreshes its `HEAP*` views in whichever agent grew the memory,
   * so a worker that grows it leaves this agent's views short. The memory
   * object itself always names the live buffer.
   */
  wasmMemory?: WebAssembly.Memory;
  _malloc(size: number): number;
  _free(pointer: number): void;
  _mln_abi_fingerprint(): number;
  _mln_abi_header_digest(): number;
  _mln_abi_entrypoint_count(): number;
  _mln_abi_entrypoint_name(entrypoint: number): number;
  _mln_abi_call(
    entrypoint: number,
    slots: number,
    diagnostic: number,
    diagnosticCapacity: number,
    diagnosticLength: number,
  ): number;
  _mln_abi_symbol(entrypoint: number): number;
  _mln_abi_log_listener_address(): number;
  _mln_abi_resource_request_listener_address(): number;
  _mln_abi_custom_geometry_fetch_listener_address(): number;
  _mln_abi_custom_geometry_cancel_listener_address(): number;
  _mln_abi_record_destroy(kind: number, record: number): void;
  _mln_abi_queue_drain(records: number, capacity: number): number;
  _mln_abi_queue_depth(): number;
  _mln_abi_transfer_issue(handle: bigint): bigint;
  _mln_abi_transfer_claim(token: bigint): bigint;
  _mln_abi_transfer_discard(token: bigint): bigint;
  UTF8ToString(pointer: number): string;
}

/**
 * Wraps an instantiated module.
 *
 * The handshake runs first for the same reason it does on the Node-API side: the
 * call ABI indexes entrypoints rather than naming them, so a module built from
 * other headers would dispatch an id to another function.
 */
export function wasmTransport(module: WasmModule): Transport {
  const fingerprint = module.UTF8ToString(module._mln_abi_fingerprint());
  const headerDigest = module.UTF8ToString(module._mln_abi_header_digest());
  verifyPayload({ fingerprint, headerDigest });

  /**
   * The current linear memory.
   *
   * Growing the memory replaces the buffer, so it is read per use rather than
   * captured. It comes from the memory object rather than from `HEAPU8`,
   * because Emscripten refreshes those views only in the agent that grew the
   * memory: a MapLibre worker that grows it would otherwise leave this agent
   * reading a buffer that ends before the record it was handed.
   */
  const memory = (): ArrayBuffer =>
    (module.wasmMemory?.buffer ?? module.HEAPU8.buffer) as ArrayBuffer;

  /** Rejects a pointer no wasm32 address can be, before it reaches a view. */
  const offsetOf = (pointer: Ptr, what: string): number => {
    if (pointer < 0n || pointer > 0xffff_ffffn) {
      throw new MaplibreError(
        "invalidArgument",
        `${what} is not a wasm32 address: ${pointer}`,
      );
    }
    return Number(pointer);
  };

  return {
    abi: "wasm32",
    pointerSize: 4,
    abiFingerprint: fingerprint,
    headerDigest,

    addSlab(byteLength: number): Slab {
      const offset = module._malloc(byteLength);
      if (offset === 0) {
        throw new AbiMismatchError(
          `the WebAssembly runtime could not allocate ${byteLength} bytes`,
        );
      }
      // One linear memory, so the slab is a region of it: its address and its
      // offset in the buffer are the same number.
      return {
        base: BigInt(offset) as Ptr,
        byteOffset: offset,
        get buffer() {
          return memory();
        },
      };
    },

    releaseSlab(base: Ptr): void {
      module._free(Number(base));
    },

    call(entrypoint, slots, diagnostic, diagnosticCapacity, diagnosticLength) {
      return module._mln_abi_call(
        entrypoint,
        Number(slots),
        Number(diagnostic),
        diagnosticCapacity,
        Number(diagnosticLength),
      );
    },

    symbol(entrypoint: number): Ptr {
      return BigInt(module._mln_abi_symbol(entrypoint)) as Ptr;
    },

    readForeign(pointer: Ptr, length: number): Uint8Array {
      // Library allocations live in the same memory, so this is a copy out of a
      // view rather than a call across a boundary. `slice` clamps rather than
      // rejecting, so the range is checked first: a short copy would surface
      // later as a decode failure that names nothing.
      const offset = offsetOf(pointer, "a foreign pointer");
      if (!Number.isInteger(length) || length < 0) {
        throw new MaplibreError(
          "invalidInput",
          `a foreign read length must be a count, not ${length}`,
        );
      }
      const buffer = memory();
      if (offset === 0 || offset + length > buffer.byteLength) {
        throw new MaplibreError(
          "invalidArgument",
          `a ${length}-byte read at ${pointer} leaves this module's memory`,
        );
      }
      return new Uint8Array(buffer.slice(offset, offset + length));
    },

    readForeignCString(pointer: Ptr): string | null {
      if (pointer === 0n) {
        return null;
      }
      const start = offsetOf(pointer, "a foreign string pointer");
      const bytes = new Uint8Array(memory());
      if (start >= bytes.length) {
        throw new MaplibreError(
          "invalidArgument",
          `a string at ${pointer} starts outside this module's memory`,
        );
      }
      let end = start;
      while (end < bytes.length && bytes[end] !== 0) {
        end += 1;
      }
      if (end === bytes.length) {
        // No terminator before the end of memory, so this address named
        // something else.
        throw new MaplibreError(
          "invalidArgument",
          `a string at ${pointer} has no terminator inside this module's memory`,
        );
      }
      return decodeUtf8(bytes.subarray(start, end));
    },

    entrypointName(entrypoint: number): string | null {
      const pointer = module._mln_abi_entrypoint_name(entrypoint);
      return pointer === 0 ? null : module.UTF8ToString(pointer);
    },

    listenerAddress(kind: number): Ptr {
      const addresses: Readonly<Record<number, () => number>> = {
        1: () => module._mln_abi_log_listener_address(),
        2: () => module._mln_abi_resource_request_listener_address(),
        3: () => module._mln_abi_custom_geometry_fetch_listener_address(),
        4: () => module._mln_abi_custom_geometry_cancel_listener_address(),
      };
      return BigInt(addresses[kind]?.() ?? 0) as Ptr;
    },

    destroyRecord(kind: number, record: Ptr): void {
      module._mln_abi_record_destroy(kind, Number(record));
    },

    drainRecords(records: Ptr, capacity: number): number {
      return module._mln_abi_queue_drain(Number(records), capacity);
    },

    recordDepth(): number {
      return module._mln_abi_queue_depth();
    },

    startRecordNotifications(): void {
      // MapLibre's threads are Web Workers here, and a worker cannot schedule
      // work on this agent through the C queue's notifier. Delivery is driven
      // by the host instead: a pump delivers what arrived, which is the same
      // moment a notifier would have woken it.
    },

    stopRecordNotifications(): void {
      // Nothing was installed, so nothing is removed.
    },

    // A wasm i64 reaches JavaScript as a signed BigInt, and every one of these
    // is an unsigned handle or token, so a value with the top bit set would
    // come back negative and match nothing.
    transferIssue: (handle) =>
      BigInt.asUintN(64, module._mln_abi_transfer_issue(handle)),
    transferClaim: (token) =>
      BigInt.asUintN(64, module._mln_abi_transfer_claim(token)),
    transferDiscard: (token) =>
      BigInt.asUintN(64, module._mln_abi_transfer_discard(token)),
  };
}
