import CMaplibreNativeC
@testable import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
#endif

#if canImport(Metal)
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func metalCallerDriverCanServiceAttachmentBeforeCompletion(
  ) async throws {
    let runtime = try RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    defer { try? runtime.closeBlockingForTests() }
    let map = try await MapHandle(
      runtime: runtime,
      options: MapOptions(width: 32, height: 32)
    )
    defer { try? map.closeBlockingForTests() }
    let device = try #require(MTLCreateSystemDefaultDevice())
    let devicePointer = NativePointer(bitPattern: UInt(bitPattern:
      Unmanaged.passUnretained(device as AnyObject).toOpaque()))

    let attachment = try map.attachMetalOwnedTexture(
      MetalOwnedTextureDescriptor(
        extent: RenderTargetExtent(width: 32, height: 32, scaleFactor: 1),
        context: MetalContextDescriptor(device: devicePointer)
      ),
      options: RenderSessionAttachOptions(driver: .callerGraphicsThread)
    )
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
    }

    #expect(try session.snapshot().state == RenderSessionState.attaching)
    #expect(try session.serviceDriverWork(maxWork: 0) > 0)
    try await attachment.completion.value
    #expect(try session.snapshot().state == RenderSessionState.attached)
    withExtendedLifetime(device) {}
  }
#endif

@Test func frameDemandPreservesCoalescingAndTimeout() {
  let demand = FrameDemand(
    options: [.ifNeeded, .present],
    token: 41,
    coalescingBoundary: 7,
    timeoutNanoseconds: 2000
  ).native

  #expect(demand.flags == 3)
  #expect(demand.token == 41)
  #expect(demand.coalescing_boundary == 7)
  #expect(demand.timeout_ns == 2000)
}

@Test func renderResultsPreserveEveryTerminalDisposition() {
  #expect(RenderResult.fromNative(0) == .rendered)
  #expect(RenderResult.fromNative(1) == .noUpdate)
  #expect(RenderResult.fromNative(2) == .sizePending)
  #expect(RenderResult.fromNative(3) == .targetNotReady)
  #expect(RenderResult.fromNative(4) == .superseded)
  #expect(RenderResult.fromNative(5) == .deadlineMissed)
  #expect(RenderResult.fromNative(99) == .unknown(99))
}

@Test func renderLifecycleStatesPreserveTargetLossAndAbandonment() {
  #expect(RenderSessionState(rawValue: 1) == .attaching)
  #expect(RenderSessionState(rawValue: 2) == .attached)
  #expect(RenderSessionState(rawValue: 3) == .detaching)
  #expect(RenderSessionState(rawValue: 4) == .detached)
  #expect(RenderSessionState(rawValue: 5) == .targetLost)
  #expect(RenderSessionState(rawValue: 6) == .abandoned)
  #expect(RenderAbandonResult.Disposition(rawValue: 1) == .quarantined)
}

@Test func gpuSynchronizationUsesFrozenKinds() {
  #expect(GPUSynchronization.cpuComplete.native.kind ==
    MLN_GPU_SYNC_CPU_COMPLETE.rawValue)
  #expect(GPUSynchronization.metalSharedEvent(
    NativePointer(bitPattern: 0x1234), value: 9
  ).native.kind == MLN_GPU_SYNC_METAL_SHARED_EVENT.rawValue)
  let semaphore = GPUSynchronization.vulkanTimelineSemaphore(
    VulkanHandle(bitPattern: 0xFEED_FACE_0000_0007), value: 11
  ).native
  #expect(semaphore.kind == MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE.rawValue)
  // A Vulkan handle stays 64 bits wide even where a pointer is not.
  #expect(semaphore.object == 0xFEED_FACE_0000_0007)
}

@Test func transferredWebGLCanvasMaterializesWorkerDescriptor() throws {
  let input = OpenGLContextDescriptor.webGL(
    .transferredCanvas(selector: "#map")
  ).nativeInput

  try input.withNative { descriptor in
    #expect(descriptor.platform == MLN_OPENGL_CONTEXT_PLATFORM_WEBGL)
    #expect(descriptor.data.webgl.kind ==
      MLN_WEBGL_CONTEXT_TRANSFERRED_CANVAS.rawValue)
    #expect(descriptor.data.webgl.context == 0)
    #expect(descriptor.data.webgl.canvas_selector.size == 4)
  }
}

@Test func attachmentOptionsCarryDirectWakes() {
  let options = RenderSessionAttachOptions(
    driver: .coreWorker,
    requestedTextureRingDepth: 3
  )
  var frameWake = mln_wake()
  frameWake.user_data = UnsafeMutableRawPointer(bitPattern: 17)
  var driverWake = mln_wake()
  driverWake.user_data = UnsafeMutableRawPointer(bitPattern: 23)
  options
    .withNative(frameWake: frameWake, driverWorkWake: driverWake) { native in
      #expect(native.pointee.driver == MLN_RENDER_DRIVER_CORE_WORKER.rawValue)
      #expect(native.pointee.requested_texture_ring_depth == 3)
      #expect(native.pointee.frame_wake.user_data == frameWake.user_data)
      #expect(native.pointee.driver_work_wake.user_data == driverWake.user_data)
    }
}

/// A Vulkan non-dispatchable handle is 64 bits wide on every platform, so a
/// carrier sized like a pointer would drop the high half on a 32-bit host.
/// Both halves of a handle that lives entirely above bit 32 have to survive
/// the trip into the native descriptor.
@Test func vulkanDescriptorsCarryFullWidthHandles() throws {
  let context = VulkanContextDescriptor(
    instance: NativePointer(bitPattern: 0x30),
    physicalDevice: NativePointer(bitPattern: 0x40),
    device: NativePointer(bitPattern: 0x50),
    graphicsQueue: NativePointer(bitPattern: 0x60),
    graphicsQueueFamilyIndex: 7,
    getInstanceProcAddr: NativePointer(bitPattern: 0x90),
    getDeviceProcAddr: NativePointer(bitPattern: 0xA0)
  )
  let extent = RenderTargetExtent(width: 64, height: 32, scaleFactor: 2)

  let texture = VulkanBorrowedTextureDescriptor(
    extent: extent,
    physicalWidth: 128,
    physicalHeight: 64,
    context: context,
    image: VulkanHandle(bitPattern: 0x8000_0000_0000_0001),
    imageView: VulkanHandle(bitPattern: 0x0000_0001_0000_0000),
    format: 44,
    initialLayout: 1,
    finalLayout: 2
  )
  try texture.nativeInput.withNativeDescriptor { descriptor in
    #expect(descriptor.pointee.image == 0x8000_0000_0000_0001)
    #expect(descriptor.pointee.image_view == 0x0000_0001_0000_0000)
    #expect(descriptor.pointee.physical_width == 128)
    #expect(descriptor.pointee.format == 44)
  }

  let surface = VulkanSurfaceDescriptor(
    extent: extent,
    context: context,
    surface: VulkanHandle(bitPattern: 0xFEDC_BA98_7654_3210)
  )
  try surface.nativeInput.withNativeDescriptor { descriptor in
    #expect(descriptor.pointee.surface == 0xFEDC_BA98_7654_3210)
    #expect(UInt(bitPattern: descriptor.pointee.context.device) == 0x50)
  }
}
