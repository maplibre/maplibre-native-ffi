package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

internal actual fun defaultRuntimeEventMask(): RuntimeEventMask {
  NativeAccess.ensureLoaded()
  return MaplibreNativeC.mln_runtime_options_default().use { RuntimeEventMask(it.event_mask()) }
}

internal actual fun defaultMapEventMask(): RuntimeEventMask {
  NativeAccess.ensureLoaded()
  return MaplibreNativeC.mln_map_options_default().use { RuntimeEventMask(it.event_mask()) }
}
