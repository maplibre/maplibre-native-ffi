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
