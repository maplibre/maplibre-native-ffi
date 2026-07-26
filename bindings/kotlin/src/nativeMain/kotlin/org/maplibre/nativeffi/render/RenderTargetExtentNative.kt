package org.maplibre.nativeffi.render

import kotlinx.cinterop.ExperimentalForeignApi
import kotlinx.cinterop.UIntVar
import kotlinx.cinterop.alloc
import kotlinx.cinterop.memScoped
import kotlinx.cinterop.ptr
import kotlinx.cinterop.sizeOf
import kotlinx.cinterop.value
import org.maplibre.nativeffi.internal.c.mln_render_target_extent
import org.maplibre.nativeffi.internal.c.mln_render_target_extent_physical_size
import org.maplibre.nativeffi.internal.status.Status

@OptIn(ExperimentalForeignApi::class)
public actual fun RenderTargetExtent.physicalSize(): PhysicalRenderTargetSize = memScoped {
  val nativeExtent = alloc<mln_render_target_extent>()
  nativeExtent.size = sizeOf<mln_render_target_extent>().toUInt()
  nativeExtent.width = width.toUInt()
  nativeExtent.height = height.toUInt()
  nativeExtent.scale_factor = scaleFactor
  val outWidth = alloc<UIntVar>()
  val outHeight = alloc<UIntVar>()
  Status.check(
    mln_render_target_extent_physical_size(nativeExtent.ptr, outWidth.ptr, outHeight.ptr)
  )
  PhysicalRenderTargetSize(outWidth.value.toInt(), outHeight.value.toInt())
}
