package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException

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
  fun liveChildren(typeName: String, childTypeNames: List<String>): InvalidStateException {
    val summary =
      childTypeNames
        .groupingBy { it }
        .eachCount()
        .entries
        .sortedBy { it.key }
        .joinToString(", ") { (name, count) -> if (count == 1) name else "$name x$count" }
    return invalidState("$typeName has ${childTypeNames.size} live child handle(s): $summary")
  }

  /** Creates a binding-owned invalid-argument error without reading stale C diagnostics. */
  fun invalidArgument(diagnostic: String): InvalidArgumentException =
    InvalidArgumentException(MaplibreStatus.INVALID_ARGUMENT.nativeCode, diagnostic)

  /** Throws the public binding invalid-argument error when a caller input fails validation. */
  inline fun requireArgument(condition: Boolean, diagnostic: () -> String) {
    if (!condition) throw invalidArgument(diagnostic())
  }

  /**
   * Creates a binding-owned unsupported error without reaching native for a diagnostic.
   *
   * Some inputs are shaped by the common API but meaningful on only one platform: a WebGL context
   * names an entry in the browser module's own table, and no desktop or mobile target has that
   * table to look it up in. The binding refuses those here rather than passing them down, because
   * what native would receive is a well-formed descriptor naming something that does not exist.
   */
  fun unsupported(diagnostic: String): UnsupportedFeatureException =
    UnsupportedFeatureException(MaplibreStatus.UNSUPPORTED.nativeCode, diagnostic)

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
