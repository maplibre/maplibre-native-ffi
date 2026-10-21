// One suite, registered under each host's runner.
//
// Case files call test() at import. An entry point in test/run hands the
// registered cases to its runner, so every case runs in one process after
// whatever ran before it.

import { setImmediate } from "node:timers";

import { MaplibreError, Status } from "../../src/index.ts";
import { type Capability, capabilities } from "./host-capabilities.ts";

export interface Case {
  readonly name: string;
  readonly needs: readonly Capability[];
  readonly run: () => Promise<void> | void;
}

const cases: Case[] = [];

/** Registers a case that runs when the host provides every capability. */
export function test(
  name: string,
  run: () => Promise<void> | void,
  options: { needs?: readonly Capability[] } = {},
): void {
  cases.push({ name, run, needs: options.needs ?? [] });
}

/**
 * The cases this host runs, in the order they registered. A case is left out
 * only for a capability that this host declares unavailable.
 */
export function scheduled(): Case[] {
  const missing = (entry: Case) =>
    entry.needs.filter((need) => capabilities[need] === undefined);
  const unexplained = cases.filter((entry) =>
    missing(entry).some((need) => !capabilities.unavailable.includes(need)),
  );
  if (unexplained.length > 0) {
    throw new Error(
      `cases left out without a declared missing capability: ${unexplained.map((entry) => entry.name).join(", ")}`,
    );
  }
  return cases.filter((entry) => missing(entry).length === 0);
}

let teardowns: (() => unknown)[] | undefined;

/**
 * Registers cleanup for the running case. Cleanups run after the case in
 * reverse order, so a map closes before the runtime that owns it.
 */
export function teardown(cleanup: () => unknown): void {
  if (teardowns === undefined) {
    throw new Error("teardown() called outside a running case");
  }
  teardowns.push(cleanup);
}

/**
 * Closes an owner unless the case already released it. A release after an
 * accepted one reaches native code, which reports the handle stale.
 */
export async function closeIfLive(close: () => Promise<void>): Promise<void> {
  try {
    await close();
  } catch (error) {
    if (
      !(error instanceof MaplibreError && error.status === Status.invalidState)
    ) {
      throw error;
    }
  }
}

/** Runs one case and then its cleanups, and rethrows the first failure. */
export async function execute(entry: Case): Promise<void> {
  const own: (() => unknown)[] = [];
  teardowns = own;
  let failure: unknown;
  let failed = false;
  try {
    await entry.run();
  } catch (error) {
    failure = error;
    failed = true;
  }
  while (own.length > 0) {
    try {
      await own.pop()!();
    } catch (error) {
      if (!failed) {
        failure = error;
        failed = true;
      }
    }
  }
  teardowns = undefined;
  if (failed) {
    throw failure;
  }
}

/**
 * Rejects when `promise` takes longer than `milliseconds`. The timer is
 * unreferenced, so it bounds a case without keeping the process alive.
 */
export function bounded<T>(
  promise: Promise<T>,
  what: string,
  milliseconds = 30_000,
): Promise<T> {
  return bound(promise, what, milliseconds, false);
}

/**
 * Waits for a Promise that a plain callback settles, such as a provider
 * request handed out of its handler. The binding holds the event loop only
 * while one of its own Promises is pending, and a plain callback holds
 * nothing, so this wait holds the loop through a referenced bound until the
 * callback arrives. Deno's runner fails a case whose loop empties while it
 * waits.
 */
export function callbackArrival<T>(
  promise: Promise<T>,
  what: string,
): Promise<T> {
  return bound(promise, what, 30_000, true);
}

function bound<T>(
  promise: Promise<T>,
  what: string,
  milliseconds: number,
  hold: boolean,
): Promise<T> {
  return new Promise<T>((resolve, reject) => {
    const timer = setTimeout(
      () => reject(new Error(`${what} took longer than ${milliseconds} ms`)),
      milliseconds,
    );
    if (!hold) {
      capabilities.unrefTimer(timer);
    }
    promise.then(
      (value) => {
        clearTimeout(timer);
        resolve(value);
      },
      (error) => {
        clearTimeout(timer);
        reject(error);
      },
    );
  });
}

/**
 * Waits for the next wake, and withdraws the wait when it takes longer than
 * `milliseconds` or when `signal` aborts. A pending wait holds the event loop,
 * so a case that fails while it waits leaves nothing behind that does.
 */
export function nextWake(
  wake: {
    next(options: { signal?: AbortSignal }): Promise<void>;
  },
  what: string,
  options: { milliseconds?: number; signal?: AbortSignal } = {},
): Promise<void> {
  const milliseconds = options.milliseconds ?? 30_000;
  const withdraw = new AbortController();
  const timer = setTimeout(
    () =>
      withdraw.abort(new Error(`${what} took longer than ${milliseconds} ms`)),
    milliseconds,
  );
  capabilities.unrefTimer(timer);
  const abort = () => withdraw.abort(options.signal!.reason);
  options.signal?.addEventListener("abort", abort, { once: true });
  return wake.next({ signal: withdraw.signal }).finally(() => {
    clearTimeout(timer);
    options.signal?.removeEventListener("abort", abort);
  });
}

/** Resolves after the event loop has run once, delivering what is queued. */
export function turn(): Promise<void> {
  return new Promise((resolve) => setImmediate(resolve));
}

const collected = new Set<symbol>();
const registry = new FinalizationRegistry<symbol>((key) => collected.add(key));

/**
 * Returns whether `target` has been collected. A case observes collection
 * this way rather than through `WeakRef.deref()`, because each deref keeps
 * its target alive until the host next clears kept objects.
 */
export function watchCollection(target: object): () => boolean {
  const key = Symbol("collectable");
  registry.register(target, key);
  return () => collected.has(key);
}

/** Runs a few rounds of forced collection and event loop turns. */
export async function collectGarbage(): Promise<void> {
  for (let round = 0; round < 4; round++) {
    capabilities.forcedGc?.();
    await turn();
    await turn();
  }
}

/**
 * Alternates a forced collection with an event loop turn until `done` holds, for
 * at most 50 rounds. The caller allocates what it expects to be collected in a
 * helper frame that has returned. JavaScriptCore scans the stack
 * conservatively, so one collection can miss an object that no frame names.
 */
export async function collectGarbageUntil(
  done: () => boolean,
  what: string,
): Promise<void> {
  for (let round = 0; round < 50; round++) {
    capabilities.forcedGc!();
    await turn();
    await turn();
    if (done()) {
      return;
    }
  }
  throw new Error(`${what}: not collected after 50 forced collections`);
}
