import CMaplibreNativeC
@testable import MaplibreNativeFFI
import Testing

@Test func frameDemandPreservesCoalescingAndDeadline() {
  let demand = FrameDemand(
    options: [.ifNeeded, .present],
    token: 41,
    coalescingBoundary: 7,
    deadlineNanoseconds: 2000
  ).native

  #expect(demand.flags == 3)
  #expect(demand.token == 41)
  #expect(demand.coalescing_boundary == 7)
  #expect(demand.deadline_ns == 2000)
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

@Test func attachmentOptionsShareTheRuntimeNotificationReceiver() {
  let options = RenderSessionAttachOptions(
    driver: .coreWorker,
    requestedTextureRingDepth: 3
  )
  options.withNative(notificationSource: 17) { native in
    #expect(native.pointee.driver == MLN_RENDER_DRIVER_CORE_WORKER.rawValue)
    #expect(native.pointee.requested_texture_ring_depth == 3)
    #expect(native.pointee.operation_source == 17)
    #expect(native.pointee.frame_source == 0)
    #expect(native.pointee.driver_work_source == 0)
  }
}
