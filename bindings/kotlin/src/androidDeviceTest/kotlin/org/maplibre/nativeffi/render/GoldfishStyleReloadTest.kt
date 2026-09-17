package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import org.maplibre.nativeffi.map.MapHandle
import org.maplibre.nativeffi.map.MapMode
import org.maplibre.nativeffi.runtime.CommandDisposition
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.runSuspendTest

class GoldfishStyleReloadTest {
  @Test
  fun repeatedSnapshotStyleReloadRendersComposedLayer(): Unit = runSuspendTest {
    withOwnedTextureSession(
      width = SNAPSHOT_SIZE,
      height = SNAPSHOT_SIZE,
      mapWidth = SNAPSHOT_SIZE,
      mapHeight = SNAPSHOT_SIZE,
      mapMode = MapMode.STATIC,
    ) { runtime, map, owned ->
      val session = owned.session
      loadBaseStyle(runtime, map, session, BASE_STYLE)
      addComposition(runtime, map, session)
      assertContentEquals(GREEN, captureCenterPixel(map, session))

      loadBaseStyle(runtime, map, session, ALTERNATE_STYLE)
      loadBaseStyle(runtime, map, session, BASE_STYLE.copyOf())
      addComposition(runtime, map, session)

      assertContentEquals(GREEN, captureCenterPixel(map, session))
    }
  }

  private suspend fun loadBaseStyle(
    runtime: RuntimeHandle,
    map: MapHandle,
    session: RenderSessionHandle,
    style: ByteArray,
  ) {
    session.completeOnDriver(map.setStyleJson(style))
    session.completeOnDriver(runtime.barrier())
  }

  private suspend fun addComposition(
    runtime: RuntimeHandle,
    map: MapHandle,
    session: RenderSessionHandle,
  ) {
    session.completeOnDriver(map.addStyleSourceJson(COMPOSED_SOURCE_ID, COMPOSED_SOURCE))
    session.completeOnDriver(map.addStyleLayerJson(COMPOSED_LAYER, ""))
    session.completeOnDriver(runtime.barrier())
  }

  /** Renders until the still image the map owes this test finishes, then reads its center pixel. */
  private suspend fun captureCenterPixel(map: MapHandle, session: RenderSessionHandle): ByteArray {
    val still = map.requestStillImage()
    val completion = session.completeOnDriver(still) { renderOneFrame() }
    check(completion.disposition == CommandDisposition.COMMITTED) {
      "still image failed: ${completion.status} ${completion.diagnostic}"
    }

    val readback = session.completeOnDriver(session.readPremultipliedRgba8())
    val center = SNAPSHOT_SIZE / 2 * readback.info.stride + SNAPSHOT_SIZE / 2 * 4
    return readback.bytes.copyOfRange(center, center + 4)
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
