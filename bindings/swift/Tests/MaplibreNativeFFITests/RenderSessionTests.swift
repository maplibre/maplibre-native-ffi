import Foundation
import GraphicsSupport
@testable import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
#endif

/// A render fixture: a session attached on the map of a map fixture. A
/// borrowed-texture session also holds a context and textures from
/// tests/graphics for the build's backend; an offscreen one holds neither.
private struct AttachedSession {
  let fixture: MapFixture
  let graphics: TestGraphics?
  let textures: [TestTexture]
  let session: RenderSessionHandle
  /// The host thread that drives a caller-driven session.
  let thread: RenderThread?

  /// Runs `body`, then tears the fixture down, also when `body` throws. The
  /// session retires before its textures and graphics go: a session still
  /// attached when they go away renders into destroyed objects, which crashes
  /// the process before the failure that skipped the teardown is reported.
  func use<Result>(
    _ body: (AttachedSession) async throws -> Result
  ) async throws -> Result {
    let result: Result
    do {
      result = try await body(self)
    } catch {
      await tearDown()
      throw error
    }
    await tearDown()
    return result
  }

  /// Detaches and disposes the session, destroys its textures, then releases
  /// the map fixture. A session that cannot detach, such as one already
  /// abandoned or never attached, is abandoned instead, and one the test
  /// released or disposed stays closed.
  func tearDown() async {
    do {
      try await session.detach()
    } catch {
      thread?.stop()
      _ = try? session.abandon()
    }
    await destroyTextures()
    thread?.stop()
    do { try session.dispose() } catch {
      Issue.record("disposing the render session failed: \(error)")
    }
    await fixture.release()
    withExtendedLifetime(graphics) {}
  }

  /// Destroys the textures once the session has retired, on the session's
  /// thread while it runs, since it may hold their OpenGL context current. A
  /// second call does nothing.
  func destroyTextures() async {
    await destroy(textures, on: thread)
  }
}

/// Destroys `textures` on `thread`, which may hold their OpenGL context
/// current, or here once it has stopped.
private func destroy(
  _ textures: [TestTexture],
  on thread: RenderThread?
) async {
  let body: @Sendable () -> Void = { textures.forEach { $0.destroy() } }
  if let thread, (try? await thread.perform(body)) != nil {
    return
  }
  body()
}

/// Attaches an offscreen session to a map showing `style` and runs `body`
/// once the attach completes, tearing the session down afterwards.
private func withOffscreenSession<Result>(
  style: String = emptyStyle,
  _ body: (AttachedSession) async throws -> Result
) async throws -> Result {
  try await attachOffscreenSession(style: style).use(body)
}

/// Attaches an offscreen session to a map showing `style` and returns it once
/// the attach completes. The session creates its own device and drives itself
/// on a core worker.
private func attachOffscreenSession(
  style: String = emptyStyle
) async throws -> AttachedSession {
  let fixture = try await MapFixture.make()
  let session: RenderSessionHandle
  do {
    try await fixture.map.setStyleJson(style)
    session = try fixture.map.attachOffscreen(
      descriptor: OffscreenDescriptor(
        logicalSize: LogicalSize(width: 32, height: 32)
      ),
      options: RenderSessionAttachOptions(
        driver: .coreWorker,
        frameWake: pulsingFrameWake
      )
    )
  } catch {
    await fixture.release()
    throw error
  }
  let attached = AttachedSession(
    fixture: fixture,
    graphics: nil,
    textures: [],
    session: session,
    thread: nil
  )
  do {
    try await session.ready
  } catch {
    await attached.tearDown()
    throw error
  }
  return attached
}

/// Attaches a borrowed-texture session with a ring of `ringDepth` textures to
/// a map showing `style` and runs `body` once the attach completes, tearing
/// the session down afterwards.
private func withBorrowedTextureSession<Result>(
  style: String = emptyStyle,
  ringDepth: Int = 1,
  _ body: (AttachedSession) async throws -> Result
) async throws -> Result {
  let fixture = try await MapFixture.make()
  let attached: AttachedSession
  do {
    try await fixture.map.setStyleJson(style)
    attached = try await attachBorrowedTexture(
      fixture: fixture,
      ringDepth: ringDepth
    )
  } catch {
    await fixture.release()
    throw error
  }
  return try await attached.use { rendered in
    rendered.thread?.service(rendered.session)
    try await rendered.session.ready
    return try await body(rendered)
  }
}

/// Attaches a session to `fixture`'s map that renders into a borrowed ring of
/// `ringDepth` textures, and returns it without waiting for the attach. A
/// Metal or Vulkan session drives itself on a core worker unless
/// `callerDriven` is set. An OpenGL session shares the fixture's context, so
/// it is always caller-driven. A caller-driven session comes with a host
/// render thread, which holds any context current and which the caller
/// starts servicing the session.
private func attachBorrowedTexture(
  fixture: MapFixture,
  ringDepth: Int = 1,
  callerDriven: Bool = false
) async throws -> AttachedSession {
  let graphics = try TestGraphics()
  let textures = try graphics.createTextures(count: ringDepth)
  let thread = callerDriven || graphics.usesCurrentContext
    ? RenderThread()
    : nil
  do {
    try await thread?.perform { try graphics.makeCurrentIfNeeded() }
    let session = try graphics.attachBorrowedTexture(
      map: fixture.map,
      textures: textures,
      options: RenderSessionAttachOptions(
        driver: thread == nil ? .coreWorker : .callerGraphicsThread,
        frameWake: pulsingFrameWake,
        driverWorkWake: thread?.driverWorkWake ?? Wake()
      )
    )
    return AttachedSession(
      fixture: fixture,
      graphics: graphics,
      textures: textures,
      session: session,
      thread: thread
    )
  } catch {
    await destroy(textures, on: thread)
    thread?.stop()
    throw error
  }
}

/// An offscreen session renders a frame, and the binding copies its pixels
/// out of the native readback.
@Test func anOffscreenSessionRendersPixelsTheBindingReadsBack() async throws {
  try await withOffscreenSession(style: redStyle) { rendered in
    #expect(try await rendered.session.awaitRenderedFrame() != nil)

    let readback = try await rendered.session.readTexture()
    #expect(readback.info.width == 32)
    #expect(readback.info.height == 32)
    let pixels = [UInt8](readback.data)
    #expect(pixels.count >= 32 * 32 * 4)
    #expect(Array(pixels.prefix(4)) == [255, 0, 0, 255])
    #expect(stride(from: 0, to: 32 * 32 * 4, by: 4).allSatisfy {
      Array(pixels[$0 ..< $0 + 4]) == [255, 0, 0, 255]
    })
  }
}

/// A custom source's callback answers through the map that it borrows, which
/// it can't release.
@Test func aCustomSourceAnswersThroughTheMapItBorrows() async throws {
  try await withOffscreenSession { rendered in
    let map = rendered.fixture.map
    let calls = LockedBox<[(
      id: UInt64,
      sourceId: String,
      refusal: Status?
    )]>([])
    try await map.addCustomGeometrySource(
      "custom",
      options: CustomGeometrySourceOptions(
        fetchTile: { borrowed, sourceId, tile in
          try? borrowed.discarding.setCustomGeometrySourceTileData(
            sourceId,
            tileId: tile,
            data: #"{"type": "FeatureCollection", "features": []}"#
          )
          var refusal: Status?
          do { try borrowed.dispose() } catch {
            refusal = (error as? MaplibreError)?.status
          }
          calls.update { $0.append((borrowed.id, sourceId, refusal)) }
        }
      )
    )
    try await map.addStyleLayerJson(
      #"{"id": "dots", "type": "circle", "source": "custom"}"#
    )
    // The map asks for its tiles as it renders.
    for _ in 0 ..< 32 where calls.value.isEmpty {
      try await rendered.session.awaitRenderedFrame()
    }
    await awaitCondition("a tile fetch") { !calls.value.isEmpty }
    let first = try #require(calls.value.first)
    #expect(first.id == map.id)
    #expect(first.sourceId == "custom")
    #expect(first.refusal == .invalidState)
    #expect(!map.isClosed)
  }
}

#if canImport(Metal)
  /// A Metal frame view lends its texture, device, and pixel format as Metal
  /// types: the texture the session rendered, on the device it attached with.
  @Test func aMetalFrameLendsTypedObjects() async throws {
    try await withBorrowedTextureSession { rendered in
      let graphics = try #require(rendered.graphics)
      // The build's backend decides whether the session renders with Metal.
      guard graphics.backend == MLN_TEST_GRAPHICS_BACKEND_METAL
      else { return }
      try await rendered.session.awaitRenderedFrame()
      let frame = try #require(try rendered.session.acquireFrame())
      try frame.withMetalTexture { view in
        let texture = try view.mtlTexture
        #expect(texture.width == 32)
        #expect(view.mtlPixelFormat == .bgra8Unorm)
        #expect(try view.mtlDevice === graphics.metalDevice)
        #expect(texture.device === graphics.metalDevice)
      }
      try frame.release(consumerCompletion: GpuSync(kind: .cpuComplete))
    }
  }
#endif

/// Before any demand, the drain and the acquire write null handles, which read
/// as no results and no frame.
@Test func aDrainBeforeAnyDemandReturnsNoValue() async throws {
  try await withBorrowedTextureSession { rendered in
    let results = try rendered.session.drainFrameResults()
    let frame = try rendered.session.acquireFrame()
    #expect(results.isEmpty)
    #expect(frame == nil)
  }
}

/// A frame view lends its texture only inside its callback, while its plain
/// values stay readable after. A release of the frame and an abandon of its
/// session are refused while it is open, and neither a second view of the same
/// frame nor a sibling frame that the host drops inside the view disturbs it.
@Test func aFrameViewExpiresWithItsScopeAndHoldsOffAbandon() async throws {
  try await withBorrowedTextureSession(ringDepth: 2) { rendered in
    let graphics = try #require(rendered.graphics)
    let session = rendered.session
    try await session.awaitRenderedFrame()
    let first = try #require(try session.acquireFrame())

    let escaped = try graphics.withTextureView(of: first) { view in
      #expect(view.width == 32)
      #expect(view.height == 32)
      try view.readTexture()
      #expect(try graphics.withTextureView(of: first) { $0.width } == 32)
      do {
        try first.release(consumerCompletion: GpuSync(kind: .cpuComplete))
        Issue.record("release inside a view should be busy")
      } catch let error as MaplibreError {
        #expect(error.status == .busy)
      }
      do {
        _ = try session.abandon()
        Issue.record("abandon inside a frame view should be refused")
      } catch let error as MaplibreError {
        #expect(error.status == .busy)
      }
      return view
    }
    #expect(escaped.width == 32)
    #expect(throws: MaplibreError.self) { try escaped.readTexture() }

    // The refused release left the frame open; a second release after the
    // accepted one is a no-op.
    #expect(!first.isClosed)
    try first.release(consumerCompletion: GpuSync(kind: .cpuComplete))
    #expect(first.isClosed)
    try first.release(consumerCompletion: GpuSync(kind: .cpuComplete))
    #expect(
      first.description
        == "AcquiredFrameHandle(id=0x\(String(first.id, radix: 16)), closed=true)"
    )
  }
}

/// A synchronous dispose of an attached session reports nothing, and the
/// release of its map completes once the session has retired.
@Test func disposingAnAttachedSessionLetsItsMapRelease() async throws {
  let rendered = try await attachOffscreenSession(style: redStyle)
  #expect(try await rendered.session.awaitRenderedFrame() != nil)
  try rendered.session.dispose()
  #expect(rendered.session.isClosed)
  // A second dispose is a no-op.
  try rendered.session.dispose()
  try await rendered.fixture.map.release()
  await rendered.fixture.release()
}

/// Releasing an attached session completes once the session has retired, so
/// its map takes another session right away.
@Test func releasingAnAttachedSessionCompletesOnceItHasRetired() async throws {
  try await attachOffscreenSession(style: redStyle).use { rendered in
    #expect(try await rendered.session.awaitRenderedFrame() != nil)
    try await rendered.session.release()
    // Native code refuses an attach while the map still has a session.
    let next = try rendered.fixture.map.attachOffscreen(
      descriptor: OffscreenDescriptor(
        logicalSize: LogicalSize(width: 32, height: 32)
      ),
      options: RenderSessionAttachOptions(driver: .coreWorker)
    )
    defer { try? next.dispose() }
    try await next.ready
  }
}

/// A caller-driven session runs on a thread the host owns: the thread
/// services driver work whenever the session's driver-work wake fires, which
/// completes the attach and renders frames.
@Test func aCallerDrivenSessionIsServicedOnAHostThread() async throws {
  let fixture = try await MapFixture.make()
  let attached: AttachedSession
  do {
    attached = try await attachBorrowedTexture(
      fixture: fixture,
      callerDriven: true
    )
  } catch {
    await fixture.release()
    throw error
  }
  try await attached.use { rendered in
    let thread = try #require(rendered.thread)
    let session = rendered.session
    #expect(try session.getSnapshot().state == .attaching)
    thread.service(session)
    try await session.ready
    #expect(try session.getCapabilities().driver == .callerGraphicsThread)

    // A demand that waits for the style's first update wakes the thread when
    // the update arrives.
    try session.demandFrame(FrameDemand(flags: [], token: 1))
    try await fixture.map.setStyleJson(redStyle)
    #expect(try await session.awaitRenderedFrame()?.disposition == .rendered)

    // Detaching needs the driver serviced; releasing needs the thread done with
    // the session. The textures go first, while the thread holds their
    // context.
    try await session.detach()
    await rendered.destroyTextures()
    thread.stop()
    try await session.release()
  }
}
