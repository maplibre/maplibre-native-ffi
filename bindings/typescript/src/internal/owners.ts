// Handle state for the generated owner classes.
//
// Handles are generational IDs that native code validates, so an owner keeps
// only its ID, a released flag, and its parent. A call on a released handle
// reaches native code, which reports it stale. Each owner holds an owner token
// whose Node-API finalizer disposes the handle if the owner is collected while
// unreleased. The release call ends the token in the same native call that
// consumes the handle, so no realm teardown between the two finalizes a
// consumed handle.

import { Status } from "../generated_values.ts";
import { hooks, native } from "./addon.ts";
import { MaplibreError } from "./errors.ts";
import type { Registrations, RootHolder } from "./realm.ts";

/** The key that only generated code passes to an owner constructor. */
export const ADOPT: unique symbol = Symbol("adopt");
export type AdoptKey = typeof ADOPT;

/** Throws unless `key` is the API's own. */
export function checkKey(key: AdoptKey): void {
  if (key !== ADOPT) {
    throw new TypeError("this object is created by the MapLibre Native API");
  }
}

interface OwnerState extends RootHolder {
  readonly handle: bigint;
  // Keeps the parent owner reachable while this owner lives.
  readonly parent: Owner | undefined;
  token: object | undefined;
  released: boolean;
}

let stateOf: (owner: Owner) => OwnerState;

export interface OwnerOptions {
  readonly parent?: Owner;
  /** A render session, which the exit path abandons. */
  readonly session?: boolean;
  /** A root such as a runtime, whose first creation arms the exit path. */
  readonly root?: boolean;
  readonly registrations?: Registrations;
}

/** The owner of one native handle. */
export abstract class Owner {
  /**
   * The native handle ID, which runtime events report as their source. It
   * stays readable after close.
   */
  readonly id: bigint;
  // The state lives on the owner, not in a WeakMap keyed by it: ArkTS keeps
  // a WeakMap value alive while it names its key, so an owner whose callback
  // captures it would never be collected.
  readonly #state: OwnerState;

  static {
    stateOf = (owner) => {
      if (!(#state in owner)) {
        throw new TypeError("not an owner of a MapLibre Native handle");
      }
      return owner.#state;
    };
  }

  constructor(
    key: AdoptKey,
    handle: bigint,
    name: string,
    options: OwnerOptions = {},
  ) {
    checkKey(key);
    this.id = handle;
    if (options.root) {
      armExit();
    }
    // The token takes ownership of the handle, so it is the last step that
    // can fail.
    this.#state = {
      handle,
      parent: options.parent,
      token: native.ownerTokenCreate(handle, name, !!options.session),
      released: false,
      roots: new Map(),
    };
    options.registrations?.commit(this.#state);
  }
}

/** The callback roots of an owner, which its registering calls extend. */
export function rootsOf(owner: Owner): RootHolder {
  return stateOf(owner);
}

function released(state: OwnerState): void {
  state.released = true;
  // The release ended the token, whose record the collector can now free.
  state.token = undefined;
}

/**
 * Releases the handle through a synchronous call. A second close does
 * nothing, and a release that throws leaves the handle with the owner.
 */
export function closeSync(
  owner: Owner,
  release: (handle: bigint, token: object | undefined) => void,
) {
  const state = stateOf(owner);
  if (state.released) {
    return;
  }
  release(state.handle, state.token);
  released(state);
}

/**
 * Releases the handle through an asynchronous call, whose Promise reports
 * retirement. Native code consumes the handle when it accepts the call. A
 * release after an accepted one still reaches native code, which reports the
 * handle stale.
 */
export function closeAsync(
  owner: Owner,
  release: (handle: bigint, token: object | undefined) => Promise<void>,
): Promise<void> {
  const state = stateOf(owner);
  let completion: Promise<void>;
  try {
    completion = release(state.handle, state.token);
  } catch (error) {
    return Promise.reject(error);
  }
  released(state);
  return completion;
}

/**
 * Gives a handle that native code transferred to the binding its owner, and
 * disposes the handle when the owner cannot be built.
 */
export function adopt<T>(
  handle: bigint,
  construct: () => T,
  dispose: (handle: bigint) => void,
): T {
  try {
    return construct();
  } catch (error) {
    try {
      dispose(handle);
    } catch {
      // The handle was already consumed, by a token that disposed it.
    }
    throw error;
  }
}

/**
 * Takes the handle that a drained delivery owns from its batch and gives it
 * an owner, as {@link adopt} does.
 */
export function takeDelivered<T>(
  index: number,
  handle: bigint,
  construct: (handle: bigint) => T,
  dispose: (handle: bigint) => void,
): T {
  native.deliveryTake(index);
  return adopt(handle, () => construct(handle), dispose);
}

/**
 * Defines the explicit resource management methods on an owner class, when
 * the host defines their symbols.
 */
export function defineDisposers(
  owner: { prototype: object },
  asyncClose: string | undefined,
  syncClose: string | undefined,
): void {
  const symbols = Symbol as unknown as {
    asyncDispose?: symbol;
    dispose?: symbol;
  };
  const define = (
    symbol: symbol | undefined,
    method: string | undefined,
    value: (this: Record<string, () => unknown>) => unknown,
  ) => {
    if (symbol === undefined || method === undefined) {
      return;
    }
    Object.defineProperty(owner.prototype, symbol, {
      configurable: true,
      writable: true,
      value,
    });
  };
  const asyncMethod = asyncClose ?? syncClose;
  // An async disposer returns a Promise even when it falls back to close().
  define(symbols.asyncDispose, asyncMethod, async function () {
    await this[asyncMethod!]!();
  });
  define(symbols.dispose, syncClose, function () {
    return this[syncClose!]!();
  });
}

/**
 * The scope of a borrowed view, whose native objects stay valid only while
 * {@link ViewScope.use} runs. Native code refuses to release the owner while
 * the scope is open.
 */
export class ViewScope {
  #active = 0;
  readonly #owner: Owner;
  readonly #begin: (handle: bigint) => bigint;
  readonly #end: (scope: bigint) => void;

  constructor(
    key: AdoptKey,
    owner: Owner,
    begin: (handle: bigint) => bigint,
    end: (scope: bigint) => void,
  ) {
    checkKey(key);
    this.#owner = owner;
    this.#begin = begin;
    this.#end = end;
  }

  /** Runs `body` synchronously inside a native view scope. */
  use<T>(body: (scope: this) => T): T {
    const scope = this.#begin(this.#owner.id);
    this.#active++;
    try {
      const result = body(this);
      // Any thenable, from this realm or another, would outlive the scope.
      if (typeof (result as { then?: unknown } | null)?.then === "function") {
        throw new TypeError("a view callback must complete synchronously");
      }
      return result;
    } finally {
      this.#active--;
      this.#end(scope);
    }
  }

  /** Returns `value` inside an active scope, and throws outside one. */
  active<T>(value: T): T {
    if (this.#active === 0) {
      throw new MaplibreError(
        Status.invalidState,
        "a borrowed native value is readable only while its view callback runs",
      );
    }
    return value;
  }
}

let exitArmed = false;

/**
 * Ends native work before exit() on the first root owner: a synchronous exit
 * listener closes every realm's wake shim and abandons every render session.
 */
function armExit(): void {
  if (exitArmed) {
    return;
  }
  exitArmed = true;
  hooks.onExit?.(() => native.shutdownAtExit());
}
