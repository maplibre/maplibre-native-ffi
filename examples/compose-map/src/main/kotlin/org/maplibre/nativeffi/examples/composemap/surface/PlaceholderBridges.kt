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

internal class WindowsOpenGlD3d12Bridge :
  PlaceholderBridge(ProducerBackend.OPENGL, ConsumerBackend.DIRECT3D12) {
  // TODO: Build this after the Vulkan-D3D12 bridge unless OpenGL parity becomes the priority:
  // 1. Reuse the Direct3D SkikoHost reflection/presentation helper from WindowsVulkanD3d12Bridge.
  //    The consumer remains Skiko Direct3D, matching the Vulkan-D3D12 bridge direction.
  // 2. Create a WGL share context for MapLibre using a real HWND/HDC pixel format, as the existing
  //    C API expects WglContextDescriptor(deviceContext, shareContext, getProcAddress). Follow the
  //    src/zig_test_support/wgl_context.zig bootstrap pattern for choosing the pixel format and
  //    creating a shareable context.
  // 3. Allocate consumer-compatible D3D12 shared storage, then expose it to the producer OpenGL
  //    context. Preferred path: GL_EXT_memory_object + GL_EXT_memory_object_win32 with a D3D12
  //    resource handle. Fallback candidates are WGL_NV_DX_interop2 or ANGLE-backed D3D11/12 if the
  //    external-memory path is unavailable on target Windows drivers.
  // 4. Pass only OpenGlTextureTarget plus WglContextHandles to MapLibre. Put Skiko/D3D12 handles,
  //    GL memory objects, HANDLE duplication/close rules, and synchronization inside the bridge.
  // 5. Carry TextureOrigin through the target, as Linux showed that producer/consumer origin varies
  //    by API pair. Verify visually before adding gesture or resize validation.
  // 6. Route all MapLibre-owned calls through a renderer owner thread if the WGL context is made
  //    current outside the EDT. The Linux EGL bridge's withRendererAccess pattern is the template.
  // 7. Start with glFinish plus a D3D12 fence wait for proof-level synchronization, then move to
  //    GL_EXT_semaphore_win32 or keyed mutex / shared-fence synchronization once texture import is
  //    stable.
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
