package org.maplibre.nativeffi

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
  System.gc()
}

// A native callback thread and the leak-report worker have no handler of their own, so their
// thread group passes each report to the default handler.
internal actual fun interceptUncaught(sink: (Throwable) -> Boolean): (() -> Unit)? {
  val previous = Thread.getDefaultUncaughtExceptionHandler()
  Thread.setDefaultUncaughtExceptionHandler { thread, error ->
    when {
      sink(error) -> {}
      previous != null -> previous.uncaughtException(thread, error)
      else -> error.printStackTrace()
    }
  }
  return { Thread.setDefaultUncaughtExceptionHandler(previous) }
}

internal actual fun testBackendName(): String? =
  System.getenv("MLN_FFI_TEST_BACKEND")?.takeIf(String::isNotEmpty)
