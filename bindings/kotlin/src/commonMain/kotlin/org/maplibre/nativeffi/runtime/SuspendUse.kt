package org.maplibre.nativeffi.runtime

import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.withContext
import org.maplibre.nativeffi.map.MapHandle

/** Runs [block] and suspends until this runtime's native teardown has finished. */
public suspend inline fun <T> RuntimeHandle.use(block: suspend (RuntimeHandle) -> T): T =
  try {
    block(this)
  } finally {
    withContext(NonCancellable) { close().await() }
  }

/** Runs [block] and suspends until this map has closed. */
public suspend inline fun <T> MapHandle.use(block: suspend (MapHandle) -> T): T =
  try {
    block(this)
  } finally {
    close()
  }
