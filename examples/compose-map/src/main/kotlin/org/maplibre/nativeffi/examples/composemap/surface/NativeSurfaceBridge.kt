package org.maplibre.nativeffi.examples.composemap.surface

import androidx.compose.ui.graphics.drawscope.DrawScope

internal interface NativeSurfaceBridge : AutoCloseable {
  val backend: ProducerBackend

  val consumerBackend: ConsumerBackend

  val capabilities: NativeSurfaceCapabilities

  fun resize(extent: SurfaceExtent) {}

  fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame

  fun completeProducerAccess(frame: NativeSurfaceFrame) {}

  fun releaseFrame(frame: NativeSurfaceFrame) {}

  fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean = false

  override fun close() {}

  companion object {
    val host: NativeSurfaceHost = detectHost()

    fun select(supportedBackends: Set<ProducerBackend>): NativeSurfaceBridge? =
      bridgeCandidates(host).firstOrNull { it.backend in supportedBackends }

    private fun bridgeCandidates(host: NativeSurfaceHost): List<NativeSurfaceBridge> =
      when (host.operatingSystem) {
        NativeSurfaceOperatingSystem.MACOS ->
          listOf(MacMetalBridge(), MacVulkanMetalBridge(), MacOpenGlMetalBridge())
        NativeSurfaceOperatingSystem.LINUX -> listOf(LinuxVulkanOpenGlBridge(), LinuxOpenGlBridge())
        NativeSurfaceOperatingSystem.WINDOWS ->
          listOf(WindowsVulkanD3d12Bridge(), WindowsOpenGlD3d12Bridge())
        NativeSurfaceOperatingSystem.UNSUPPORTED -> emptyList()
      }
  }
}

internal data class NativeSurfaceFrameLease(
  override val frameId: Long,
  override val extent: SurfaceExtent,
  override val target: NativeSurfaceTarget,
  override val presentationTimeNanos: Long?,
) : NativeSurfaceFrame

private abstract class PlaceholderBridge(
  override val backend: ProducerBackend,
  override val consumerBackend: ConsumerBackend,
) : NativeSurfaceBridge {
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
      isPlaceholder = true,
    )

  override fun resize(extent: SurfaceExtent) {
    if (extent != currentExtent) {
      currentExtent = extent
      generation += 1
    }
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame =
    NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(extent, generation),
      presentationTimeNanos = presentationTimeNanos,
    )

  protected abstract fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget
}

private class MacMetalBridge : PlaceholderBridge(ProducerBackend.METAL, ConsumerBackend.METAL) {
  private var texture = NativeHandle(0)
  private var pixelFormat = 0L
  private var generation = 0L
  private var currentExtent = SurfaceExtent.Empty

  override val capabilities: NativeSurfaceCapabilities =
    NativeSurfaceCapabilities(
      producerBackend = backend,
      consumerBackend = consumerBackend,
      supportsExplicitSynchronization = false,
      supportsResizeWithoutRecreate = false,
      isPlaceholder = false,
    )

  override fun resize(extent: SurfaceExtent) {
    if (extent == currentExtent && texture.address != 0L) {
      return
    }
    recreateTexture(extent)
    currentExtent = extent
    generation += 1
  }

  override fun acquireFrame(
    frameId: Long,
    extent: SurfaceExtent,
    presentationTimeNanos: Long?,
  ): NativeSurfaceFrame {
    if (texture.address == 0L || extent != currentExtent) {
      resize(extent)
    }
    return NativeSurfaceFrameLease(
      frameId = frameId,
      extent = extent,
      target = target(extent, generation),
      presentationTimeNanos = presentationTimeNanos,
    )
  }

  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    MetalTextureTarget(
      texture =
        texture.takeIf { it.address != 0L }
          ?: throw NativeSurfaceBridgeException("Skiko Metal texture allocation returned null"),
      pixelFormat = pixelFormat,
      extent = extent,
      generation = generation,
    )

  override fun draw(scope: DrawScope, target: NativeSurfaceTarget): Boolean {
    if (target !is MetalTextureTarget || target.texture.address == 0L) {
      return false
    }
    return SkikoHost.drawMetalTexture(scope, target)
  }

  override fun close() {
    disposeTexture()
  }

  private fun recreateTexture(extent: SurfaceExtent) {
    if (extent.isEmpty) {
      disposeTexture()
      return
    }
    val metalDevice = SkikoHost.requireMetalDevice()
    val textureAddress =
      MacMetalBridgeNative.createMetalTexture(
        metalDevice = metalDevice.ptr,
        oldTexture = texture.address,
        width = extent.physicalWidth,
        height = extent.physicalHeight,
      )
    texture = NativeHandle(textureAddress)
    pixelFormat = MacMetalBridgeNative.texturePixelFormat(textureAddress)
  }

  private fun disposeTexture() {
    if (texture.address != 0L) {
      SkikoHost.forgetMetalTexture(texture)
      MacMetalBridgeNative.disposeMetalTexture(texture.address)
      texture = NativeHandle(0)
      pixelFormat = 0
    }
  }
}

private class MacVulkanMetalBridge :
  PlaceholderBridge(ProducerBackend.VULKAN, ConsumerBackend.METAL) {
  // TODO(surface): use SkikoHost.requireMetalDevice(), allocate a Metal texture first, import it
  // into Vulkan with VK_EXT_external_memory_metal, and expose the MapLibre Vulkan context handles
  // needed by MapLibreSurfaceRenderer.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    VulkanImageTarget(
      image = NativeHandle(0),
      imageView = NativeHandle(0),
      format = 0,
      initialLayout = 0,
      finalLayout = 0,
      queueFamilyIndex = 0,
      extent = extent,
      generation = generation,
    )
}

private class MacOpenGlMetalBridge :
  PlaceholderBridge(ProducerBackend.OPENGL, ConsumerBackend.METAL) {
  // TODO(surface): build this from IOSurface-backed storage after the same-API Metal path proves
  // the SkikoHost reflection and draw path; define the producer CGL/EGL context contract before
  // exposing an OpenGlTextureTarget to MapLibre.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    OpenGlTextureTarget(
      textureName = 0,
      textureTarget = 0,
      format = 0,
      contextProvider = OpenGlContextProvider {},
      extent = extent,
      generation = generation,
    )
}

private class LinuxVulkanOpenGlBridge :
  PlaceholderBridge(ProducerBackend.VULKAN, ConsumerBackend.OPENGL) {
  // TODO: Allocate a Vulkan image with Linux external-memory support, export it as dma-buf or an
  // opaque FD, import it into Skiko's OpenGL context with the matching EGL/GL memory extension, and
  // add explicit sync FD ownership for producer-to-consumer and consumer-to-producer handoff.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    VulkanImageTarget(
      image = NativeHandle(0),
      imageView = NativeHandle(0),
      format = 0,
      initialLayout = 0,
      finalLayout = 0,
      queueFamilyIndex = 0,
      extent = extent,
      generation = generation,
    )
}

private class LinuxOpenGlBridge :
  PlaceholderBridge(ProducerBackend.OPENGL, ConsumerBackend.OPENGL) {
  // TODO: Bind the target to the active Skiko OpenGL context, define context-current guarantees for
  // producer callbacks, and add GL fence ownership so Skiko never samples while the producer
  // writes.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    OpenGlTextureTarget(
      textureName = 0,
      textureTarget = 0,
      format = 0,
      contextProvider = OpenGlContextProvider {},
      extent = extent,
      generation = generation,
    )
}

private class WindowsVulkanD3d12Bridge :
  PlaceholderBridge(ProducerBackend.VULKAN, ConsumerBackend.DIRECT3D12) {
  // TODO: Create a D3D12 shared resource handle, import it into Vulkan with
  // VK_KHR_external_memory_win32, pair it with external semaphore/fence handles, and expose the
  // consumer-side D3D12 texture to Skiko's Direct3D renderer without forcing Skiko onto OpenGL.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    VulkanImageTarget(
      image = NativeHandle(0),
      imageView = NativeHandle(0),
      format = 0,
      initialLayout = 0,
      finalLayout = 0,
      queueFamilyIndex = 0,
      extent = extent,
      generation = generation,
    )
}

private class WindowsOpenGlD3d12Bridge :
  PlaceholderBridge(ProducerBackend.OPENGL, ConsumerBackend.DIRECT3D12) {
  // TODO: Define the WGL-to-D3D12 sharing path, including keyed mutex or fence synchronization,
  // lifetime for HANDLE ownership, and fallback behavior when the required WGL/D3D interop
  // extensions are unavailable.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    OpenGlTextureTarget(
      textureName = 0,
      textureTarget = 0,
      format = 0,
      contextProvider = OpenGlContextProvider {},
      extent = extent,
      generation = generation,
    )
}

private fun detectHost(): NativeSurfaceHost {
  val os = System.getProperty("os.name").lowercase()
  return when {
    os.contains("mac") ->
      NativeSurfaceHost(NativeSurfaceOperatingSystem.MACOS, ConsumerBackend.METAL)
    os.contains("linux") ->
      NativeSurfaceHost(NativeSurfaceOperatingSystem.LINUX, ConsumerBackend.OPENGL)
    os.contains("windows") ->
      NativeSurfaceHost(NativeSurfaceOperatingSystem.WINDOWS, ConsumerBackend.DIRECT3D12)
    else -> NativeSurfaceHost(NativeSurfaceOperatingSystem.UNSUPPORTED, ConsumerBackend.OPENGL)
  }
}
