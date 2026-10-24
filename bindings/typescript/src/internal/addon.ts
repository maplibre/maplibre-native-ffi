// The loaded Node-API addon, which an entry point binds before any call.

import { ABI_DIGEST, type NativeOperations } from "../generated_native.ts";
import { AbiVersionMismatchError } from "./errors.ts";

/**
 * One drained delivery: its type, its token, its payload, and the error that
 * converting the payload threw, if any. A completion's payload is its
 * {@link CompletionHeader}; a retirement has none; another delivery's is its
 * copied arguments, and its type is its `mln_delivery_type` value.
 */
export type DrainedEntry = [
  type: "completion" | "retired" | number,
  token: number,
  payload: unknown,
  error?: unknown,
];

/** The terminal status of a drained completion, without its value. */
export interface CompletionHeader {
  readonly status: number;
  readonly generation: bigint;
  readonly renderUpdateGeneration: bigint;
  readonly diagnostic: string;
}

/** The runtime exports of the addon, beside the generated operations. */
export interface Addon extends NativeOperations {
  install(
    receive: (kind: number, ...values: unknown[]) => void,
    register: (value: unknown) => number,
    makeError: (status: number, diagnostic: string) => Error,
    takeNative: (
      value: unknown,
      kind: string,
      path: string,
    ) => readonly bigint[] | undefined,
  ): void;
  abiDigest(): string;
  setReferenced(referenced: boolean): void;
  ownerTokenCreate(handle: bigint, name: string, session: boolean): object;
  shutdownAtExit(): void;
  /**
   * Drains the realm's queue into a batch that the realm holds until
   * {@link Addon.batchRelease}. Returns undefined when nothing is queued, and
   * null when the batch could not be allocated.
   */
  drain(): DrainedEntry[] | undefined | null;
  /** Converts a drained completion's value, taking its handle if `take`. */
  completionValue(index: number, decoder: number, take: boolean): unknown;
  /** Takes the owned handles of a drained delivery from the batch. */
  deliveryTake(index: number): void;
  batchRelease(): void;
}

/** How an entry point connects the runtime to its host. */
export interface HostHooks {
  /** Runs `listener` synchronously when the process begins to exit. */
  onExit?: (listener: () => void) => void;
  /**
   * Reports an owner that was collected while open. Without this hook, the
   * report goes to `console.warn`.
   */
  warnLeak?: (message: string) => void;
}

export let native: Addon = undefined as never;
export let hooks: HostHooks = {};

/**
 * Checks the addon's ABI digest before any other call and makes it the
 * addon that every operation calls.
 */
export function bindAddon(
  addon: Addon,
  hostHooks: HostHooks,
  install: (addon: Addon) => void,
): void {
  const digest = addon.abiDigest();
  if (digest !== ABI_DIGEST) {
    throw new AbiVersionMismatchError(
      `the native addon was generated from other C headers (ABI digest ${digest}, expected ${ABI_DIGEST})`,
    );
  }
  // The addon refuses a second copy of the binding in one realm, before this
  // copy calls anything else.
  install(addon);
  native = addon;
  hooks = hostHooks;
}
