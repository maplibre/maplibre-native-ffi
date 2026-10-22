@file:OptIn(ExperimentalWasmJsInterop::class)

package org.maplibre.nativeffi.internal.lifecycle

internal actual class WeakBox<T : Any> actual constructor(value: T) {
  private val reference: JsAny = weakRef(value.toJsReference())

  @Suppress("UNCHECKED_CAST") actual fun get(): T? = deref(reference)?.get() as T?
}

private fun weakRef(value: JsReference<Any>): JsAny = js("new WeakRef(value)")

private fun deref(reference: JsAny): JsReference<Any>? = js("reference.deref()")
