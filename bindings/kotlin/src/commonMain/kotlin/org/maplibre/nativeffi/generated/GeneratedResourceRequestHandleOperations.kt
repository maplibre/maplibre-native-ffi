// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.maplibre.nativeffi.internal.c.C
import org.maplibre.nativeffi.internal.c.UpcallStubs
import org.maplibre.nativeffi.internal.call.*
import org.maplibre.nativeffi.internal.callback.CallbackOwner
import org.maplibre.nativeffi.internal.lifecycle.*
import org.maplibre.nativeffi.internal.memory.*

public abstract class GeneratedResourceRequestHandleOperations internal constructor() {
  internal abstract val binding: DecisionOwnerState
  internal val bindingCallbacks: CallbackOwner = CallbackOwner()

  public fun resourceRequestCancelled(): Boolean =
    nativeCall(this, binding, "mln_resource_request_cancelled", Access.READ) {
      val out = allocate(1)
      check(C.mln_resource_request_cancelled(handle, out, diagnostic))
      readBool(out)
    }

  public fun resourceRequestComplete(response: ResourceResponse): Unit =
    nativeComplete(this, binding, "mln_resource_request_complete") {
      check(C.mln_resource_request_complete(handle, writeResourceResponse(response), diagnostic))
    }

  public fun resourceRequestRelease(): Unit =
    nativeClose(this, binding, "mln_resource_request_release") {
      C.mln_resource_request_release(handle)
    }

  public fun resourceRequestSetCancelCallback(callback: ResourceRequestCancelCallback): Boolean =
    nativeCall(this, binding, "mln_resource_request_set_cancel_callback", Access.READ) {
      val token =
        registrations.register(GeneratedResourceRequestCancelCallbackRegistration(callback), handle)
      val out = allocate(1)
      check(
        C.mln_resource_request_set_cancel_callback(
          handle,
          UpcallStubs.resourceRequestCancelCallback,
          token,
          UpcallStubs.releaseRoot,
          out,
          diagnostic,
        )
      )
      val outCancelled = readBool(out)
      if (!outCancelled) accept(bindingCallbacks)
      outCancelled
    }

  public fun resourceRequestWaitUntilRetired(): Unit =
    nativeCall(this, binding, "mln_resource_request_wait_until_retired", Access.ISSUED) {
      check(C.mln_resource_request_wait_until_retired(handle, diagnostic))
    }
}
