package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.error.UnsupportedFeatureException

/**
 * Metal session-owned texture frames do not exist in a browser build.
 *
 * MapLibre Native compiles one render backend per build, and the browser target compiles OpenGL
 * against WebGL. Nothing in this build can attach a Metal render target, so nothing can produce one
 * of these frames. The type exists because the common API declares it, and it has no constructor a
 * caller could reach -- which is the binding reporting the build's real capability rather than
 * inventing a rule of its own.
 */
public actual class MetalOwnedTextureFrameHandle private constructor() : AutoCloseable {
  public actual fun frame(): MetalOwnedTextureFrame =
    throw UnsupportedFeatureException(
      MaplibreStatus.UNSUPPORTED.nativeCode,
      "Metal render targets are not supported by the browser build of MapLibre Native",
    )

  /** Always closed: no instance is reachable, so none is ever open. */
  public actual val isClosed: Boolean
    get() = true

  public actual override fun close() {
    // Unreachable; no instance exists.
  }
}
