package org.maplibre.nativeffi.internal.status

import org.maplibre.nativeffi.MaplibreException
import org.maplibre.nativeffi.Status

/**
 * Returns normally for OK and throws the [MaplibreException] for any other [status], reading the
 * call's [diagnostic] only on failure.
 */
internal inline fun checkStatus(status: Int, diagnostic: () -> String) {
  if (status != Status.OK.rawValue) {
    throw MaplibreException(Status(status), diagnostic())
  }
}
