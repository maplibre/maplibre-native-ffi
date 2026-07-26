package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.NativeAccess

public actual fun RenderTargetExtent.physicalSize(): PhysicalRenderTargetSize {
  NativeAccess.ensureLoaded()
  val (width, height) = NativeAccess.renderTargetExtentPhysicalSize(this)
  return PhysicalRenderTargetSize(width, height)
}
