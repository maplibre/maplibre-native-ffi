// The loaded Node-API addon, which an entry point binds before any call.

import type { NativeOperations } from "../generated_native.ts";
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
  setReferenced(referenced: boolean): void;
  ownerTokenCreate(handle: bigint, name: string, session: boolean): object;
  shutdownAtExit(): void;
  /**
   * Drains the realm's queue into a batch that the realm holds until
   * {@link Addon.batchRelease}. Returns undefined when nothing is queued, and
   * null when the batch could not be allocated.
   */
  drain(): DrainedEntry[] | undefined | null;
  /**
   * Converts a drained completion's value through the decoder that the
   * operation's export recorded.
   */
  completionValue(index: number): unknown;
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
  /**
   * Reports an error that a host callback threw, where the global scope has
   * no `reportError`. Without this hook, the report goes to `console.error`.
   */
  reportError?: (error: unknown) => void;
}

export let native: Addon = undefined as never;
export let hooks: HostHooks = {};

/** The C ABI version that this binding was generated for. */
export const EXPECTED_C_ABI_VERSION = 0;

/**
 * Checks the C ABI version of the addon's embedded library before any other
 * call and makes it the addon that every operation calls.
 */
export function bindAddon(
  addon: Addon,
  hostHooks: HostHooks,
  install: (addon: Addon) => void,
): void {
  const version = addon.mln_c_version();
  if (version !== EXPECTED_C_ABI_VERSION) {
    throw new AbiVersionMismatchError(
      `unsupported MapLibre Native C ABI version ${version}; expected ${EXPECTED_C_ABI_VERSION}`,
    );
  }
  // The addon refuses a second copy of the binding in one realm, before this
  // copy calls anything else.
  install(addon);
  native = addon;
  hooks = hostHooks;
}
