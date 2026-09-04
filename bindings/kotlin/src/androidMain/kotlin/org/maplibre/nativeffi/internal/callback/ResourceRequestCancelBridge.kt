package org.maplibre.nativeffi.internal.callback

import org.bytedeco.javacpp.Pointer
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC

/**
 * Bridges native resource request cancellations to the request cancel registry.
 *
 * One process-wide thunk serves every request, and a registry token travels as its user data.
 * JavaCPP's function pointer pool holds ten slots per generated class, which a per-request thunk
 * would exhaust after ten live requests.
 */
internal object ResourceRequestCancelBridge {
  /** The C callback to register, valid for the life of the process. */
  val stub: MaplibreNativeC.mln_resource_request_cancel_callback =
    object : MaplibreNativeC.mln_resource_request_cancel_callback() {
      override fun call(userData: Pointer?) {
        ResourceRequestCancelRegistry.dispatch(userData?.address() ?: 0L)
      }
    }

  /** Returns the user data that routes a cancellation back to one request's state. */
  fun userData(token: Long): Pointer = JavaCppSupport.addressPointer(token)
}
