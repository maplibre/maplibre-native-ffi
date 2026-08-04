package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.BrowserModule
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderTargetExtent

/**
 * Calls the C API's own scaling, so a browser host derives the same physical size as every other
 * platform rather than repeating the formula and rounding differently.
 *
 * The lowered signature is `(i32, i32, i32) -> i32`: a pointer to the extent's three fields, then
 * the two out-pointers, returning a status. Nothing here is proxied to a worker, because this call
 * touches no runtime state and so has no owner thread to reach.
 */
@JsFun(
  "(extent, outWidth, outHeight) => " +
    "globalThis.__maplibreNativeC._mln_render_target_extent_physical_size(extent, outWidth, outHeight)"
)
private external fun physicalSize(extent: Int, outWidth: Int, outHeight: Int): Int

// The two output words follow the descriptor in one allocation, so this costs one scratch
// acquisition rather than three.
private val OUT_WIDTH_OFFSET = MlnRenderTargetExtent.SIZEOF
private val OUT_HEIGHT_OFFSET = MlnRenderTargetExtent.SIZEOF + 4
private val SCRATCH_SIZE = MlnRenderTargetExtent.SIZEOF + 8

public actual fun RenderTargetExtent.physicalSize(): PhysicalRenderTargetSize {
  BrowserModule.require()
  return Heap.withScratch(SCRATCH_SIZE) { scratch ->
    // The leading `size` field is how the C API versions a descriptor, so it carries the size the
    // binding was generated against rather than being left zero.
    MlnRenderTargetExtent.setSize(scratch, MlnRenderTargetExtent.SIZEOF)
    MlnRenderTargetExtent.setWidth(scratch, width)
    MlnRenderTargetExtent.setHeight(scratch, height)
    MlnRenderTargetExtent.setScaleFactor(scratch, scaleFactor)
    Status.check(
      physicalSize(
        scratch.address,
        (scratch + OUT_WIDTH_OFFSET).address,
        (scratch + OUT_HEIGHT_OFFSET).address,
      )
    )
    PhysicalRenderTargetSize(
      Heap.loadInt(scratch + OUT_WIDTH_OFFSET),
      Heap.loadInt(scratch + OUT_HEIGHT_OFFSET),
    )
  }
}
