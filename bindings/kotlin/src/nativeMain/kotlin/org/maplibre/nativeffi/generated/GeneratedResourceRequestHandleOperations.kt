// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedResourceRequestHandleOperations
internal actual constructor() {
  internal val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingResourceRequestHandleHandle(): ULong

  internal abstract fun <T> bindingReadResourceRequestHandle(block: (ULong) -> T): T

  internal abstract fun bindingCompleteResourceRequestHandle(call: (ULong) -> Int)

  internal abstract fun bindingRegisterResourceRequestHandleCancel(
    callback: () -> Unit,
    call: (ULong, Long) -> org.maplibre.nativeffi.internal.callback.DecisionCancelSetResult,
  ): Boolean

  internal abstract fun bindingIssuedResourceRequestHandleHandle(): ULong

  internal abstract fun bindingCloseResourceRequestHandle(call: (ULong) -> Int)

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
          BindingStatus.check(mln_resource_request_cancelled(raw, out.ptr))
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
          mln_resource_request_complete(raw, GeneratedValues.writeResourceResponse(arena, response))
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
          0
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun resourceRequestSetCancelCallback(callback: ResourceRequestCancelCallback) {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_set_cancel_callback",
      )
      org.maplibre.nativeffi.internal.callback.CallbackRegistrationScope().use { registrations ->
        val token =
          registrations.register(GeneratedResourceRequestCancelCallbackRegistration(callback))
        BindingStatus.check(
          mln_resource_request_set_cancel_callback(
            GeneratedDirectCallbacks.ResourceRequestCancelCallbackStub,
            token.toCPointer<ByteVar>(),
            GeneratedDirectCallbacks.ResourceRequestCancelCallbackReleaseStub,
          )
        )
        registrations.accept(org.maplibre.nativeffi.internal.callback.CallbackOwner.global)
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
        BindingStatus.check(
          mln_resource_request_wait_until_retired(bindingIssuedResourceRequestHandleHandle())
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
