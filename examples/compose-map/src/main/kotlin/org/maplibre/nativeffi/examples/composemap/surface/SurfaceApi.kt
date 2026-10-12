package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.runtime.Composable
import androidx.compose.runtime.remember
import kotlin.math.ceil
import kotlin.math.max
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow

@Composable
public fun rememberNativeSurfaceController(): NativeSurfaceController = remember {
  NativeSurfaceController()
}

public class NativeSurfaceController internal constructor() {
  private val mutableState = MutableStateFlow<NativeSurfaceState>(NativeSurfaceState.Inactive)
  private var requestFrameCallback: (() -> Unit)? = null
  private var disposeCallback: (() -> Unit)? = null

  public val state: StateFlow<NativeSurfaceState> = mutableState

  public fun requestFrame() {
    requestFrameCallback?.invoke()
  }

  public fun dispose() {
    try {
      disposeCallback?.invoke()
    } finally {
      disconnect()
      setState(NativeSurfaceState.Inactive)
    }
  }

  internal fun connect(onRequestFrame: () -> Unit, onDispose: () -> Unit) {
    requestFrameCallback = onRequestFrame
    disposeCallback = onDispose
  }

  internal fun disconnect() {
    requestFrameCallback = null
    disposeCallback = null
  }

  internal fun setState(state: NativeSurfaceState) {
    mutableState.value = state
  }
}

public interface NativeSurfaceRenderer : AutoCloseable {
  public val backend: ProducerBackend

  public fun onSurfaceAvailable(session: NativeSurfaceSession) {}

  public fun onSurfaceChanged(extent: SurfaceExtent) {}

  public fun render(frame: NativeSurfaceFrame): NativeSurfaceRenderResult

  public fun onSurfaceLost() {}

  override fun close() {
    onSurfaceLost()
  }
}

public interface NativeSurfaceSession {
  public val backend: ProducerBackend

  public val capabilities: NativeSurfaceCapabilities

  public fun requestFrame()

  public fun <T> withRendererAccess(action: () -> T): T
}

public interface NativeSurfaceFrame {
  public val frameId: Long

  public val extent: SurfaceExtent

  public val target: NativeSurfaceTarget

  public val presentationTimeNanos: Long?
}

public sealed interface NativeSurfaceRenderResult {
  /** A frame reached [target], the ring slot of the frame's target that the consumer draws. */
  public data class Rendered(public val target: NativeSurfaceTarget) : NativeSurfaceRenderResult

  public data object Skipped : NativeSurfaceRenderResult
}

public sealed interface NativeSurfaceState {
  public data object Inactive : NativeSurfaceState

  public data class Ready(
    public val backend: ProducerBackend,
    public val capabilities: NativeSurfaceCapabilities,
  ) : NativeSurfaceState

  public data class Unsupported(
    public val requestedBackend: ProducerBackend,
    public val host: NativeSurfaceHost,
  ) : NativeSurfaceState

  public data class Failed(public val message: String, public val cause: Throwable? = null) :
    NativeSurfaceState
}

public enum class ProducerBackend {
  METAL,
  VULKAN,
  OPENGL,
}

public enum class ConsumerBackend {
  METAL,
  DIRECT3D12,
  OPENGL,
}

public data class NativeSurfaceCapabilities(
  public val producerBackend: ProducerBackend,
  public val consumerBackend: ConsumerBackend,
  public val supportsExplicitSynchronization: Boolean,
  public val supportsResizeWithoutRecreate: Boolean,
)

public data class NativeSurfaceHost(
  public val operatingSystem: NativeSurfaceOperatingSystem,
  public val consumerBackend: ConsumerBackend,
)

public enum class NativeSurfaceOperatingSystem {
  MACOS,
  LINUX,
  WINDOWS,
  UNSUPPORTED,
}

public data class SurfaceExtent(
  public val width: Int,
  public val height: Int,
  public val scaleFactor: Double,
  public val physicalWidth: Int = physicalDimension(width, scaleFactor),
  public val physicalHeight: Int = physicalDimension(height, scaleFactor),
) {
  public val isEmpty: Boolean
    get() =
      width <= 0 ||
        height <= 0 ||
        physicalWidth <= 0 ||
        physicalHeight <= 0 ||
        !(scaleFactor > 0.0) ||
        !scaleFactor.isFinite()

  public fun log(label: String) {
    println(
      "$label: logical=${width}x${height} physical=${physicalWidth}x${physicalHeight} scale=${"%.2f".format(scaleFactor)}"
    )
  }

  public companion object {
    public val Empty: SurfaceExtent = SurfaceExtent(0, 0, 1.0, 0, 0)

    public fun fromPhysical(
      physicalWidth: Int,
      physicalHeight: Int,
      scaleFactor: Double,
    ): SurfaceExtent {
      val scale = if (scaleFactor > 0.0 && scaleFactor.isFinite()) scaleFactor else 1.0
      val safePhysicalWidth = physicalWidth.coerceAtLeast(0)
      val safePhysicalHeight = physicalHeight.coerceAtLeast(0)
      val logicalWidth =
        if (safePhysicalWidth == 0) 0 else max(1, ceil(safePhysicalWidth / scale).toInt())
      val logicalHeight =
        if (safePhysicalHeight == 0) 0 else max(1, ceil(safePhysicalHeight / scale).toInt())
      val normalizedPhysicalWidth =
        if (logicalWidth == 0) 0 else physicalDimension(logicalWidth, scale)
      val normalizedPhysicalHeight =
        if (logicalHeight == 0) 0 else physicalDimension(logicalHeight, scale)
      return SurfaceExtent(
        logicalWidth,
        logicalHeight,
        scale,
        normalizedPhysicalWidth,
        normalizedPhysicalHeight,
      )
    }
  }
}

private fun physicalDimension(logicalSize: Int, scaleFactor: Double): Int =
  max(1, ceil(logicalSize.coerceAtLeast(1) * scaleFactor).toInt())

@JvmInline public value class NativeHandle(public val address: Long)

/**
 * The ring of textures a bridge lends the producer for one frame. The producer renders into one
 * slot at a time, and [slot] names the texture of a slot for the consumer to draw.
 */
public sealed interface NativeSurfaceTarget {
  public val backend: ProducerBackend

  public val extent: SurfaceExtent

  public val generation: Long

  /** The number of textures in the ring. */
  public val ringDepth: Int

  /** The ring slot whose texture is in front. */
  public val slotIndex: Int

  /** This target with the texture of ring slot [index] in front. */
  public fun slot(index: Int): NativeSurfaceTarget
}

/**
 * A ring of Metal textures and the Skiko device they were allocated on. A bridge allocates a new
 * ring on every resize, so a producer keeping per-device state compares [device] rather than the
 * textures. [texture] is the texture in front, the first of [ring] unless [slot] chose another.
 */
public data class MetalTextureTarget(
  public val texture: NativeHandle,
  public val device: NativeHandle,
  public val pixelFormat: Long,
  public val origin: TextureOrigin = TextureOrigin.TOP_LEFT,
  override val extent: SurfaceExtent,
  override val generation: Long,
  public val ring: List<NativeHandle> = listOf(texture),
  override val slotIndex: Int = 0,
) : NativeSurfaceTarget {
  override val backend: ProducerBackend = ProducerBackend.METAL

  override val ringDepth: Int
    get() = ring.size

  override fun slot(index: Int): MetalTextureTarget = copy(texture = ring[index], slotIndex = index)
}

public enum class TextureOrigin {
  TOP_LEFT,
  BOTTOM_LEFT,
}

/** One Vulkan image of a ring and its view. */
public data class VulkanImageSlot(
  public val image: NativeHandle,
  public val imageView: NativeHandle,
)

/**
 * A ring of Vulkan images. [image] and [imageView] are the image in front, the first of [ring]
 * unless [slot] chose another.
 */
public data class VulkanImageTarget(
  public val context: VulkanContextHandles,
  public val image: NativeHandle,
  public val imageView: NativeHandle,
  public val format: Int,
  public val initialLayout: Int,
  public val finalLayout: Int,
  public val queueFamilyIndex: Int,
  override val extent: SurfaceExtent,
  override val generation: Long,
  public val ring: List<VulkanImageSlot> = listOf(VulkanImageSlot(image, imageView)),
  override val slotIndex: Int = 0,
) : NativeSurfaceTarget {
  override val backend: ProducerBackend = ProducerBackend.VULKAN

  override val ringDepth: Int
    get() = ring.size

  override fun slot(index: Int): VulkanImageTarget =
    copy(image = ring[index].image, imageView = ring[index].imageView, slotIndex = index)
}

public data class VulkanContextHandles(
  public val instance: NativeHandle,
  public val physicalDevice: NativeHandle,
  public val device: NativeHandle,
  public val graphicsQueue: NativeHandle,
  public val graphicsQueueFamilyIndex: Int,
  public val getInstanceProcAddr: NativeHandle,
  public val getDeviceProcAddr: NativeHandle,
)

public fun interface OpenGlContextProvider {
  public fun makeCurrent()
}

public sealed interface OpenGlContextHandles

public data class EglContextHandles(
  public val display: NativeHandle,
  public val config: NativeHandle,
  public val shareContext: NativeHandle,
  public val getProcAddress: NativeHandle,
) : OpenGlContextHandles

public data class WglContextHandles(
  public val deviceContext: NativeHandle,
  public val shareContext: NativeHandle,
  public val getProcAddress: NativeHandle,
) : OpenGlContextHandles

/**
 * A ring of OpenGL textures. [textureName] is the texture in front, the first of [ring] unless
 * [slot] chose another.
 */
public data class OpenGlTextureTarget(
  public val context: OpenGlContextHandles,
  public val textureName: Int,
  public val textureTarget: Int,
  public val format: Int,
  public val origin: TextureOrigin = TextureOrigin.TOP_LEFT,
  public val contextProvider: OpenGlContextProvider,
  override val extent: SurfaceExtent,
  override val generation: Long,
  public val ring: List<Int> = listOf(textureName),
  override val slotIndex: Int = 0,
) : NativeSurfaceTarget {
  override val backend: ProducerBackend = ProducerBackend.OPENGL

  override val ringDepth: Int
    get() = ring.size

  override fun slot(index: Int): OpenGlTextureTarget =
    copy(textureName = ring[index], slotIndex = index)
}
