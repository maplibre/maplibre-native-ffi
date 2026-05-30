package org.maplibre.nativeffi.render

/** Borrowed Metal texture frame valid only while its frame handle is open. */
public class MetalOwnedTextureFrame
internal constructor(
  private val scope: FrameScope,
  private val generationValue: Long,
  private val widthValue: Int,
  private val heightValue: Int,
  private val scaleFactorValue: Double,
  private val frameIdValue: Long,
  private val textureValue: NativePointer,
  private val deviceValue: NativePointer,
  private val pixelFormatValue: Long,
) {
  /** Session generation preserved as a native `uint64_t` bit pattern in [Long]. */
  public fun generation(): Long = checked { generationValue }

  public fun width(): Int = checked { widthValue }

  public fun height(): Int = checked { heightValue }

  public fun scaleFactor(): Double = checked { scaleFactorValue }

  /** Opaque frame identity preserved as a native `uint64_t` bit pattern in [Long]. */
  public fun frameId(): Long = checked { frameIdValue }

  public fun texture(): NativePointer = checked { textureValue }

  public fun device(): NativePointer = checked { deviceValue }

  /** Backend-native Metal pixel format preserved as a native uint64 bit pattern. */
  public fun pixelFormat(): Long = checked { pixelFormatValue }

  private inline fun <T> checked(block: () -> T): T {
    scope.ensureActive()
    return block()
  }
}
