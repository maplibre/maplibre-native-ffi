// Generated from the C headers by tools/bindgen. Do not edit.
package org.maplibre.nativeffi.generated

import kotlinx.cinterop.*
import org.maplibre.nativeffi.generated.*
import org.maplibre.nativeffi.internal.c.*
import org.maplibre.nativeffi.internal.callback.*
import org.maplibre.nativeffi.internal.status.NativeDiagnostics
import platform.posix.size_t
import platform.posix.size_tVar

@OptIn(ExperimentalForeignApi::class)
public actual abstract class GeneratedRenderFrameBatchOperations internal actual constructor() {
  internal abstract fun bindingRenderFrameBatchHandle(): ULong

  internal abstract fun bindingCloseRenderFrameBatch(call: (ULong) -> Unit)

  public actual fun count(): ULong {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderFrameBatchHandle().toLong(),
        "mln_render_frame_batch_count",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<size_tVar>()
        NativeDiagnostics.check { diagnostic ->
          mln_render_frame_batch_count(bindingRenderFrameBatchHandle(), output.ptr, diagnostic)
        }
        output.value.toULong()
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun get(indexValue: ULong): RenderFrameResult {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
        bindingRenderFrameBatchHandle().toLong(),
        "mln_render_frame_batch_get",
      )
      return memScoped {
        val arena = this
        val output = arena.alloc<mln_render_frame_result>()
        output.size = sizeOf<mln_render_frame_result>().toUInt()
        NativeDiagnostics.check { diagnostic ->
          mln_render_frame_batch_get(
            bindingRenderFrameBatchHandle(),
            indexValue.convert<size_t>(),
            output.ptr,
            diagnostic,
          )
        }
        GeneratedValues.readRenderFrameResult(output)
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }

  public actual fun release(): Unit {
    try {
      org.maplibre.nativeffi.internal.callback.CallbackAdmission.checkOperation(
        "mln_render_frame_batch_release"
      )
      return bindingCloseRenderFrameBatch { owner ->
        org.maplibre.nativeffi.internal.callback.CallbackAdmission.check(
          owner.toLong(),
          "mln_render_frame_batch_release",
        )
        memScoped {
          val arena = this
          mln_render_frame_batch_release(owner)
        }
      }
    } finally {
      org.maplibre.nativeffi.internal.lifecycle.bindingKeepAlive(this)
    }
  }
}
