/**
 * The Node-API transport.
 *
 * A runtime payload package owns one compiled addon and the metadata naming what
 * it was built for. This module turns that payload into the transport the shared
 * layer uses, after checking that the payload describes the same ABI this
 * package was generated from.
 */

import { verifyPayload } from "./handshake.ts";
import type { Ptr, Slab, Transport } from "./transport.ts";

/** The addon surface a Node-API payload exposes. */
export interface NodeApiAddon {
  abiFingerprint(): string;
  abiHeaderDigest(): string;
  entrypointCount(): number;
  entrypointName(entrypoint: number): string | null;
  registerSlab(buffer: ArrayBuffer): bigint;
  call(
    entrypoint: number,
    slots: bigint,
    diagnostic: bigint,
    diagnosticCapacity: number,
    diagnosticLength: bigint,
  ): number;
  symbol(entrypoint: number): bigint;
  readForeign(pointer: bigint, length: number): Uint8Array;
  readForeignCString(pointer: bigint): string | null;
  listenerAddress(kind: number): bigint;
  createCallbackOwner(): bigint;
  destroyCallbackOwner(owner: bigint): void;
  registerCallback(owner: bigint): bigint;
  destroyRecord(kind: number, record: bigint): void;
  drainRecords(owner: bigint, records: bigint, capacity: number): number;
  recordDepth(owner: bigint): number;
  startRecordNotifications(owner: bigint, callback: () => void): void;
  stopRecordNotifications(owner: bigint): void;
  transferIssue(handle: bigint): bigint;
  transferClaim(token: bigint): bigint;
  transferDiscard(token: bigint): bigint;
}

/**
 * Wraps a loaded addon.
 *
 * The handshake runs before anything else, because the call ABI indexes
 * entrypoints rather than naming them: a payload built from different headers
 * would dispatch an id to another function with another signature.
 */
export function nodeApiTransport(addon: NodeApiAddon): Transport {
  const fingerprint = addon.abiFingerprint();
  const headerDigest = addon.abiHeaderDigest();
  verifyPayload({ fingerprint, headerDigest });

  // Slabs are retained here for the transport's life. A backing store does not
  // move, so the address stays valid; letting one be collected would leave
  // native code holding a freed address.
  const slabs = new Map<bigint, Slab>();

  return {
    abi: "native64",
    pointerSize: 8,
    abiFingerprint: fingerprint,
    headerDigest,

    addSlab(byteLength: number): Slab {
      const buffer = new ArrayBuffer(byteLength);
      // A Node-API slab owns its buffer outright, so it starts at byte zero.
      const slab: Slab = {
        base: addon.registerSlab(buffer),
        byteOffset: 0,
        buffer,
      };
      slabs.set(slab.base, slab);
      return slab;
    },

    releaseSlab(base: Ptr): void {
      // Dropping the reference is the whole retirement: the backing store goes
      // when the collector reaches it, and no native code holds its address
      // once the allocator says the slab is empty.
      slabs.delete(base);
    },

    call(entrypoint, slots, diagnostic, diagnosticCapacity, diagnosticLength) {
      return addon.call(
        entrypoint,
        slots,
        diagnostic,
        diagnosticCapacity,
        diagnosticLength,
      );
    },

    symbol(entrypoint: number): Ptr {
      return addon.symbol(entrypoint);
    },

    readForeign(pointer: Ptr, length: number): Uint8Array {
      return addon.readForeign(pointer, length);
    },

    readForeignCString(pointer: Ptr): string | null {
      return addon.readForeignCString(pointer);
    },

    entrypointName(entrypoint: number): string | null {
      return addon.entrypointName(entrypoint);
    },

    listenerAddress: (kind: number): Ptr => addon.listenerAddress(kind),
    createCallbackOwner: () => addon.createCallbackOwner(),
    destroyCallbackOwner: (owner: bigint) => {
      addon.destroyCallbackOwner(owner);
    },
    registerCallback: (owner: bigint) => addon.registerCallback(owner),
    destroyRecord: (kind: number, record: Ptr) => {
      addon.destroyRecord(kind, record);
    },
    drainRecords: (owner: bigint, records: Ptr, capacity: number) =>
      addon.drainRecords(owner, records, capacity),
    recordDepth: (owner: bigint) => addon.recordDepth(owner),
    startRecordNotifications: (owner: bigint, drain: () => void) => {
      addon.startRecordNotifications(owner, drain);
    },
    stopRecordNotifications: (owner: bigint) => {
      addon.stopRecordNotifications(owner);
    },

    transferIssue: (handle) => addon.transferIssue(handle),
    transferClaim: (token) => addon.transferClaim(token),
    transferDiscard: (token) => addon.transferDiscard(token),
  };
}
