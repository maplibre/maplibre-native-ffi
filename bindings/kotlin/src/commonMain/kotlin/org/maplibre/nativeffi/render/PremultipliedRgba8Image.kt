package org.maplibre.nativeffi.render

import org.maplibre.nativeffi.internal.status.Status

/** Caller-owned premultiplied RGBA8 pixels. */
public class PremultipliedRgba8Image(
  public val width: Int,
  public val height: Int,
  public val stride: Int,
  pixels: ByteArray,
) {
  private val pixelBytes: ByteArray = pixels.copyOf()

  public val pixels: ByteArray
    get() = pixelBytes.copyOf()

  init {
    Status.requireArgument(width > 0 && height > 0) { "width and height must be positive" }
    Status.requireArgument(stride >= 0) { "stride must be non-negative" }
    val rowBytes = width.toLong() * 4L
    Status.requireArgument(stride.toLong() >= rowBytes) { "stride must be at least width * 4" }
    val requiredBytes = if (height == 1) rowBytes else (height.toLong() - 1L) * stride + rowBytes
    Status.requireArgument(pixels.size.toLong() >= requiredBytes) {
      "pixels length must include every row's pixel bytes"
    }
  }

  override fun equals(other: Any?): Boolean =
    other is PremultipliedRgba8Image &&
      width == other.width &&
      height == other.height &&
      stride == other.stride &&
      pixelBytes.contentEquals(other.pixelBytes)

  override fun hashCode(): Int {
    var result = width.hashCode()
    result = 31 * result + height.hashCode()
    result = 31 * result + stride.hashCode()
    result = 31 * result + pixelBytes.contentHashCode()
    return result
  }

  override fun toString(): String =
    "PremultipliedRgba8Image(width=$width, height=$height, stride=$stride, pixels=${pixelBytes.size} bytes)"
}
