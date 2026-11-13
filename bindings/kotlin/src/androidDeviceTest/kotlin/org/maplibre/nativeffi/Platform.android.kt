package org.maplibre.nativeffi

import androidx.test.platform.app.InstrumentationRegistry
import java.lang.ref.WeakReference

internal actual class TestThread actual constructor(block: () -> Unit) {
  @Volatile private var failure: Throwable? = null
  private val thread =
    Thread(
        {
          try {
            block()
          } catch (error: Throwable) {
            failure = error
          }
        },
        "maplibre-test-background",
      )
      .apply { start() }

  actual fun join() {
    thread.join()
    failure?.let { throw it }
  }
}

internal actual class TestWeakReference actual constructor(value: Any) {
  private val reference = WeakReference(value)

  actual val value: Any?
    get() = reference.get()
}

internal actual suspend fun requestCollection() {
  Runtime.getRuntime().gc()
}

internal actual fun interceptUncaught(sink: (Throwable) -> Boolean): (() -> Unit)? = null

internal actual fun testBackendName(): String? =
  InstrumentationRegistry.getArguments().getString("mlnTestBackend")?.takeIf(String::isNotEmpty)
