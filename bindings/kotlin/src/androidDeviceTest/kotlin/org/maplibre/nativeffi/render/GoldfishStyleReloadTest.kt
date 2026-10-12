package org.maplibre.nativeffi.render

import kotlin.test.Test
import kotlin.test.assertContentEquals
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runtime.assertCommitted

/**
 * A regression for the emulator's goldfish GL driver: a static map that reloads its style and
 * re-adds a composed layer keeps rendering that layer.
 */
class GoldfishStyleReloadTest {
  @Test
  fun repeatedSnapshotStyleReloadRendersComposedLayer(): Unit = runSuspendTest {
    withOwnedTexture(width = SNAPSHOT_SIZE, height = SNAPSHOT_SIZE) {
      setStyle(BASE_STYLE)
      addComposition()
      assertContentEquals(GREEN, captureCenterPixel())

      setStyle(ALTERNATE_STYLE)
      setStyle(BASE_STYLE)
      addComposition()
      assertContentEquals(GREEN, captureCenterPixel())
    }
  }

  private suspend fun OwnedTextureFixture.addComposition() {
    val map = mapFixture.map
    assertCommitted(
      complete(map.addStyleSourceJson(COMPOSED_SOURCE_ID, COMPOSED_SOURCE.encodeToByteArray()))
    )
    assertCommitted(complete(map.addStyleLayerJson(COMPOSED_LAYER.encodeToByteArray(), "")))
  }

  /** Renders the still image the map owes this test, then reads its center pixel. */
  private suspend fun OwnedTextureFixture.captureCenterPixel(): ByteArray {
    renderStill()
    val readback = complete(session.readTexture())
    val center = SNAPSHOT_SIZE / 2 * readback.info.stride.toInt() + SNAPSHOT_SIZE / 2 * 4
    return readback.data.copyOfRange(center, center + 4)
  }

  private companion object {
    const val SNAPSHOT_SIZE = 64
    const val COMPOSED_SOURCE_ID = "composed-point"
    val GREEN = byteArrayOf(0, -1, 0, -1)
    const val BASE_STYLE =
      """{"version":8,"sources":{},"layers":[{"id":"base","type":"background","paint":{"background-color":"#000000"}}]}"""
    const val ALTERNATE_STYLE =
      """{"version":8,"sources":{},"layers":[{"id":"alternate","type":"background","paint":{"background-color":"#0000ff"}}]}"""
    const val COMPOSED_SOURCE = """{"type":"geojson","data":{"type":"Point","coordinates":[0,0]}}"""
    const val COMPOSED_LAYER =
      """{"id":"composed-circle","type":"circle","source":"composed-point","paint":{"circle-color":"#00ff00","circle-radius":20}}"""
  }
}
