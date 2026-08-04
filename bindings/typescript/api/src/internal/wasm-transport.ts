/**
 * The WebAssembly transport.
 *
 * The same shared layer runs over this and over the Node-API addon. What changes
 * is only what a pointer is: here every address is an offset in the module's one
 * linear memory, so a slab is a region of that memory rather than a buffer of
 * its own, and library-owned memory is reachable through the same views instead
 * of through a copy call.
 *
 * The module is a portable JavaScript-host build. Nothing here touches `window`,
 * `document`, or a canvas: a browser, Node, Bun, and Deno all instantiate the
 * same artifact.
 */

import { ABI_FINGERPRINT, ABI_HEADER_DIGEST } from "../raw/fingerprint.ts";
import { AbiMismatchError } from "./node-transport.ts";
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
  _mln_abi_queue_drain(records: number, capacity: number): number;
  _mln_abi_queue_depth(): number;
  _mln_abi_transfer_issue(handle: bigint): bigint;
  _mln_abi_transfer_claim(token: bigint): bigint;
  _mln_abi_transfer_discard(token: bigint): bigint;
  UTF8ToString(pointer: number): string;
}

const decoder = new TextDecoder();

/**
 * Wraps an instantiated module.
 *
 * The handshake runs first for the same reason it does on the Node-API side: the
 * call ABI indexes entrypoints rather than naming them, so a module built from
 * other headers would dispatch an id to another function.
 */
export function wasmTransport(module: WasmModule): Transport {
  const fingerprint = module.UTF8ToString(module._mln_abi_fingerprint());
  if (fingerprint !== ABI_FINGERPRINT) {
    throw new AbiMismatchError(
      `the WebAssembly runtime reports ABI fingerprint ${fingerprint}, and this ` +
        `package was generated from ${ABI_FINGERPRINT}`,
    );
  }
  const headerDigest = module.UTF8ToString(module._mln_abi_header_digest());
  if (headerDigest !== ABI_HEADER_DIGEST) {
    throw new AbiMismatchError(
      `the WebAssembly runtime was built against public headers digesting to ` +
        `${headerDigest}, and this package was generated from ${ABI_HEADER_DIGEST}`,
    );
  }

  /**
   * The current linear memory.
   *
   * Growing the memory replaces the buffer, so it is read per use rather than
   * captured. Offsets survive growth; the views over them do not.
   */
  const memory = (): ArrayBuffer => module.HEAPU8.buffer as ArrayBuffer;

  let drain: (() => void) | undefined;

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
      // view rather than a call across a boundary.
      const offset = Number(pointer);
      return new Uint8Array(memory().slice(offset, offset + length));
    },

    readForeignCString(pointer: Ptr): string | null {
      if (pointer === 0n) {
        return null;
      }
      const bytes = new Uint8Array(memory());
      let end = Number(pointer);
      while (bytes[end] !== 0 && end < bytes.length) {
        end += 1;
      }
      return decoder.decode(bytes.subarray(Number(pointer), end));
    },

    entrypointName(entrypoint: number): string | null {
      const pointer = module._mln_abi_entrypoint_name(entrypoint);
      return pointer === 0 ? null : module.UTF8ToString(pointer);
    },

    listenerAddress(kind: number): Ptr {
      const address =
        kind === 1
          ? module._mln_abi_log_listener_address()
          : kind === 2
            ? module._mln_abi_resource_request_listener_address()
            : 0;
      return BigInt(address) as Ptr;
    },

    drainRecords(records: Ptr, capacity: number): number {
      return module._mln_abi_queue_drain(Number(records), capacity);
    },

    recordDepth(): number {
      return module._mln_abi_queue_depth();
    },

    startRecordNotifications(sink: () => void): void {
      // MapLibre's threads are Web Workers here, and a worker cannot call into
      // this context directly. The queue is drained when the host pumps, which
      // is the same moment a browser would have been woken.
      drain = sink;
    },

    stopRecordNotifications(): void {
      drain = undefined;
    },

    transferIssue: (handle) => module._mln_abi_transfer_issue(handle),
    transferClaim: (token) => module._mln_abi_transfer_claim(token),
    transferDiscard: (token) => module._mln_abi_transfer_discard(token),

    /** @internal Lets the pump deliver what the workers queued. */
    get pendingDrain(): (() => void) | undefined {
      return drain;
    },
  } as Transport;
}
