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
  /** The address of this slab's first byte. */
  readonly base: Ptr;
  /**
   * Where the slab starts inside `buffer`.
   *
   * A Node-API slab owns its buffer, so this is zero. A WebAssembly slab is a
   * region of the one linear memory, so it is the slab's offset in it.
   */
  readonly byteOffset: number;
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
   * Retires a slab that holds no live allocation.
   *
   * A transport keeps every slab reachable until this is called, because native
   * code holds addresses into them.
   */
  releaseSlab(base: Ptr): void;

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

  /** Reports the address of the listener a callback family registers. */
  listenerAddress(kind: number): Ptr;

  /**
   * Creates the owner a callback registry registers under.
   *
   * A process holds one copy of the shared library and may hold many host
   * execution contexts, each a JavaScript realm with a module graph of its own.
   * A realm cannot see what another realm chose, so an owner and the
   * registration identities under it come from the shared layer, where every
   * realm's are visible at once. Reports zero when none can be created.
   */
  createCallbackOwner(): bigint;

  /** Destroys an owner, releasing records still queued for it. */
  destroyCallbackOwner(owner: bigint): void;

  /**
   * Reserves a registration identity belonging to an owner.
   *
   * Reports zero when the owner is gone or has no identity left to give.
   */
  registerCallback(owner: bigint): bigint;

  /**
   * Releases a record a drain delivered.
   *
   * Each family owns its records differently, and the shared layer does not
   * have to know which: naming the kind is enough.
   */
  destroyRecord(kind: number, record: Ptr): void;

  /** Moves an owner's queued records into host storage, reporting how many. */
  drainRecords(owner: bigint, records: Ptr, capacity: number): number;

  /** Reports how many of an owner's callback records are waiting. */
  recordDepth(owner: bigint): number;

  /**
   * Installs the signal that wakes an owner's context when its record queues.
   *
   * The signal runs on the host's own execution context, not on the MapLibre
   * thread that produced the record. It is per owner, so a context that starts
   * later leaves the contexts already running still able to be woken.
   */
  startRecordNotifications(owner: bigint, drain: () => void): void;

  /** Removes the signal, leaving queued records for an explicit drain. */
  stopRecordNotifications(owner: bigint): void;

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
  badResultStorage: 4,
} as const;
