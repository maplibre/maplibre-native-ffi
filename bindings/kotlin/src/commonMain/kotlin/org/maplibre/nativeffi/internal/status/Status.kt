package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.InvalidStateException
import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.error.MaplibreStatus

/** Converts C ABI status values to Kotlin exceptions. */
internal object Status {
  /**
   * Returns normally for OK and throws the mapped Kotlin exception otherwise, reading the call's
   * [diagnostic] only on failure.
   */
  inline fun check(nativeStatusCode: Int, diagnostic: () -> String) {
    if (nativeStatusCode != MaplibreStatus.OK.nativeCode) {
      throw exception(nativeStatusCode, diagnostic())
    }
  }

  /** Builds the mapped Kotlin exception for a failed status and its call's diagnostic. */
  fun exception(nativeStatusCode: Int, diagnostic: String): MaplibreException =
    MaplibreException.forStatus(
      MaplibreStatus.fromNative(nativeStatusCode),
      nativeStatusCode,
      diagnostic,
    )

  /** Creates the binding-owned error for using an already closed handle. */
  fun released(typeName: String): InvalidStateException =
    InvalidStateException(MaplibreStatus.INVALID_STATE.nativeCode, "$typeName is already closed")

  /** Creates the binding-owned error for a live-state violation. */
  fun invalidState(diagnostic: String): InvalidStateException =
    InvalidStateException(MaplibreStatus.INVALID_STATE.nativeCode, diagnostic)

  /** Creates a binding-owned invalid-argument error. */
  fun invalidArgument(diagnostic: String): InvalidArgumentException =
    InvalidArgumentException(MaplibreStatus.INVALID_ARGUMENT.nativeCode, diagnostic)

  /** Throws the public binding invalid-argument error when a caller input fails validation. */
  inline fun requireArgument(condition: Boolean, diagnostic: () -> String) {
    if (!condition) throw invalidArgument(diagnostic())
  }
}
