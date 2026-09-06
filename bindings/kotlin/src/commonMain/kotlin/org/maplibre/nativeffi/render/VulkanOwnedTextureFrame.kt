package org.maplibre.nativeffi.render

/** Borrowed Vulkan texture frame valid only while its frame handle is open. */
public class VulkanOwnedTextureFrame
internal constructor(
  private val scope: FrameScope,
  private val generationValue: Long,
  private val widthValue: Int,
  private val heightValue: Int,
  private val scaleFactorValue: Double,
  private val frameIdValue: Long,
  private val imageValue: VulkanHandle,
  private val imageViewValue: VulkanHandle,
  private val deviceValue: NativePointer,
  private val formatValue: Int,
  private val layoutValue: Int,
) {
  /** Session generation preserved as a native `uint64_t` bit pattern in [Long]. */
  public fun generation(): Long = checked { generationValue }

  public fun width(): Int = checked { widthValue }

  public fun height(): Int = checked { heightValue }

  public fun scaleFactor(): Double = checked { scaleFactorValue }

  /** Opaque frame identity preserved as a native `uint64_t` bit pattern in [Long]. */
  public fun frameId(): Long = checked { frameIdValue }

  public fun image(): VulkanHandle = checked { imageValue }

  public fun imageView(): VulkanHandle = checked { imageViewValue }

  public fun device(): NativePointer = checked { deviceValue }

  public fun format(): Int = checked { formatValue }

  public fun layout(): Int = checked { layoutValue }

  private inline fun <T> checked(block: () -> T): T {
    scope.ensureActive()
    return block()
  }
}
