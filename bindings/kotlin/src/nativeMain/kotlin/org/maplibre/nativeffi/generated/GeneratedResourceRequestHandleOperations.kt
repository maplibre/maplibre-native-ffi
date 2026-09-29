// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.NativeDiagnostics

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedResourceRequestHandleOperations
internal actual constructor() {
  internal actual val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingResourceRequestHandleHandle(): ULong

  internal abstract fun <T> bindingReadResourceRequestHandle(block: (ULong) -> T): T

  internal abstract fun bindingCompleteResourceRequestHandle(call: (ULong) -> Unit)

  internal abstract fun bindingIssuedResourceRequestHandleHandle(): ULong

  internal abstract fun bindingCloseResourceRequestHandle(call: (ULong) -> Unit)

  public actual fun resourceRequestCancelled(): Boolean {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_cancelled",
      )
      return bindingReadResourceRequestHandle { raw ->
        memScoped {
          val arena = this
          val out = alloc<BooleanVar>()
          NativeDiagnostics.check { diagnostic ->
            mln_resource_request_cancelled(raw, out.ptr, diagnostic)
          }
          out.value
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestComplete(response: ResourceResponse): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_complete",
      )
      return bindingCompleteResourceRequestHandle { raw ->
        memScoped {
          val arena = this
          NativeDiagnostics.check { diagnostic ->
            mln_resource_request_complete(
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
        memScoped {
          val arena = this
          mln_resource_request_release(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestSetCancelCallback(
    callback: ResourceRequestCancelCallback
  ): Boolean {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_set_cancel_callback",
      )
      return bindingReadResourceRequestHandle { raw ->
        org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use { registrations ->
          memScoped {
            val token =
              registrations.register(
                GeneratedResourceRequestCancelCallbackRegistration(callback),
                raw.toLong(),
              )
            val out = alloc<BooleanVar>()
            NativeDiagnostics.check { diagnostic ->
              mln_resource_request_set_cancel_callback(
                raw,
                GeneratedDirectCallbacks.ResourceRequestCancelCallbackStub,
                token.toCPointer<ByteVar>(),
                GeneratedDirectCallbacks.ResourceRequestCancelCallbackReleaseStub,
                out.ptr,
                diagnostic,
              )
            }
            val outCancelled = out.value
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
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingIssuedResourceRequestHandleHandle().toLong(),
        "mln_resource_request_wait_until_retired",
      )
      return memScoped {
        val arena = this
        NativeDiagnostics.check { diagnostic ->
          mln_resource_request_wait_until_retired(
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
