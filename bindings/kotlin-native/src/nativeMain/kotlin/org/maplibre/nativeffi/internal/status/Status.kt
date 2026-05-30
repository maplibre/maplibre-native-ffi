package org.maplibre.nativeffi.internal.status

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.toKString
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.mln_thread_last_error_message

/** Converts C ABI status values to Kotlin exceptions. */
@OptIn(ExperimentalForeignApi::class)
internal object Status {
  /** Returns normally for OK and throws the mapped Kotlin exception otherwise. */
  fun check(nativeStatusCode: Int) {
    if (nativeStatusCode == MaplibreStatus.OK.nativeCode) {
      return
    }

    throw exception(nativeStatusCode)
  }

  /** Builds the mapped Kotlin exception and copies the current thread diagnostic immediately. */
  fun exception(nativeStatusCode: Int): MaplibreException {
    val status = MaplibreStatus.fromNative(nativeStatusCode)
    val diagnostic = currentDiagnostic()
    return MaplibreException.forStatus(status, nativeStatusCode, diagnostic)
  }

  /** Creates the binding-owned error for using an already closed handle. */
  fun released(typeName: String): InvalidStateException =
    InvalidStateException(MaplibreStatus.INVALID_STATE.nativeCode, "$typeName is already closed")

  /** Copies the current C thread-local diagnostic into a Kotlin-owned string. */
  fun currentDiagnostic(): String = mln_thread_last_error_message()?.toKString().orEmpty()
}
