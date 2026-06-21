package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Converts C ABI status values to Kotlin exceptions. */
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

  /** Creates the binding-owned error for a live-state violation. */
  fun invalidState(diagnostic: String): InvalidStateException =
    InvalidStateException(MaplibreStatus.INVALID_STATE.nativeCode, diagnostic)

  /** Creates the binding-owned error for closing a parent with live child handles. */
  fun liveChildren(typeName: String, childCount: Int): InvalidStateException =
    invalidState("$typeName has $childCount live child handle(s)")

  /** Creates a binding-owned invalid-argument error without reading stale C diagnostics. */
  fun invalidArgument(diagnostic: String): InvalidArgumentException =
    InvalidArgumentException(MaplibreStatus.INVALID_ARGUMENT.nativeCode, diagnostic)

  /** Throws the public binding invalid-argument error when a caller input fails validation. */
  inline fun requireArgument(condition: Boolean, diagnostic: () -> String) {
    if (!condition) throw invalidArgument(diagnostic())
  }

  /** Creates the binding-owned error for closing a callback owner from inside its callback. */
  fun callbackReentry(typeName: String): InvalidStateException =
    invalidState("$typeName callback cannot be closed from inside its callback")

  /** Copies the current C thread-local diagnostic into a Kotlin-owned string. */
  fun currentDiagnostic(): String = NativeDiagnostics.currentDiagnostic()
}

/** Platform bridge for copying the native thread-local diagnostic. */
internal expect object NativeDiagnostics {
  fun currentDiagnostic(): String
}
