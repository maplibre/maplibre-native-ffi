// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.bytedeco.javacpp.BoolPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

public actual abstract class GeneratedResourceRequestHandleOperations
internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingResourceRequestHandleHandle(): Long

  internal abstract fun <T> bindingReadResourceRequestHandle(block: (Long) -> T): T

  internal abstract fun bindingCompleteResourceRequestHandle(call: (Long) -> Unit)

  internal abstract fun bindingIssuedResourceRequestHandleHandle(): Long

  internal abstract fun bindingCloseResourceRequestHandle(call: (Long) -> Unit)

  public actual fun resourceRequestCancelled(): Boolean {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_cancelled",
      )
      return bindingReadResourceRequestHandle { raw ->
        PointerScope().use { arena ->
          val out = BoolPointer(1L)
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_resource_request_cancelled(raw, out, diagnostic)
          }
          out.get(0)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestComplete(response: ResourceResponse): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_complete",
      )
      return bindingCompleteResourceRequestHandle { raw ->
        PointerScope().use { arena ->
          NativeDiagnostics.check { diagnostic ->
            MaplibreNativeC.mln_resource_request_complete(
              raw,
              GeneratedValues.writeResourceResponse(arena, response),
              diagnostic,
            )
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestRelease(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_resource_request_release"
      )
      return bindingCloseResourceRequestHandle { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_resource_request_release",
        )
        PointerScope().use { arena -> MaplibreNativeC.mln_resource_request_release(owner) }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestSetCancelCallback(
    callback: ResourceRequestCancelCallback
  ): Boolean {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_set_cancel_callback",
      )
      return bindingReadResourceRequestHandle { raw ->
        org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use { registrations ->
          PointerScope().use { arena ->
            val token =
              registrations.register(
                GeneratedResourceRequestCancelCallbackRegistration(callback),
                raw.toLong(),
              )
            val out = BoolPointer(1L)
            NativeDiagnostics.check { diagnostic ->
              MaplibreNativeC.mln_resource_request_set_cancel_callback(
                raw,
                GeneratedDirectCallbacks.ResourceRequestCancelCallbackStub,
                org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(token),
                GeneratedDirectCallbacks.ResourceRequestCancelCallbackReleaseStub,
                out,
                diagnostic,
              )
            }
            val outCancelled = out.get(0)
            if (!outCancelled) registrations.accept(bindingCallbacks)
            outCancelled
          }
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestWaitUntilRetired(): Unit {
    try {
      NativeAccess.ensureLoaded()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingIssuedResourceRequestHandleHandle().toLong(),
        "mln_resource_request_wait_until_retired",
      )
      return PointerScope().use { arena ->
        NativeDiagnostics.check { diagnostic ->
          MaplibreNativeC.mln_resource_request_wait_until_retired(
            bindingIssuedResourceRequestHandleHandle(),
            diagnostic,
          )
        }
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
