package org.maplibre.nativeffi.internal.lifecycle

import kotlin.experimental.ExperimentalNativeApi
import kotlin.native.ref.WeakReference

@OptIn(ExperimentalNativeApi::class)
internal actual class WeakBox<T : Any> actual constructor(value: T) {
  private val reference = WeakReference(value)

  actual fun get(): T? = reference.value
}
