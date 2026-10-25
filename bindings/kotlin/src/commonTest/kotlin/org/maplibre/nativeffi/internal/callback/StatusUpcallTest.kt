package org.maplibre.nativeffi.internal.callback

import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.concurrent.atomics.update
import kotlin.test.Test
import kotlin.test.assertEquals
import org.maplibre.nativeffi.error.CallbackException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.Status
import org.maplibre.nativeffi.interceptUncaught

/** How the upcall of a status callback turns what its host callback does into a status. */
@OptIn(ExperimentalAtomicApi::class)
class StatusUpcallTest {
  private class Host(val run: () -> Unit)

  private fun statusOf(run: () -> Unit): Int {
    val owner = CallbackOwner()
    val root = CallbackRoot(Host(run))
    owner.retain(root)
    val token = CallbackRoots.retain(root)
    try {
      return statusUpcall<Host>(CALLBACK, token, Status.NATIVE_ERROR.rawValue) {
        it.run()
        Status.OK.rawValue
      }
    } finally {
      CallbackRoots.release(token)
    }
  }

  @Test
  fun aStatusExceptionFailsQuietlyAndAnyOtherThrowIsReported() {
    val reports = AtomicReference(emptyList<Throwable?>())
    val restore = interceptUncaught { failure ->
      // Other tests' callbacks may fail concurrently, so only this callback's reports count.
      (failure is CallbackException && failure.callback == CALLBACK).also { taken ->
        if (taken) reports.update { it + failure.cause }
      }
    }
    val bug = IllegalStateException("host bug")
    val notAFailure = MaplibreException(Status.OK, "not a failure")
    try {
      assertEquals(Status.OK.rawValue, statusOf {})
      // The host's answer: native receives the status it carries.
      assertEquals(
        Status.INVALID_ARGUMENT.rawValue,
        statusOf { throw MaplibreException(Status.INVALID_ARGUMENT, "refused") },
      )
      // A bug: native receives the failure status.
      assertEquals(Status.NATIVE_ERROR.rawValue, statusOf { throw bug })
      // A status exception that carries no failure is a bug too.
      assertEquals(Status.NATIVE_ERROR.rawValue, statusOf { throw notAFailure })
    } finally {
      restore?.invoke()
    }
    // Every platform reports on the calling thread, so the reports are in by now.
    if (restore != null) assertEquals(listOf(bug, notAFailure), reports.load())
  }

  private companion object {
    const val CALLBACK = "test_status_callback"
  }
}
