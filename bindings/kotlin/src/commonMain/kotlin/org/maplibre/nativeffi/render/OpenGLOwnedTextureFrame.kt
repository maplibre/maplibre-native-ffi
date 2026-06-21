package org.maplibre.nativeffi.render

/** Borrowed OpenGL texture frame valid only while its frame handle is open. */
public class OpenGLOwnedTextureFrame
internal constructor(
  private val scope: FrameScope,
  private val generationValue: Long,
  private val widthValue: Int,
  private val heightValue: Int,
  private val scaleFactorValue: Double,
  private val frameIdValue: Long,
  private val textureValue: Int,
  private val targetValue: Int,
  private val internalFormatValue: Int,
  private val formatValue: Int,
  private val typeValue: Int,
) {
  public fun generation(): Long = active { generationValue }

  public fun width(): Int = active { widthValue }

  public fun height(): Int = active { heightValue }

  public fun scaleFactor(): Double = active { scaleFactorValue }

  public fun frameId(): Long = active { frameIdValue }

  public fun texture(): Int = active { textureValue }

  public fun target(): Int = active { targetValue }

  public fun internalFormat(): Int = active { internalFormatValue }

  public fun format(): Int = active { formatValue }

  public fun type(): Int = active { typeValue }

  private inline fun <T> active(block: () -> T): T {
    scope.ensureActive()
    return block()
  }
}
