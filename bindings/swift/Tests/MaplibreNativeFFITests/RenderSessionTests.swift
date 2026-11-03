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

  /// Runs `body`, then closes the fixture, also when `body` throws. The
  /// session retires before its textures and graphics go: a session still
  /// attached when they go away renders into destroyed objects, which crashes
  /// the process before the failure that skipped the close is reported.
  func use<Result>(
    _ body: (AttachedSession) async throws -> Result
  ) async throws -> Result {
    let result: Result
    do {
      result = try await body(self)
    } catch {
      await close()
      throw error
    }
    await close()
    return result
  }

  /// Detaches and disposes the session, destroys its textures, then closes
  /// the map fixture. A session that cannot detach, such as one already
  /// abandoned or never attached, is abandoned instead, and one the test
  /// closed stays closed.
  func close() async {
    do {
      try await session.detach()
    } catch {
      thread?.stop()
      _ = try? session.abandon()
    }
    await destroyTextures()
    thread?.stop()
    do { try session.dispose() } catch {
      Issue.record("closing the render session failed: \(error)")
    }
    await fixture.close()
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
/// once the attach completes, closing the session afterwards.
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
  let created: NewRenderSession
  do {
    try await fixture.map.setStyleJson(style)
    created = try fixture.map.attachOffscreen(
      descriptor: OffscreenDescriptor(
        extent: LogicalExtent(width: 32, height: 32, scaleFactor: 1)
      ),
      options: RenderSessionAttachOptions(
        driver: .coreWorker,
        frameWake: pulsingFrameWake
      )
    )
  } catch {
    await fixture.close()
    throw error
  }
  let attached = AttachedSession(
    fixture: fixture,
    graphics: nil,
    textures: [],
    session: created.session,
    thread: nil
  )
  do {
    try await created.ready.value
  } catch {
    await attached.close()
    throw error
  }
  return attached
}

/// Attaches a borrowed-texture session with a ring of `ringDepth` textures to
/// a map showing `style` and runs `body` once the attach completes, closing
/// the session afterwards.
private func withBorrowedTextureSession<Result>(
  style: String = emptyStyle,
  ringDepth: Int = 1,
  _ body: (AttachedSession) async throws -> Result
) async throws -> Result {
  let fixture = try await MapFixture.make()
  let attachment: (AttachedSession, NewRenderSession)
  do {
    try await fixture.map.setStyleJson(style)
    attachment = try await attachBorrowedTexture(
      fixture: fixture,
      ringDepth: ringDepth
    )
  } catch {
    await fixture.close()
    throw error
  }
  let (attached, created) = attachment
  return try await attached.use { rendered in
    rendered.thread?.service(rendered.session)
    try await created.ready.value
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
) async throws -> (AttachedSession, NewRenderSession) {
  let graphics = try TestGraphics()
  let textures = try graphics.createTextures(count: ringDepth)
  let thread = callerDriven || graphics.usesCurrentContext
    ? RenderThread()
    : nil
  do {
    try await thread?.perform { try graphics.makeCurrentIfNeeded() }
    let created = try graphics.attachBorrowedTexture(
      map: fixture.map,
      textures: textures,
      options: RenderSessionAttachOptions(
        driver: thread == nil ? .coreWorker : .callerGraphicsThread,
        frameWake: pulsingFrameWake,
        driverWorkWake: thread?.driverWorkWake ?? Wake()
      )
    )
    let attached = AttachedSession(
      fixture: fixture,
      graphics: graphics,
      textures: textures,
      session: created.session,
      thread: thread
    )
    return (attached, created)
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
      try frame.release(consumerCompletion: GpuSync())
    }
  }
#endif

/// Before any demand, native reports the drain and the acquire as not ready,
/// which reads as no results and no frame.
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
        try first.release(consumerCompletion: GpuSync())
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
    try first.release(consumerCompletion: nil)
    #expect(first.isClosed)
    try first.release(consumerCompletion: nil)
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
  try await rendered.fixture.map.close()
  await rendered.fixture.close()
}

/// Closing an attached session completes once the session has retired, so
/// its map takes another session right away.
@Test func closingAnAttachedSessionCompletesOnceItHasRetired() async throws {
  try await attachOffscreenSession(style: redStyle).use { rendered in
    #expect(try await rendered.session.awaitRenderedFrame() != nil)
    try await rendered.session.close()
    // Native code refuses an attach while the map still has a session.
    let next = try rendered.fixture.map.attachOffscreen(
      descriptor: OffscreenDescriptor(
        extent: LogicalExtent(width: 32, height: 32, scaleFactor: 1)
      ),
      options: RenderSessionAttachOptions(driver: .coreWorker)
    )
    defer { try? next.session.dispose() }
    try await next.ready.value
  }
}

/// A caller-driven session runs on a thread the host owns: the thread
/// services driver work whenever the session's driver-work wake fires, which
/// completes the attach and renders frames.
@Test func aCallerDrivenSessionIsServicedOnAHostThread() async throws {
  let fixture = try await MapFixture.make()
  let attachment: (AttachedSession, NewRenderSession)
  do {
    attachment = try await attachBorrowedTexture(
      fixture: fixture,
      callerDriven: true
    )
  } catch {
    await fixture.close()
    throw error
  }
  let (attached, created) = attachment
  try await attached.use { rendered in
    let thread = try #require(rendered.thread)
    let session = rendered.session
    #expect(try session.getSnapshot().state == .attaching)
    thread.service(session)
    try await created.ready.value
    #expect(try session.getCapabilities().driver == .callerGraphicsThread)

    // A demand that waits for the style's first update wakes the thread when
    // the update arrives.
    try session.requestFrame(demand: FrameDemand(flags: [], token: 1))
    try await fixture.map.setStyleJson(redStyle)
    #expect(try await session.awaitRenderedFrame()?.disposition == .rendered)

    // Detaching needs the driver serviced; closing needs the thread done with
    // the session. The textures go first, while the thread holds their
    // context.
    try await session.detach()
    await rendered.destroyTextures()
    thread.stop()
    try await session.close()
  }
}
