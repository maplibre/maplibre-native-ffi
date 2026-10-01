package org.maplibre.nativeffi.internal.lifecycle

import java.lang.ref.WeakReference

internal actual class WeakBox<T : Any> actual constructor(value: T) {
  private val reference = WeakReference(value)

  actual fun get(): T? = reference.get()
}
