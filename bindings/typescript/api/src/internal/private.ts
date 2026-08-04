/**
 * The bridge between public wrappers and the state they hide.
 *
 * A public wrapper owns native state that no public code may reach: reaching it
 * would let a caller build a second owner for one native handle, and the first
 * owner would then use a released id or destroy it twice. TypeScript's
 * `@internal` is documentation, and a private field is unreachable from a
 * sibling module, so the association lives here in module-private maps that
 * nothing outside this package can name.
 */

import type { HandleState } from "./handle.ts";
import type { Native } from "./native.ts";

const handleStates = new WeakMap<object, HandleState>();
const natives = new WeakMap<object, Native>();
const registries = new WeakMap<
  object,
  { readonly registrationCount: number }
>();

/** @internal Associates a facade with the callback registry it owns. */
export function attachCallbackRegistry(
  wrapper: object,
  registry: { readonly registrationCount: number },
): void {
  registries.set(wrapper, registry);
}

/**
 * @internal How many callback registrations a facade holds.
 *
 * A conformance case reads this to say a registration made for a call that
 * then failed did not outlive it, which nothing in the public API exposes.
 */
export function registrationCountOf(wrapper: object): number {
  const registry = registries.get(wrapper);
  if (registry === undefined) {
    throw new Error("this object owns no callback registry");
  }
  return registry.registrationCount;
}

/** Associates a wrapper with the handle state it owns. */
export function attachHandleState(wrapper: object, state: HandleState): void {
  handleStates.set(wrapper, state);
}

/** Reports a wrapper's handle state, for the modules that build on it. */
export function handleStateOf(wrapper: object): HandleState {
  const state = handleStates.get(wrapper);
  if (state === undefined) {
    throw new TypeError("this object is not a MapLibre handle wrapper");
  }
  return state;
}

/** Associates a facade with the library it loaded. */
export function attachNative(wrapper: object, native: Native): void {
  natives.set(wrapper, native);
}

export function nativeOf(wrapper: object): Native {
  const native = natives.get(wrapper);
  if (native === undefined) {
    throw new TypeError("this object is not a loaded MapLibre library");
  }
  return native;
}

/**
 * The live maps one runtime owns, by their native identity.
 *
 * A map-sourced event carries the id of the map that produced it, and the
 * specification asks the binding to resolve it to the public wrapper when it can
 * prove one is live. The references are weak: a host that drops a map without
 * closing it has leaked it, and holding it here would hide that.
 */
const liveMaps = new WeakMap<object, globalThis.Map<bigint, WeakRef<object>>>();

export function registerMap(runtime: object, id: bigint, map: object): void {
  let maps = liveMaps.get(runtime);
  if (maps === undefined) {
    maps = new globalThis.Map();
    liveMaps.set(runtime, maps);
  }
  maps.set(id, new WeakRef(map));
}

export function unregisterMap(runtime: object, id: bigint): void {
  liveMaps.get(runtime)?.delete(id);
}

/** Reports the live map an id names, or `undefined` when none can be proven. */
export function mapForId(runtime: object, id: bigint): object | undefined {
  const reference = liveMaps.get(runtime)?.get(id);
  if (reference === undefined) {
    return undefined;
  }
  const map = reference.deref();
  if (map === undefined) {
    liveMaps.get(runtime)?.delete(id);
  }
  return map;
}
