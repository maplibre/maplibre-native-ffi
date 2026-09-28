// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import java.lang.foreign.Arena
import java.lang.foreign.MemorySegment
import java.lang.foreign.ValueLayout
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.loader.NativeAccess
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
        Arena.ofConfined().use { arena ->
          val out = arena.allocate(ValueLayout.JAVA_BOOLEAN)
          BindingStatus.check(MapLibreNativeC.mln_resource_request_cancelled(raw, out))
          out.get(ValueLayout.JAVA_BOOLEAN, 0)
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_resource_request_complete(
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
        Arena.ofConfined().use { arena ->
          MapLibreNativeC.mln_resource_request_release(owner)
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
        Arena.ofConfined().use { arena ->
          val out = arena.allocate(ValueLayout.JAVA_BOOLEAN)
          val status =
            MapLibreNativeC.mln_resource_request_set_cancel_callback(
              raw,
              GeneratedDirectCallbacks.ResourceRequestCancelCallbackStub,
              MemorySegment.ofAddress(token),
              out,
            )
          org.maplibre.nativeffi.internal.callback.ResourceRequestCancelSetResult(
            status,
            out.get(ValueLayout.JAVA_BOOLEAN, 0),
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
      return Arena.ofConfined().use { arena ->
        BindingStatus.check(
          MapLibreNativeC.mln_resource_request_wait_until_retired(
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
