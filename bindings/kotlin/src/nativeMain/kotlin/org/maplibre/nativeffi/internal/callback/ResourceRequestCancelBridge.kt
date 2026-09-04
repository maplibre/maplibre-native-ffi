package org.maplibre.nativeffi.internal.callback

import kotlinx.cinterop.COpaquePointer
import kotlinx.cinterop.CPointed
import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.staticCFunction
import kotlinx.cinterop.toCPointer
import kotlinx.cinterop.toLong
import org.maplibre.nativeffi.internal.c.mln_resource_request_cancel_callback

/**
 * Bridges native resource request cancellations to the request cancel registry.
 *
 * One process-wide C function serves every request, and a registry token travels as its user data.
 * A per-request stable reference would have to be disposed while a cancellation still runs, on a
 * MapLibre thread that no binding call brackets.
 */
@OptIn(ExperimentalForeignApi::class)
internal object ResourceRequestCancelBridge {
  /** The C callback pointer to register, valid for the life of the process. */
  val stub: mln_resource_request_cancel_callback = staticCFunction(::dispatchResourceRequestCancel)

  /** Returns the user data that routes a cancellation back to one request's state. */
  fun userData(token: Long): COpaquePointer? = token.toCPointer<CPointed>()
}

@OptIn(ExperimentalForeignApi::class)
private fun dispatchResourceRequestCancel(userData: COpaquePointer?) {
  ResourceRequestCancelRegistry.dispatch(userData?.toLong() ?: 0L)
}
