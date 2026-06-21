package org.maplibre.nativeffi.examples.composemap.surface

internal abstract class PlaceholderBridge(
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

internal class LinuxVulkanOpenGlBridge :
  PlaceholderBridge(ProducerBackend.VULKAN, ConsumerBackend.OPENGL) {
  // TODO: Allocate a Vulkan image with Linux external-memory support, export it as dma-buf or an
  // opaque FD, import it into Skiko's OpenGL context with the matching EGL/GL memory extension, and
  // add explicit sync FD ownership for producer-to-consumer and consumer-to-producer handoff.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    VulkanImageTarget(
      context = PlaceholderVulkanContextHandles,
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

internal class LinuxOpenGlBridge :
  PlaceholderBridge(ProducerBackend.OPENGL, ConsumerBackend.OPENGL) {
  // TODO: Create bridge-owned external-memory storage, import it into both MapLibre's EGL context
  // and Skiko's OpenGL context, and add GL semaphore/fence ownership for producer-to-consumer
  // handoff.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    OpenGlTextureTarget(
      context = PlaceholderEglContextHandles,
      textureName = 0,
      textureTarget = 0,
      format = 0,
      contextProvider = OpenGlContextProvider {},
      extent = extent,
      generation = generation,
    )
}

internal class WindowsVulkanD3d12Bridge :
  PlaceholderBridge(ProducerBackend.VULKAN, ConsumerBackend.DIRECT3D12) {
  // TODO: Create a D3D12 shared resource handle, import it into Vulkan with
  // VK_KHR_external_memory_win32, pair it with external semaphore/fence handles, and expose the
  // consumer-side D3D12 texture to Skiko's Direct3D renderer without forcing Skiko onto OpenGL.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    VulkanImageTarget(
      context = PlaceholderVulkanContextHandles,
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

internal class WindowsOpenGlD3d12Bridge :
  PlaceholderBridge(ProducerBackend.OPENGL, ConsumerBackend.DIRECT3D12) {
  // TODO: Define the WGL-to-D3D12 sharing path, including keyed mutex or fence synchronization,
  // lifetime for HANDLE ownership, and fail-fast diagnostics when the required WGL/D3D interop
  // extensions are unavailable.
  override fun target(extent: SurfaceExtent, generation: Long): NativeSurfaceTarget =
    OpenGlTextureTarget(
      context = PlaceholderWglContextHandles,
      textureName = 0,
      textureTarget = 0,
      format = 0,
      contextProvider = OpenGlContextProvider {},
      extent = extent,
      generation = generation,
    )
}

private val PlaceholderVulkanContextHandles =
  VulkanContextHandles(
    instance = NativeHandle(0),
    physicalDevice = NativeHandle(0),
    device = NativeHandle(0),
    graphicsQueue = NativeHandle(0),
    graphicsQueueFamilyIndex = 0,
    getInstanceProcAddr = NativeHandle(0),
    getDeviceProcAddr = NativeHandle(0),
  )

private val PlaceholderEglContextHandles =
  EglContextHandles(
    display = NativeHandle(0),
    config = NativeHandle(0),
    shareContext = NativeHandle(0),
    getProcAddress = NativeHandle(0),
  )

private val PlaceholderWglContextHandles =
  WglContextHandles(
    deviceContext = NativeHandle(0),
    shareContext = NativeHandle(0),
    getProcAddress = NativeHandle(0),
  )
