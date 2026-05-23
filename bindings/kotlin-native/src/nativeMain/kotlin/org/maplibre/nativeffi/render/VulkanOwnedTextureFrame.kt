package org.maplibre.nativeffi.render

/** Borrowed Vulkan texture frame valid only while its frame handle is open. */
public class VulkanOwnedTextureFrame
internal constructor(
  private val scope: FrameScope,
  private val generationValue: ULong,
  private val widthValue: UInt,
  private val heightValue: UInt,
  private val scaleFactorValue: Double,
  private val frameIdValue: ULong,
  private val imageValue: NativePointer,
  private val imageViewValue: NativePointer,
  private val deviceValue: NativePointer,
  private val formatValue: UInt,
  private val layoutValue: UInt,
) {
  public fun generation(): ULong = checked { generationValue }

  public fun width(): UInt = checked { widthValue }

  public fun height(): UInt = checked { heightValue }

  public fun scaleFactor(): Double = checked { scaleFactorValue }

  public fun frameId(): ULong = checked { frameIdValue }

  public fun image(): NativePointer = checked { imageValue }

  public fun imageView(): NativePointer = checked { imageViewValue }

  public fun device(): NativePointer = checked { deviceValue }

  public fun format(): UInt = checked { formatValue }

  public fun layout(): UInt = checked { layoutValue }

  private inline fun <T> checked(block: () -> T): T {
    scope.ensureActive()
    return block()
  }
}
