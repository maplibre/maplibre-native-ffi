import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
  import QuartzCore
#endif

#if canImport(Metal)
  /// Runs one caller-graphics-thread session's driver loop on a dedicated
  /// thread, the way a host's render thread does.
  ///
  /// The session binds to the first thread that services it, so every driver
  /// call has to come from one thread; running the loop off the test's own
  /// task also leaves the test free to await the session's commands, which
  /// only make progress while the loop runs.
  private final class DriverLoop: @unchecked Sendable {
    private let stopping = LockedBox(false)
    private let stopped = DispatchSemaphore(value: 0)

    init(_ session: RenderSessionHandle) {
      let stopping = stopping
      let stopped = stopped
      let thread = Thread {
        while !stopping.value {
          _ = try? session.serviceDriverWork(maxWork: 0)
          usleep(500)
        }
        stopped.signal()
      }
      thread.name = "test render driver"
      thread.start()
    }

    func stop() {
      stopping.update { $0 = true }
      _ = stopped.wait(timeout: .now() + .seconds(10))
    }
  }

  private func metalObjectPointer(_ object: AnyObject) -> NativePointer {
    NativePointer(bitPattern: UInt(bitPattern:
      Unmanaged.passUnretained(object).toOpaque()))
  }

  private func attachMetalTexture(
    map: MapHandle,
    device: MTLDevice
  ) throws -> RenderSessionAttachment {
    try map.attachMetalOwnedTexture(
      MetalOwnedTextureDescriptor(
        extent: RenderTargetExtent(width: 32, height: 32, scaleFactor: 1),
        context: MetalContextDescriptor(device: metalObjectPointer(device))
      ),
      options: RenderSessionAttachOptions(driver: .callerGraphicsThread)
    )
  }

  /// Builds a caller-owned texture the session renders into, cleared so a
  /// readback starts from a known value.
  private func makeBorrowedTexture(
    device: MTLDevice,
    width: Int,
    height: Int
  ) throws -> MTLTexture {
    let descriptor = MTLTextureDescriptor.texture2DDescriptor(
      pixelFormat: .rgba8Unorm,
      width: width,
      height: height,
      mipmapped: false
    )
    descriptor.usage = [.renderTarget, .shaderRead]
    let texture = try #require(device.makeTexture(descriptor: descriptor))
    texture.replace(
      region: MTLRegionMake2D(0, 0, width, height),
      mipmapLevel: 0,
      withBytes: [UInt8](repeating: 0, count: width * height * 4),
      bytesPerRow: width * 4
    )
    return texture
  }

  private func borrowedTextureDescriptor(
    _ texture: MTLTexture
  ) -> MetalBorrowedTextureDescriptor {
    MetalBorrowedTextureDescriptor(
      extent: RenderTargetExtent(
        width: UInt32(texture.width),
        height: UInt32(texture.height),
        scaleFactor: 1
      ),
      physicalWidth: UInt32(texture.width),
      physicalHeight: UInt32(texture.height),
      texture: metalObjectPointer(texture)
    )
  }

  /// Reports whether every pixel of `texture` carries the background color
  /// that `redBackgroundStyleJSON` paints.
  ///
  /// A texture created without an explicit storage mode is managed on macOS,
  /// so its CPU copy stays stale until a blit synchronizes it.
  private func isPaintedRed(
    _ texture: MTLTexture,
    device: MTLDevice
  ) throws -> Bool {
    let queue = try #require(device.makeCommandQueue())
    let buffer = try #require(queue.makeCommandBuffer())
    let encoder = try #require(buffer.makeBlitCommandEncoder())
    encoder.synchronize(resource: texture)
    encoder.endEncoding()
    buffer.commit()
    buffer.waitUntilCompleted()

    let bytesPerRow = texture.width * 4
    var pixels = [UInt8](repeating: 0, count: bytesPerRow * texture.height)
    try pixels.withUnsafeMutableBytes { bytes in
      try texture.getBytes(
        #require(bytes.baseAddress),
        bytesPerRow: bytesPerRow,
        from: MTLRegionMake2D(0, 0, texture.width, texture.height),
        mipmapLevel: 0
      )
    }
    return stride(from: 0, to: pixels.count, by: 4).allSatisfy {
      Array(pixels[$0 ..< $0 + 4]) == [255, 0, 0, 255]
    }
  }

  /// Requests one frame and returns the terminal result for its own token.
  private func renderFrame(
    _ session: RenderSessionHandle,
    token: UInt64
  ) async throws -> RenderFrameResult? {
    try session.requestFrame(FrameDemand(options: [], token: token))
    var results: [RenderFrameResult] = []
    guard try await waitUntilTrue("frame \(token)", timeout: 20, condition: {
      results += try session.drainFrameResults()
      return results.contains { $0.token == token }
    }) else { return nil }
    return results.first { $0.token == token }
  }

  /// Renders one frame at a time until `condition` holds, the way a host's
  /// render loop does. Records an issue naming `subject` at the deadline.
  private func renderUntil(
    _ subject: String,
    session: RenderSessionHandle,
    timeout: TimeInterval = 20,
    condition: () throws -> Bool
  ) async throws -> Bool {
    let deadline = Date().addingTimeInterval(timeout)
    var token: UInt64 = 0
    while Date() < deadline {
      token += 1
      guard try await renderFrame(session, token: token) != nil else {
        return false
      }
      if try condition() { return true }
    }
    Issue.record("timed out waiting for \(subject)")
    return false
  }

  /// Runs `body` and asserts that it reports `kind`.
  private func expectFailure(
    _ subject: String,
    kind: MaplibreErrorKind,
    _ body: () async throws -> Void
  ) async {
    do {
      try await body()
      Issue.record("\(subject) should have been rejected")
    } catch let error as MaplibreError {
      #expect(error.kind == kind)
    } catch {
      Issue.record("\(subject) reported \(error)")
    }
  }

  /// A style that paints every pixel, so a readback proves which target the
  /// session rendered into.
  private let redBackgroundStyleJSON = Data(##"""
  {"version":8,"sources":{},"layers":[{"id":"background",
  "type":"background","paint":{"background-color":"#ff0000"}}]}
  """##.utf8)

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
    let attachment = try attachMetalTexture(map: map, device: device)
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

/// A frame result kind a newer native library adds reaches the host with its
/// raw value instead of collapsing onto a named one.
@Test func anUnnamedRenderResultKeepsItsRawValue() {
  #expect(RenderResult.fromNative(99) == .unknown(99))
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

#if canImport(Metal)
  /// BND-167, BND-168, BND-169. An owned texture session leases one rendered
  /// frame at a time: an empty ring polls without an error, a leased frame
  /// reads its texture and its result, releasing returns the slot exactly
  /// once, and a released frame rejects every read.
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func acquiredFrameLeasesAndReturnsOneTextureRingSlot() async throws {
    let runtime = try RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    defer { try? runtime.closeBlockingForTests() }
    let map = try await MapHandle(
      runtime: runtime,
      options: MapOptions(width: 32, height: 32)
    )
    defer { try? map.closeBlockingForTests() }
    try await map.setStyleJSON(emptyStyleJSON)

    let device = try #require(MTLCreateSystemDefaultDevice())
    let attachment = try attachMetalTexture(map: map, device: device)
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
      withExtendedLifetime(device) {}
    }
    let loop = DriverLoop(session)
    defer { loop.stop() }
    try await attachment.completion.value

    #expect(try session.capabilities().features.contains(.frameAcquisition))
    // An empty ring polls without reporting an error.
    #expect(try session.drainFrameResults().isEmpty)
    #expect(try session.acquireFrame() == nil)

    try session.requestFrame(FrameDemand(options: [], token: 7))
    var results: [RenderFrameResult] = []
    #expect(try await waitUntilTrue("a rendered frame", timeout: 20) {
      results += try session.drainFrameResults()
      return !results.isEmpty
    })
    let rendered = try #require(results.first { $0.token == 7 })
    #expect(rendered.result == .rendered)

    let frame = try #require(try session.acquireFrame())
    // The handle's lock is released before the host closure runs, so the
    // closure may call back into the same handle.
    var readInsideTheClosure: RenderFrameResult?
    let texture = try frame
      .withMetalTexture { texture -> MetalOwnedTextureFrame in
        readInsideTheClosure = try frame.result()
        return texture
      }
    #expect(texture.width == 32)
    #expect(texture.height == 32)
    #expect(!texture.texture.isNull)
    #expect(readInsideTheClosure?.frameGeneration == rendered.frameGeneration)

    #expect(!frame.isReleased)
    try frame.release()
    #expect(frame.isReleased)
    // Releasing an already released frame changes nothing.
    try frame.release()
    #expect(frame.isReleased)

    for read in [
      { _ = try frame.result() },
      { try frame.withMetalTexture { _ in } },
      { _ = try frame.producerSynchronization() },
    ] as [() throws -> Void] {
      do {
        try read()
        Issue.record("a released frame should reject a read")
      } catch let error as MaplibreError {
        #expect(error.kind == .invalidState)
      }
    }
  }

  /// BND-048. A frame handle that goes out of scope without being released
  /// keeps its ring slot, and the leak reporter names it.
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func droppingAnUnreleasedFrameIsReportedAsALeak() async throws {
    let runtime = try RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    defer { try? runtime.closeBlockingForTests() }
    let map = try await MapHandle(
      runtime: runtime,
      options: MapOptions(width: 32, height: 32)
    )
    defer { try? map.closeBlockingForTests() }
    try await map.setStyleJSON(emptyStyleJSON)

    let device = try #require(MTLCreateSystemDefaultDevice())
    let attachment = try attachMetalTexture(map: map, device: device)
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
      withExtendedLifetime(device) {}
    }
    let loop = DriverLoop(session)
    defer { loop.stop() }
    try await attachment.completion.value

    try session.requestFrame(FrameDemand(options: [], token: 11))
    var results: [RenderFrameResult] = []
    #expect(try await waitUntilTrue("a rendered frame", timeout: 20) {
      results += try session.drainFrameResults()
      return !results.isEmpty
    })

    let leaks = LockedBox([NativeHandleLeak]())
    try NativeHandleLeakTestSupport.withHandler({ leak in
      leaks.update { $0.append(leak) }
    }) {
      // The handle's last reference dies at the end of this scope.
      let frame = try session.acquireFrame()
      #expect(frame != nil)
    }

    let leak = try #require(leaks.read { $0.first })
    #expect(leak.typeName == "AcquiredFrameHandle")
    #expect(leak.handle != 0)
    #expect(leak.detail.contains("release"))
  }

  /// The frame-ready wake schedules the host when results become drainable
  /// and stops once the handler is cleared.
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func frameReadyHandlerRunsUntilItIsCleared() async throws {
    let runtime = try RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    defer { try? runtime.closeBlockingForTests() }
    let map = try await MapHandle(
      runtime: runtime,
      options: MapOptions(width: 32, height: 32)
    )
    defer { try? map.closeBlockingForTests() }
    try await map.setStyleJSON(emptyStyleJSON)

    let device = try #require(MTLCreateSystemDefaultDevice())
    let attachment = try attachMetalTexture(map: map, device: device)
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
      withExtendedLifetime(device) {}
    }
    let loop = DriverLoop(session)
    defer { loop.stop() }
    try await attachment.completion.value

    let wakes = LockedBox(0)
    try session.setFrameReadyHandler { wakes.update { $0 += 1 } }
    try session.requestFrame(FrameDemand(options: [], token: 21))
    var results: [RenderFrameResult] = []
    #expect(try await waitUntilTrue("the frame wake", timeout: 20) {
      results += try session.drainFrameResults()
      return !results.isEmpty && wakes.value > 0
    })
    let afterFirstFrame = wakes.value
    #expect(afterFirstFrame > 0)

    try session.setFrameReadyHandler(nil)
    results = []
    try session.requestFrame(FrameDemand(options: [], token: 22))
    #expect(try await waitUntilTrue("the second frame", timeout: 20) {
      results += try session.drainFrameResults()
      return results.contains { $0.token == 22 }
    })
    #expect(wakes.value == afterFirstFrame)
  }

  /// BND-165, BND-183. A session resize is the one authority on an attached
  /// map's size, and both it and a direct map resize keep the scale factor
  /// they were created with.
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func sessionResizeRejectsAnotherScaleFactorAndResizesTheMap(
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
    try await map.setStyleJSON(emptyStyleJSON)

    let device = try #require(MTLCreateSystemDefaultDevice())
    let attachment = try attachMetalTexture(map: map, device: device)
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
      withExtendedLifetime(device) {}
    }
    let loop = DriverLoop(session)
    defer { loop.stop() }
    try await attachment.completion.value

    await expectFailure("another session scale factor",
                        kind: .invalidArgument)
    {
      try await session.resize(
        RenderTargetExtent(width: 48, height: 24, scaleFactor: 2)
      )
    }

    try await session.resize(
      RenderTargetExtent(width: 48, height: 24, scaleFactor: 1)
    )
    #expect(try session.snapshot().extent.width == 48)
    #expect(try map.snapshot().logicalExtent.width == 48)
    #expect(try map.snapshot().logicalExtent.height == 24)

    await expectFailure("another map scale factor", kind: .invalidArgument) {
      _ = try await map.resize(
        to: MapLogicalExtent(width: 48, height: 24, scaleFactor: 3)
      )
    }
  }

  /// BND-176. A session renders through the target kind it attached with, so a
  /// replacement of another kind is rejected and leaves the session usable.
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func setTargetReportsUnsupportedForAnotherTargetKind() async throws {
    let runtime = try RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    defer { try? runtime.closeBlockingForTests() }
    let map = try await MapHandle(
      runtime: runtime,
      options: MapOptions(width: 32, height: 32)
    )
    defer { try? map.closeBlockingForTests() }
    try await map.setStyleJSON(emptyStyleJSON)

    let device = try #require(MTLCreateSystemDefaultDevice())
    let attachment = try attachMetalTexture(map: map, device: device)
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
      withExtendedLifetime(device) {}
    }
    let loop = DriverLoop(session)
    defer { loop.stop() }
    try await attachment.completion.value

    // This session owns its texture, so it renders into neither a caller-owned
    // texture nor a surface. Both replacements name a live backend object, so
    // each rejection is about the target kind and not a null handle.
    let replacement = try makeBorrowedTexture(
      device: device,
      width: 32,
      height: 32
    )
    await expectFailure("a caller-owned texture", kind: .unsupported) {
      try await session
        .setMetalBorrowedTextureTarget(borrowedTextureDescriptor(replacement))
    }

    let layer = CAMetalLayer()
    layer.device = device
    layer.pixelFormat = .bgra8Unorm
    layer.framebufferOnly = false
    layer.drawableSize = CGSize(width: 32, height: 32)
    await expectFailure("a surface", kind: .unsupported) {
      try await session.setMetalSurfaceTarget(MetalSurfaceDescriptor(
        extent: RenderTargetExtent(width: 32, height: 32, scaleFactor: 1),
        context: MetalContextDescriptor(device: metalObjectPointer(device)),
        layer: metalObjectPointer(layer)
      ))
    }

    // The rejections left the session usable.
    #expect(try await renderFrame(session, token: 5)?.result == .rendered)
    withExtendedLifetime(replacement) {}
    withExtendedLifetime(layer) {}
  }

  /// BND-175, BND-183. A caller-owned texture is sized by its owner, so a host
  /// that follows a resize hands over a texture at the new size instead of
  /// resizing the session: the session rejects the resize, paints whichever
  /// texture it was handed, and the map takes the new extent from its own
  /// resize.
  @Test(.enabled(
    if: Maplibre.supportedRenderBackends().contains(.metal),
    "The selected native preset does not provide Metal."
  )) func setTargetRendersIntoTheReplacementCallerOwnedTexture() async throws {
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
    let texture = try makeBorrowedTexture(device: device, width: 32, height: 32)
    let attachment = try map.attachMetalBorrowedTexture(
      borrowedTextureDescriptor(texture),
      options: RenderSessionAttachOptions(driver: .callerGraphicsThread)
    )
    let session = attachment.session
    defer {
      _ = try? session.abandon()
      try? session.close()
      withExtendedLifetime(device) {}
      withExtendedLifetime(texture) {}
    }
    let loop = DriverLoop(session)
    defer { loop.stop() }
    try await attachment.completion.value

    try await map.setStyleJSON(redBackgroundStyleJSON)
    #expect(try await renderUntil("the attached texture", session: session) {
      try isPaintedRed(texture, device: device)
    })

    await expectFailure("resizing a caller-owned texture", kind: .unsupported) {
      try await session.resize(
        RenderTargetExtent(width: 48, height: 24, scaleFactor: 1)
      )
    }

    let replacement = try makeBorrowedTexture(
      device: device,
      width: 48,
      height: 24
    )
    #expect(try !isPaintedRed(replacement, device: device))
    try await session
      .setMetalBorrowedTextureTarget(borrowedTextureDescriptor(replacement))
    // A retarget replaces the graphics resource only, so the map takes the
    // replacement extent from its own resize.
    _ = try await map.resize(
      to: MapLogicalExtent(width: 48, height: 24, scaleFactor: 1)
    )
    #expect(try await renderUntil("the replacement texture", session: session) {
      try isPaintedRed(replacement, device: device)
    })
    #expect(try map.snapshot().logicalExtent.width == 48)
    #expect(try map.snapshot().logicalExtent.height == 24)
    withExtendedLifetime(replacement) {}
  }
#endif
