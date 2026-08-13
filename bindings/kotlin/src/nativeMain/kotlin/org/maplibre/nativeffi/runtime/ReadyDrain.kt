package org.maplibre.nativeffi.runtime

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.ULongVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.get
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.mln_notification_source_drain_ready
import org.maplibre.nativeffi.internal.c.mln_ready_batch_get
import org.maplibre.nativeffi.internal.c.mln_ready_batch_release
import org.maplibre.nativeffi.internal.c.mln_ready_batch_view
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class)
internal fun drainNativeReady(source: ULong): List<ReadyEndpoint> = memScoped {
  val outBatch = alloc<ULongVar>()
  outBatch.value = 0uL
  Status.check(mln_notification_source_drain_ready(source, outBatch.ptr))
  try {
    val view = alloc<mln_ready_batch_view>()
    view.size = sizeOf<mln_ready_batch_view>().toUInt()
    Status.check(mln_ready_batch_get(outBatch.value, view.ptr))
    List(view.endpoint_count.toInt()) { index ->
      val endpoint = view.endpoints!![index]
      ReadyEndpoint(ReadyEndpoint.Kind.fromNative(endpoint.kind.toInt()), endpoint.id.toLong())
    }
  } finally {
    mln_ready_batch_release(outBatch.value)
  }
}
