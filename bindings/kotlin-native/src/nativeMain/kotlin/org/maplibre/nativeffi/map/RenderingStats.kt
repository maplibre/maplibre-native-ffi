package org.maplibre.nativeffi.map

/** Rendering statistics copied from a runtime event. */
public data class RenderingStats(
  public val encodingTime: Double,
  public val renderingTime: Double,
  public val frameCount: Long,
  public val drawCallCount: Long,
  public val totalDrawCallCount: Long,
)
