package org.maplibre.nativeffi.examples.composemap.surface

internal data class NativeSurfaceFrameLease(
  override val frameId: Long,
  override val extent: SurfaceExtent,
  override val target: NativeSurfaceTarget,
  override val presentationTimeNanos: Long?,
) : NativeSurfaceFrame
