// What the addon delivers to this realm, and what keeps the realm's event loop
// alive while it waits.
//
// Every completion and queued callback of this realm names the realm's
// delivery queue and a token. The addon's wake shim posts to this realm when
// the queue becomes nonempty, and receive() drains it on the JavaScript
// thread, after the native callback that queued each delivery has returned,
// then dispatches each delivery by its token. A wake registration posts its
// own token the same way. Callback registrations reach their host values
// through a table of weak references, so a callback that captures its owner
// leaves the owner collectable.

import {
  type Addon,
  type CompletionHeader,
  type DrainedEntry,
  hooks,
  native,
} from "./addon.ts";
import { MaplibreError } from "./errors.ts";
import { takeNativeRegistration } from "./native_registrations.ts";

// The kinds of message that the addon's wake shim posts.
const WAKE = 1;
const RELEASE = 2;
const LEAK = 3;

// The token that the queue's own wake posts.
const QUEUE = 0;
const MAX_TOKEN = 0x7fffffff;

/** The callback roots of one owner, or of the realm for global calls. */
export interface RootHolder {
  readonly roots: Map<number, unknown>;
}

const registrations = new Map<number, WeakRef<RootHolder>>();
const realmRoots: RootHolder = { roots: new Map() };
let transaction: Registrations | undefined;

export interface PendingCompletion {
  /** Settles from a drained completion, which index names in its batch. */
  settle(index: number, header: CompletionHeader): void;
  /** Rejects with the error that converting the completion threw. */
  fail(error: unknown): void;
}

const completions = new Map<number, PendingCompletion>();
let nextToken = 1;

/** A token that no pending completion or rooted registration holds. */
function allocateToken(): number {
  while (registrations.has(nextToken) || completions.has(nextToken)) {
    nextToken = nextToken === MAX_TOKEN ? 1 : nextToken + 1;
  }
  const token = nextToken;
  nextToken = nextToken === MAX_TOKEN ? 1 : nextToken + 1;
  return token;
}

/** The host values that one call registers with native code. */
export class Registrations {
  readonly #entries: [number, unknown][] = [];

  add(value: unknown): number {
    const token = allocateToken();
    this.#entries.push([token, value]);
    return token;
  }

  /** Roots the accepted values in `holder`, or in the realm. */
  commit(holder: RootHolder = realmRoots): void {
    for (const [token, value] of this.#entries) {
      holder.roots.set(token, value);
      registrations.set(token, new WeakRef(holder));
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

function register(value: unknown): number {
  if (transaction === undefined) {
    throw new Error("a callback was converted outside a registering call");
  }
  return transaction.add(value);
}

function rooted(token: number): unknown {
  return registrations.get(token)?.deref()?.roots.get(token);
}

/** Runs after each root release, for the suite's waits on one. */
export const rootReleaseListeners = new Set<() => void>();

function retire(token: number): void {
  registrations.get(token)?.deref()?.roots.delete(token);
  if (registrations.delete(token)) {
    for (const listener of rootReleaseListeners) {
      listener();
    }
  }
}

/**
 * How a drained delivery reaches the host value of its registration, by
 * delivery type. Generated code defines the table.
 */
export type DeliveryHandler = (
  target: unknown,
  payload: Readonly<Record<string, unknown>>,
  index: number,
) => void;

let deliveries: Readonly<Record<number, DeliveryHandler>> = {};

export function defineDeliveries(
  table: Readonly<Record<number, DeliveryHandler>>,
): void {
  deliveries = table;
}

// Pending binding Promises hold the event loop through the addon's
// threadsafe function. Nothing else does.
let pending = 0;

/** Holds the event loop open until the matching unhold(). */
export function hold(): void {
  if (pending++ === 0) {
    native.setReferenced(true);
  }
}

export function unhold(): void {
  if (--pending === 0) {
    native.setReferenced(false);
  }
}

/** Registers a pending completion and returns its token. */
export function addCompletion(completion: PendingCompletion): number {
  const token = allocateToken();
  completions.set(token, completion);
  hold();
  return token;
}

/** Forgets a completion whose submission native code rejected. */
export function removeCompletion(token: number): void {
  if (completions.delete(token)) {
    unhold();
  }
}

/** The completions that wait for delivery, for the suite's leak checks. */
export function pendingCompletions(): number {
  return completions.size;
}

/**
 * The callback roots that native code may still deliver to, for the suite's
 * leak checks. Each root leaves when native code retires it.
 */
export function liveRoots(): number {
  return registrations.size;
}

function dispatch(entry: DrainedEntry, index: number): void {
  const [type, token, payload, error] = entry as [
    DrainedEntry[0],
    number,
    Readonly<Record<string, unknown>>,
    unknown,
  ];
  if (type === "completion") {
    const completion = completions.get(token);
    if (completion === undefined) {
      return;
    }
    completions.delete(token);
    unhold();
    if (error !== undefined) {
      completion.fail(error);
    } else {
      completion.settle(index, payload as unknown as CompletionHeader);
    }
    return;
  }
  if (type === "retired") {
    retire(token);
    return;
  }
  // A delivery for a registration this realm no longer holds is released
  // unread with its batch.
  const target = rooted(token);
  const handler = deliveries[type];
  if (target === undefined || handler === undefined) {
    return;
  }
  if (error !== undefined) {
    reportError(error);
    return;
  }
  try {
    handler(target, payload, index);
  } catch (thrown) {
    reportError(thrown);
  }
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

/**
 * Drains the realm's queue until it is empty and dispatches each delivery in
 * push order. A batch that could not be allocated leaves its deliveries
 * queued, and the realm drains again shortly, since a pending completion may
 * get no later push to wake the queue.
 */
function drain(): void {
  for (;;) {
    let entries;
    try {
      entries = native.drain();
    } catch (error) {
      reportError(error);
      return;
    }
    if (entries === undefined) {
      return;
    }
    if (entries === null) {
      setTimeout(drain, 50);
      return;
    }
    try {
      entries.forEach(dispatch);
    } finally {
      native.batchRelease();
    }
  }
}

function receive(kind: number, ...values: unknown[]): void {
  try {
    switch (kind) {
      case WAKE: {
        const token = values[0] as number;
        if (token === QUEUE) {
          drain();
        } else {
          (rooted(token) as (() => void) | undefined)?.();
        }
        return;
      }
      case RELEASE:
        retire(values[0] as number);
        return;
      case LEAK: {
        const handle = values[1] as bigint;
        const message = `${String(values[0])} 0x${handle.toString(
          16,
        )} was collected while open, so its handle was disposed; close it explicitly`;
        if (hooks.warnLeak) {
          hooks.warnLeak(message);
        } else {
          console.warn(`MapLibre Native: ${message}`);
        }
        return;
      }
    }
  } catch (error) {
    reportError(error);
  }
}

/**
 * Gives the addon this realm's receiver, registration, error factory, and
 * native registration taker.
 */
export function installRealm(addon: Addon): void {
  addon.install(
    receive,
    register,
    (status, diagnostic) => new MaplibreError(status, diagnostic),
    takeNativeRegistration,
  );
}
