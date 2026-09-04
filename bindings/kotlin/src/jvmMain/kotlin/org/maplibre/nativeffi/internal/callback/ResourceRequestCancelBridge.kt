package org.maplibre.nativeffi.internal.callback

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancel_callback

/**
 * Bridges native resource request cancellations to the request cancel registry.
 *
 * One process-wide upcall stub in the global arena serves every request, and a registry token
 * travels as its user data. A per-request stub would need an arena that closes while a cancellation
 * still runs, on a MapLibre thread that no binding call brackets.
 */
internal object ResourceRequestCancelBridge {
  /** The C callback pointer to register, valid for the life of the process. */
  val stub: MemorySegment =
    mln_resource_request_cancel_callback.allocate(
      { userData -> ResourceRequestCancelRegistry.dispatch(userData.address()) },
      Arena.global(),
    )

  /** Returns the user data that routes a cancellation back to one request's state. */
  fun userData(token: Long): MemorySegment = MemorySegment.ofAddress(token)
}
