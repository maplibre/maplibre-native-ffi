import CMaplibreNativeC
@testable import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
#endif

#if canImport(Metal)
  @Test func metalCallerDriverCanServiceAttachmentBeforeCompletion(
  ) async throws {
    let runtime = try RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    defer { try? runtime.closeBlockingForTests() }
    let map = try await MapHandle(
      runtime: runtime,
      options: MapOptions(width: 32, height: 32)
    )
    defer { try? map.close() }
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
  #expect(GPUSynchronization.vulkanTimelineSemaphore(
    NativePointer(bitPattern: 0x5678), value: 11
  ).native.kind == MLN_GPU_SYNC_VULKAN_TIMELINE_SEMAPHORE.rawValue)
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
