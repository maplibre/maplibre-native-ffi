package org.maplibre.nativeffi.map

import kotlin.test.Test
import kotlin.test.assertContentEquals
import kotlin.test.assertFailsWith
import kotlin.test.assertNotNull
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.render.PremultipliedRgba8Image
import org.maplibre.nativeffi.runtime.RuntimeHandle
import org.maplibre.nativeffi.runtime.RuntimeOptions
import org.maplibre.nativeffi.runtime.runSuspendTest
import org.maplibre.nativeffi.runtime.use
import org.maplibre.nativeffi.style.StyleImageOptions

class ImageUploadTest {
  @Test
  fun paddedStyleImagesSurviveArrayReleaseAndCollection(): Unit = runSuspendTest {
    RuntimeHandle.create(RuntimeOptions()).use { runtime ->
      MapHandle.create(runtime, MapOptions().apply { mapMode = MapMode.STATIC }).await().use { map
        ->
        map.setStyleJson("""{"version":8,"sources":{},"layers":[]}""".encodeToByteArray()).await()
        for (width in listOf(2, 128)) {
          val expected = uploadImage(map, width)
          System.gc()
          val copied = assertNotNull(map.copyStyleImagePremultipliedRgba8("image").await())
          assertContentEquals(expected, copied.image.pixels)
        }
      }
    }
  }

  private suspend fun uploadImage(map: MapHandle, width: Int): ByteArray {
    val rowBytes = width * 4
    val stride = rowBytes + 8
    val pixels = ByteArray(stride * width) { 99 }
    val expected = ByteArray(rowBytes * width)
    for (y in 0 until width) {
      for (x in 0 until width) {
        val color = byteArrayOf(x.toByte(), y.toByte(), 0, 255.toByte())
        color.copyInto(pixels, y * stride + x * 4)
        color.copyInto(expected, y * rowBytes + x * 4)
      }
    }
    val image = PremultipliedRgba8Image(width, width, stride, pixels)
    assertFailsWith<InvalidArgumentException> {
      map.setStyleImage("", image, StyleImageOptions()).await()
    }
    map.setStyleImage("image", image, StyleImageOptions()).await()
    // The submit copies the padded rows into native memory, so the caller's array is untouched.
    assertContentEquals(pixels, image.pixels)
    return expected
  }
}
