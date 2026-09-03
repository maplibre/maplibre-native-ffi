package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertTrue
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.runtime.RuntimeEventType
import org.maplibre.nativeffi.runtime.RuntimeHandle

class GoldfishStyleReloadTest {
  @Test
  fun repeatedSnapshotStyleReloadRendersComposedLayer() {
    withOwnedTextureSession(
      width = SNAPSHOT_SIZE,
      height = SNAPSHOT_SIZE,
      mapWidth = SNAPSHOT_SIZE,
      mapHeight = SNAPSHOT_SIZE,
      mapMode = MapMode.STATIC,
    ) { runtime, map, owned ->
      loadBaseStyle(runtime, map, BASE_STYLE)
      addComposition(map)
      assertContentEquals(GREEN, captureCenterPixel(runtime, map, owned.session))

      loadBaseStyle(runtime, map, ALTERNATE_STYLE)
      loadBaseStyle(runtime, map, BASE_STYLE.copyOf())
      addComposition(map)

      assertContentEquals(GREEN, captureCenterPixel(runtime, map, owned.session))
    }
  }

  private fun loadBaseStyle(runtime: RuntimeHandle, map: MapHandle, style: ByteArray) {
    map.setStyleJson(style)
    assertTrue(waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED))
  }

  private fun addComposition(map: MapHandle) {
    map.addStyleSourceJson(COMPOSED_SOURCE_ID, COMPOSED_SOURCE)
    map.addStyleLayerJson(COMPOSED_LAYER, "")
  }

  private fun captureCenterPixel(
    runtime: RuntimeHandle,
    map: MapHandle,
    session: RenderSessionHandle,
  ): ByteArray {
    map.requestStillImage()
    var captured: ByteArray? = null
    repeat(10_000) {
      runtime.pump(0)
      var finished = false
      for (event in runtime.drainEvents().events.filter { it.mapSource == map }) {
        when (event.type) {
          RuntimeEventType.MAP_RENDER_UPDATE_AVAILABLE ->
            if (session.renderUpdate().result == RenderResult.RENDERED) {
              val info = session.textureImageInfo()
              NativeBuffer.allocate(info.byteLength).use { buffer ->
                session.readPremultipliedRgba8(buffer)
                val centerOffset = SNAPSHOT_SIZE / 2 * info.stride + SNAPSHOT_SIZE / 2 * 4
                captured = buffer.toByteArray().copyOfRange(centerOffset, centerOffset + 4)
              }
            }
          RuntimeEventType.MAP_STILL_IMAGE_FINISHED -> finished = true
          RuntimeEventType.MAP_STILL_IMAGE_FAILED -> error(event.message)
        }
      }
      if (finished) return captured ?: error("still image finished without a rendered frame")
      runtime.pump(1)
    }
    error("still image did not finish")
  }

  private companion object {
    private const val SNAPSHOT_SIZE = 64
    private const val COMPOSED_SOURCE_ID = "composed-point"
    private val GREEN = byteArrayOf(0, -1, 0, -1)
    private val BASE_STYLE =
      jsonBytes(
        """{"version":8,"sources":{},"layers":[{"id":"base","type":"background","paint":{"background-color":"#000000"}}]}"""
      )
    private val ALTERNATE_STYLE =
      jsonBytes(
        """{"version":8,"sources":{},"layers":[{"id":"alternate","type":"background","paint":{"background-color":"#0000ff"}}]}"""
      )
    private val COMPOSED_SOURCE =
      jsonBytes("""{"type":"geojson","data":{"type":"Point","coordinates":[0,0]}}""")
    private val COMPOSED_LAYER =
      jsonBytes(
        """{"id":"composed-circle","type":"circle","source":"composed-point","paint":{"circle-color":"#00ff00","circle-radius":20}}"""
      )
  }
}
