// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import org.bytedeco.javacpp.*
import org.bytedeco.javacpp.BoolPointer
import org.maplibre.nativeffi.NativeAccess
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.internal.status.Status as BindingStatus

public actual abstract class GeneratedResourceRequestHandleOperations
internal actual constructor() {
  internal val bindingCallbacks = org.maplibre.nativeffi.internal.callback.CallbackOwner()

  internal abstract fun bindingResourceRequestHandleHandle(): Long

  internal abstract fun bindingCompleteResourceRequestHandle(call: (Long) -> Int)

  internal abstract fun <T> bindingReadResourceRequestHandle(block: (Long) -> T): T

  internal abstract fun bindingRegisterResourceRequestHandleCancel(
    callback: () -> Unit,
    call: (Long, Long) -> org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult,
  ): Boolean

  internal abstract fun bindingIssuedResourceRequestHandleHandle(): Long

  internal abstract fun bindingCloseResourceRequestHandle(call: (Long) -> Int)

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
          BindingStatus.check(MaplibreNativeC.mln_resource_request_cancelled(raw, out))
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
          MaplibreNativeC.mln_resource_request_complete(
            raw,
            GeneratedValues.writeResourceResponse(arena, response),
          )
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
        PointerScope().use { arena ->
          MaplibreNativeC.mln_resource_request_release(owner)
          0
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
      NativeAccess.ensureLoaded()
      val owner = bindingResourceRequestHandleHandle().toLong()
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingResourceRequestHandleHandle().toLong(),
        "mln_resource_request_set_cancel_callback",
      )
      return bindingRegisterResourceRequestHandleCancel({
        val scope =
          org.maplibre.nativeffi.internal.callback.CallbackAdmission.scope(
            owner,
            setOf(
              "mln_resource_request_complete",
              "mln_resource_request_cancelled",
              "mln_resource_request_set_cancel_callback",
              "mln_resource_request_release",
            ),
          )
        try {
          callback()
        } finally {
          scope.close()
        }
      }) { raw, token ->
        PointerScope().use { arena ->
          val out = BoolPointer(1L)
          val status =
            MaplibreNativeC.mln_resource_request_set_cancel_callback(
              raw,
              GeneratedDirectCallbacks.ResourceRequestCancelCallbackStub,
              org.maplibre.nativeffi.internal.javacpp.JavaCppSupport.addressPointer(token),
              out,
            )
          org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult(
            status,
            out.get(0),
          )
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
        BindingStatus.check(
          MaplibreNativeC.mln_resource_request_wait_until_retired(
            bindingIssuedResourceRequestHandleHandle()
          )
        )
        Unit
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
