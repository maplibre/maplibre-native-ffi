@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.lifecycle

// The page runs each FinalizationRegistry callback as a task of its own, never inside a C call,
// and disposal works from any thread, so a callback disposes directly. The registry holds each
// state strongly, and a state never holds its owner.

internal actual fun disposeWhenUnreachable(owner: Any, state: HandleState): Any {
  register(registry, owner.toJsReference(), state.toJsReference())
  return Unit
}

private val registry: JsAny = finalizationRegistry { state -> state.get().disposeLeaked() }

private fun finalizationRegistry(dispose: (JsReference<HandleState>) -> Unit): JsAny =
  js("new FinalizationRegistry(dispose)")

private fun register(
  registry: JsAny,
  target: JsReference<Any>,
  held: JsReference<HandleState>,
): Unit = js("registry.register(target, held)")
