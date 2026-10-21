package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.error.MaplibreException
import org.maplibre.nativeffi.generated.Status

/**
 * Returns normally for OK and throws the [MaplibreException] for any other [status], reading the
 * call's [diagnostic] only on failure.
 */
internal inline fun checkStatus(status: Int, diagnostic: () -> String) {
  if (status != Status.OK.rawValue) {
    throw MaplibreException(Status(status), diagnostic())
  }
}

/** The error for a state the binding refuses before it reaches native. */
internal fun invalidState(diagnostic: String): MaplibreException =
  MaplibreException(Status.INVALID_STATE, diagnostic)

/** The error for an input the binding cannot hand to native. */
internal fun invalidArgument(diagnostic: String): MaplibreException =
  MaplibreException(Status.INVALID_ARGUMENT, diagnostic)
