/**
 * The whole native boundary the shared layer sits on.
 *
 * A transport supplies memory the C API can address, one normalized call, and a
 * way to copy bytes out of library-owned memory. Everything else — struct
 * encoding, handle state, error mapping, the public API — is written once above
 * this interface and runs unchanged over a Node-API addon or a WebAssembly
 * instance.
 */

import type { AbiClass } from "../raw/layouts.ts";

declare const pointerBrand: unique symbol;

/**
 * A native address.
 *
 * Addresses are `bigint` rather than `number` because a `number` cannot carry
 * one: Android tags heap pointers in the top byte from Android 11 on, and a
 * pointer converted through a double loses the tag the allocator requires.
 */
export type Ptr = bigint & { readonly [pointerBrand]?: never };

/** A block of memory both JavaScript and the C API can reach. */
export interface Slab {
  /** The address of byte zero of `buffer`. */
  readonly base: Ptr;
  /**
   * The storage itself.
   *
   * A WebAssembly transport replaces this object when its memory grows, so
   * callers read `buffer` again rather than caching a view across a call that
   * can allocate.
   */
  readonly buffer: ArrayBuffer;
}

export interface Transport {
  /** Selects which generated layout table describes this transport's structs. */
  readonly abi: AbiClass;
  readonly pointerSize: 4 | 8;
  /** The ABI schema fingerprint the payload's dispatch was generated from. */
  readonly abiFingerprint: string;
  /** The digest of the public headers the payload was built against. */
  readonly headerDigest: string;

  /** Adds a slab. Existing slabs never move, so earlier addresses stay valid. */
  addSlab(byteLength: number): Slab;

  /**
   * Calls one entrypoint through the shared normalized dispatch.
   *
   * The diagnostic is copied inside a failing call, before any host code runs,
   * because the C API's diagnostic is thread-local and the next call replaces
   * it. Returns an `AbiCallStatus`.
   */
  call(
    entrypoint: number,
    slots: Ptr,
    diagnostic: Ptr,
    diagnosticCapacity: number,
    diagnosticLength: Ptr,
  ): number;

  /** Reports an entrypoint's address, for a struct field read as a callback. */
  symbol(entrypoint: number): Ptr;

  /** Copies bytes out of library-owned memory, which sits outside every slab. */
  readForeign(pointer: Ptr, length: number): Uint8Array;

  /** Copies a null-terminated library-owned string. */
  readForeignCString(pointer: Ptr): string | null;

  /** Names an entrypoint, so a failure can say which call it came from. */
  entrypointName(entrypoint: number): string | null;

  /** Issues a one-shot token naming a handle, for a move to another context. */
  transferIssue(handle: bigint): bigint;
  /** Claims a token, reporting the handle it named, or zero when it is spent. */
  transferClaim(token: bigint): bigint;
  /** Discards an unclaimed token, reporting the handle it named. */
  transferDiscard(token: bigint): bigint;
}

/** How a normalized call was dispatched. This describes the call, not its result. */
export const AbiCallStatus = {
  ok: 0,
  unknownEntrypoint: 1,
  nullSlots: 2,
  misalignedSlots: 3,
} as const;
