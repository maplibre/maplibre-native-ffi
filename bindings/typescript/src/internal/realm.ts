// What the addon's dispatch reaches in this realm, and what keeps the realm's
// event loop alive while it waits.
//
// Every completion, registration, and wake of this realm names the realm's
// delivery queue. The addon dispatches the queue on the JavaScript thread when
// its wake posts to this realm, after the native callback that queued each
// call has returned. A completion's call settles its Promise directly. A
// registration's call reaches its host value through a root that the
// registration's owner keeps, so a callback that captures its owner leaves the
// owner collectable, and a generated adapter calls the host value with the
// call's arguments.

import { type Addon, hooks, native } from "./addon.ts";
import { MaplibreError } from "./errors.ts";
import { takeNativeRegistration } from "./native_registrations.ts";

/**
 * The root of one registration, which the addon references until native code
 * releases the registration. It reaches the registration's host value through
 * its holder, which it does not keep reachable.
 */
export interface Root {
  holder?: WeakRef<RootHolder>;
}

/** The callback roots of one owner, or of the realm for global calls. */
export interface RootHolder {
  readonly roots: Map<Root, unknown>;
}

const realmRoots: RootHolder = { roots: new Map() };
let transaction: Registrations | undefined;
// The roots that a holder took and native code has not released.
let liveCount = 0;

/** The host values that one call registers with native code. */
export class Registrations {
  readonly #entries: [Root, unknown][] = [];

  add(value: unknown): Root {
    const root: Root = {};
    this.#entries.push([root, value]);
    return root;
  }

  /** Roots the accepted values in `holder`, or in the realm. */
  commit(holder: RootHolder = realmRoots): void {
    for (const [root, value] of this.#entries) {
      holder.roots.set(root, value);
      root.holder = new WeakRef(holder);
      liveCount++;
    }
    this.#entries.length = 0;
  }
}

/**
 * Runs a call whose arguments register callbacks. The addon registers each
 * one through this call's transaction, and the call commits the roots once
 * native code accepts them. A call that throws registered nothing.
 */
export function withRegistrations<T>(
  body: (registrations: Registrations) => T,
): T {
  const previous = transaction;
  const current = new Registrations();
  transaction = current;
  try {
    return body(current);
  } finally {
    transaction = previous;
  }
}

function register(value: unknown): Root {
  if (transaction === undefined) {
    throw new Error("a callback was converted outside a registering call");
  }
  return transaction.add(value);
}

/** Runs after each root release, for the suite's waits on one. */
export const rootReleaseListeners = new Set<() => void>();

function retire(root: Root): void {
  const holder = root.holder;
  if (holder === undefined) {
    return;
  }
  root.holder = undefined;
  holder.deref()?.roots.delete(root);
  liveCount--;
  for (const listener of rootReleaseListeners) {
    listener();
  }
}

/**
 * How a dispatched call reaches the host value of its registration, by the
 * callback type that the addon's trampoline numbers. Generated code defines
 * the adapters.
 */
export type CallbackAdapter = (target: unknown, args: unknown[]) => void;

let adapters: readonly CallbackAdapter[] = [];

export function defineCallbacks(table: readonly CallbackAdapter[]): void {
  adapters = table;
}

// Pending binding Promises hold the event loop through the addon's
// threadsafe function. Nothing else does.
let pending = 0;
// The completions and thread starts that wait for native code.
let outstanding = 0;

/** Holds the event loop open until the matching unhold(). */
export function hold(): void {
  // Counts only a hold that took, so a call that throws here, such as a
  // browser page's call before loadModule(), leaves nothing held.
  if (pending === 0) {
    native.setReferenced(true);
  }
  pending++;
}

export function unhold(): void {
  if (--pending === 0) {
    native.setReferenced(false);
  }
}

/**
 * Holds the event loop for one completion or thread start, and returns what
 * ends the hold, which only its first call does.
 */
export function awaitNative(): () => void {
  hold();
  outstanding++;
  let held = true;
  return () => {
    if (held) {
      held = false;
      outstanding--;
      unhold();
    }
  };
}

/**
 * The completions and thread starts that wait for native code, for the
 * suite's leak checks.
 */
export function pendingCompletions(): number {
  return outstanding;
}

/**
 * The callback roots that native code may still call through, for the
 * suite's leak checks. Each root leaves when native code releases it.
 */
export function liveRoots(): number {
  return liveCount;
}

/**
 * Calls the host value of a registration with the arguments that the addon's
 * trampoline converted, or reports the error that converting them threw.
 * Returns whether a host value received the arguments: none does once the
 * collector reclaims the registration's owner.
 */
function invoke(
  callback: number,
  root: Root,
  error: unknown,
  ...args: unknown[]
): boolean {
  const target = root.holder?.deref()?.roots.get(root);
  if (target === undefined) {
    return false;
  }
  if (error !== undefined) {
    reportError(error);
    return false;
  }
  try {
    adapters[callback]!(target, args);
  } catch (thrown) {
    reportError(thrown);
  }
  return true;
}

/**
 * Reports an error that a host callback threw through the global
 * `reportError`, or the entry point's hook where the global scope has none.
 */
function reportError(error: unknown): void {
  const report =
    (globalThis as { reportError?: (error: unknown) => void }).reportError ??
    hooks.reportError;
  if (report) {
    report(error);
  } else {
    console.error(error);
  }
}

function reportLeak(name: string, handle: bigint): void {
  const message = `${name} 0x${handle.toString(
    16,
  )} was collected while open, so its handle was disposed; close it explicitly`;
  try {
    if (hooks.warnLeak) {
      hooks.warnLeak(message);
    } else {
      console.warn(`MapLibre Native: ${message}`);
    }
  } catch (error) {
    reportError(error);
  }
}

/**
 * Gives the addon this realm's registration, callback, release, error
 * factory, native registration taker, and leak report.
 */
export function installRealm(addon: Addon): void {
  addon.install(
    register,
    invoke,
    retire,
    (status, diagnostic) => new MaplibreError(status, diagnostic),
    takeNativeRegistration,
    reportLeak,
  );
}
