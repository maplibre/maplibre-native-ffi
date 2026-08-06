package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.generated.MlnRenderTargetExtent
import org.maplibre.nativeffi.internal.wasm.generated.mln_render_target_extent_physical_size

// The two output words follow the descriptor in one allocation, so this costs one scratch
// acquisition rather than three.
private val OUT_WIDTH_OFFSET = MlnRenderTargetExtent.SIZEOF
private val OUT_HEIGHT_OFFSET = MlnRenderTargetExtent.SIZEOF + 4
private val SCRATCH_SIZE = MlnRenderTargetExtent.SIZEOF + 8

/**
 * Calls the C API's own scaling, so a browser host derives the same physical size as every other
 * platform rather than repeating the formula and rounding differently.
 */
public actual fun RenderTargetExtent.physicalSize(): PhysicalRenderTargetSize =
  Heap.withScratch(SCRATCH_SIZE) { scratch ->
    // The leading `size` field is how the C API versions a descriptor, so it carries the size the
    // binding was generated against rather than being left zero.
    MlnRenderTargetExtent.setSize(scratch, MlnRenderTargetExtent.SIZEOF)
    MlnRenderTargetExtent.setWidth(scratch, width)
    MlnRenderTargetExtent.setHeight(scratch, height)
    MlnRenderTargetExtent.setScaleFactor(scratch, scaleFactor)
    Status.check(
      mln_render_target_extent_physical_size(
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
