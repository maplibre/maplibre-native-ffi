package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.Deferred
import org.maplibre.nativeffi.map.MapHandle

/** Test-scoped lifecycle helper for the eager map-creation result. */
internal suspend inline fun <T> Deferred<MapHandle>.use(block: suspend (MapHandle) -> T): T =
  await().use(block)
