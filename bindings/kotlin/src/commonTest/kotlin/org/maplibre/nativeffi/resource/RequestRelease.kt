package org.maplibre.nativeffi.resource

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.memory.NativeArena

/** Whether native has released the request handle [raw], which then refuses every call. */
internal fun requestIsReleased(raw: Long): Boolean =
  NativeArena().use { arena ->
    C.mln_resource_request_is_cancelled(raw, arena.allocate(1), 0L) != MaplibreStatus.OK.nativeCode
  }
