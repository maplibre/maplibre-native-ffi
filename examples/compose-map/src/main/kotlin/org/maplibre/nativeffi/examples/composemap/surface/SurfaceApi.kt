package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.runtime.Composable
import kotlinx.coroutines.flow.StateFlow

@Composable
public fun rememberNativeSurfaceController(): NativeSurfaceController =
  rememberNativeSurfaceControllerImpl()

public interface NativeSurfaceController {
  public val state: StateFlow<NativeSurfaceState>

  public fun requestFrame()

  public fun dispose()
}

public interface NativeSurfaceRenderer {
  public val supportedBackends: Set<ProducerBackend>

  public fun onSurfaceAvailable(session: NativeSurfaceSession) {}

  public fun onSurfaceChanged(extent: SurfaceExtent) {}

  public fun render(frame: NativeSurfaceFrame): NativeSurfaceRenderResult

  public fun onSurfaceLost() {}
}

public interface NativeSurfaceSession {
  public val backend: ProducerBackend

  public val capabilities: NativeSurfaceCapabilities

  public fun requestFrame()
}

public interface NativeSurfaceFrame {
  public val frameId: Long

  public val extent: SurfaceExtent

  public val target: NativeSurfaceTarget

  public val presentationTimeNanos: Long?
}

public sealed interface NativeSurfaceRenderResult {
  public data object Rendered : NativeSurfaceRenderResult

  public data object Skipped : NativeSurfaceRenderResult
}

public sealed interface NativeSurfaceState {
  public data object Inactive : NativeSurfaceState

  public data class Ready(
    public val backend: ProducerBackend,
    public val capabilities: NativeSurfaceCapabilities,
  ) : NativeSurfaceState

  public data class Unsupported(
    public val requestedBackends: Set<ProducerBackend>,
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
  public val isPlaceholder: Boolean,
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

public data class SurfaceExtent(public val width: Int, public val height: Int) {
  public val isEmpty: Boolean
    get() = width <= 0 || height <= 0

  public companion object {
    public val Empty: SurfaceExtent = SurfaceExtent(0, 0)
  }
}

@JvmInline public value class NativeHandle(public val address: Long)

public sealed interface NativeSurfaceTarget {
  public val backend: ProducerBackend

  public val extent: SurfaceExtent

  public val generation: Long
}

public data class MetalTextureTarget(
  public val texture: NativeHandle,
  public val pixelFormat: Long,
  override val extent: SurfaceExtent,
  override val generation: Long,
) : NativeSurfaceTarget {
  override val backend: ProducerBackend = ProducerBackend.METAL
}

public data class VulkanImageTarget(
  public val image: NativeHandle,
  public val imageView: NativeHandle,
  public val format: Int,
  public val initialLayout: Int,
  public val finalLayout: Int,
  public val queueFamilyIndex: Int,
  override val extent: SurfaceExtent,
  override val generation: Long,
) : NativeSurfaceTarget {
  override val backend: ProducerBackend = ProducerBackend.VULKAN
}

public fun interface OpenGlContextProvider {
  public fun makeCurrent()
}

public data class OpenGlTextureTarget(
  public val textureName: Int,
  public val textureTarget: Int,
  public val format: Int,
  public val contextProvider: OpenGlContextProvider,
  override val extent: SurfaceExtent,
  override val generation: Long,
) : NativeSurfaceTarget {
  override val backend: ProducerBackend = ProducerBackend.OPENGL
}
