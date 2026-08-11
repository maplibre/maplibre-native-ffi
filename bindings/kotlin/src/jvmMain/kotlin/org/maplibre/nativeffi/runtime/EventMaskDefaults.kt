package org.maplibre.nativeffi.runtime

import org.maplibre.nativeffi.internal.loader.NativeAccess

internal actual fun defaultRuntimeEventMask(): RuntimeEventMask =
  RuntimeEventMask(NativeAccess.defaultRuntimeOptionsEventMask())

internal actual fun defaultMapEventMask(): RuntimeEventMask =
  RuntimeEventMask(NativeAccess.defaultMapOptionsEventMask())
