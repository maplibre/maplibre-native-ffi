package org.maplibre.nativeffi.render

/** Borrowed Metal texture frame valid only while its frame handle is open. */
public class MetalOwnedTextureFrame
internal constructor(
  private val scope: FrameScope,
  private val generationValue: ULong,
  private val widthValue: UInt,
  private val heightValue: UInt,
  private val scaleFactorValue: Double,
  private val frameIdValue: ULong,
  private val textureValue: NativePointer,
  private val deviceValue: NativePointer,
  private val pixelFormatValue: ULong,
) {
  public fun generation(): ULong = checked { generationValue }

  public fun width(): UInt = checked { widthValue }

  public fun height(): UInt = checked { heightValue }

  public fun scaleFactor(): Double = checked { scaleFactorValue }

  public fun frameId(): ULong = checked { frameIdValue }

  public fun texture(): NativePointer = checked { textureValue }

  public fun device(): NativePointer = checked { deviceValue }

  public fun pixelFormat(): ULong = checked { pixelFormatValue }

  private inline fun <T> checked(block: () -> T): T {
    scope.ensureActive()
    return block()
  }
}
