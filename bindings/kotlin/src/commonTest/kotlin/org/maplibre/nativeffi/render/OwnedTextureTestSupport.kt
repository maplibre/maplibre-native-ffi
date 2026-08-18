package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.map.MapOptions
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions

/** Backend-owned texture frame valid only while this handle is open. */
internal interface OwnedTextureTestFrame : AutoCloseable {
  val width: Int
  val height: Int
  val isClosed: Boolean
}

/** Backend-owned texture session plus the graphics context that keeps it valid. */
internal interface OwnedTextureTestSession : AutoCloseable {
  val session: RenderSessionHandle

  fun acquireFrame(): OwnedTextureTestFrame

  /**
   * Attaches a second owned-texture session on [session]'s map, reusing this fixture's graphics
   * context. Native rejects the second attach; the fixture keeps the live context current.
   */
  fun attachAnotherOwnedTexture(width: Int, height: Int): RenderSessionHandle
}

/**
 * Attaches an owned-texture session for common render tests.
 *
 * Returns null when this source set has no fixture for the loaded native backend, so the common
 * session tests stay compiled on every target.
 */
internal expect object OwnedTextureTestSupport {
  fun attach(map: MapHandle, width: Int, height: Int): OwnedTextureTestSession?
}

internal inline fun withOwnedTextureSession(
  width: Int = 32,
  height: Int = 16,
  mapWidth: Int = 64,
  mapHeight: Int = 64,
  mapMode: MapMode = MapMode.CONTINUOUS,
  crossinline block: (RuntimeHandle, MapHandle, OwnedTextureTestSession) -> Unit,
) {
  val runtime = RuntimeHandle.create(RuntimeOptions())
  try {
    val map =
      MapHandle.create(
        runtime,
        MapOptions().apply {
          this.width = mapWidth
          this.height = mapHeight
          this.mapMode = mapMode
        },
      )
    try {
      val owned = OwnedTextureTestSupport.attach(map, width, height) ?: return
      owned.use { block(runtime, map, owned) }
    } finally {
      map.close()
    }
  } finally {
    runtime.close()
  }
}
