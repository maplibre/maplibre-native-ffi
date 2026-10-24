import Foundation
import GraphicsSupport
@testable import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
#endif

/// A render fixture: an owned-texture session attached on the map of a map
/// fixture, with a context from tests/graphics for the build's backend.
private struct OwnedTextureSession {
  let fixture: MapFixture
  let graphics: TestGraphics
  let session: RenderSessionHandle
  /// The host thread that drives an OpenGL session, which Metal and Vulkan
  /// sessions do not need.
  let thread: RenderThread?

  /// Runs `body`, then closes the fixture, also when `body` throws. The
  /// graphics device outlives the close: a session still attached when the
  /// device goes away renders on a destroyed device, which crashes the
  /// process before the failure that skipped the close is reported.
  func use<Result>(
    _ body: (OwnedTextureSession) async throws -> Result
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

  /// Detaches and disposes the session, then closes the map fixture. A
  /// session that cannot detach, such as one already abandoned or never
  /// attached, is abandoned instead, and one the test closed stays closed.
  func close() async {
    do {
      try await session.detach()
    } catch {
      thread?.stop()
      _ = try? session.abandon()
    }
    thread?.stop()
    do { try session.dispose() } catch {
      Issue.record("closing the render session failed: \(error)")
    }
    await fixture.close()
    withExtendedLifetime(graphics) {}
  }
}

/// Attaches an owned-texture session to a map showing `style` and runs `body`
/// once the attach completes, closing the session afterwards.
private func withOwnedTextureSession<Result>(
  style: Data = emptyStyle,
  ringDepth: UInt32 = RenderSessionAttachOptions.default
    .requestedTextureRingDepth,
  _ body: (OwnedTextureSession) async throws -> Result
) async throws -> Result {
  try await attachOwnedTextureSession(style: style, ringDepth: ringDepth)
    .use(body)
}

/// Attaches an owned-texture session to a map showing `style` and returns it
/// once the attach completes. A Metal or Vulkan session drives itself on a
/// core worker. An OpenGL session exposes its frames only through the host's
/// share group, so it joins the fixture's context and a host render thread
/// drives it.
private func attachOwnedTextureSession(
  style: Data = emptyStyle,
  ringDepth: UInt32 = RenderSessionAttachOptions.default
    .requestedTextureRingDepth
) async throws -> OwnedTextureSession {
  let fixture = try await MapFixture.make()
  var thread: RenderThread?
  let graphics: TestGraphics
  let created: NewRenderSession
  do {
    try await fixture.map.setStyleJson(style)
    graphics = try TestGraphics()
    if graphics.backend == MLN_TEST_GRAPHICS_BACKEND_EGL {
      let host = RenderThread()
      thread = host
      try await host.perform { try graphics.makeCurrentIfNeeded() }
    }
    created = try graphics.attachOwnedTexture(
      map: fixture.map,
      options: RenderSessionAttachOptions(
        driver: thread == nil ? .coreWorker : .callerGraphicsThread,
        requestedTextureRingDepth: ringDepth,
        frameWake: pulsingFrameWake,
        driverWorkWake: thread?.driverWorkWake ??
          RenderSessionAttachOptions.default.driverWorkWake
      )
    )
  } catch {
    thread?.stop()
    await fixture.close()
    throw error
  }
  let attached = OwnedTextureSession(
    fixture: fixture,
    graphics: graphics,
    session: created.session,
    thread: thread
  )
  do {
    thread?.service(created.session)
    try await created.ready.value
  } catch {
    await attached.close()
    throw error
  }
  return attached
}

/// An owned-texture session renders a frame, and the binding copies its
/// pixels out of the native readback.
@Test func anOwnedTextureSessionRendersPixelsTheBindingReadsBack(
) async throws {
  try await withOwnedTextureSession(style: redStyle) { rendered in
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
    try await withOwnedTextureSession { rendered in
      // The build's backend decides whether the session renders with Metal.
      guard rendered.graphics.backend == MLN_TEST_GRAPHICS_BACKEND_METAL
      else { return }
      try await rendered.session.awaitRenderedFrame()
      let frame = try #require(try rendered.session.acquireFrame())
      try frame.withMetalTexture { view in
        let texture = try view.mtlTexture
        #expect(texture.width == 32)
        #expect(try view.mtlPixelFormat == .bgra8Unorm)
        #expect(try view.mtlDevice === rendered.graphics.metalDevice)
        #expect(texture.device === rendered.graphics.metalDevice)
      }
      try frame.release(consumerCompletion: .default)
    }
  }
#endif

/// Before any demand, native reports the drain and the acquire as not ready,
/// which reads as nil.
@Test func aDrainBeforeAnyDemandReturnsNil() async throws {
  try await withOwnedTextureSession { rendered in
    let batch = try rendered.session.drainFrameResults()
    let frame = try rendered.session.acquireFrame()
    #expect(batch == nil)
    #expect(frame == nil)
  }
}

/// A frame view is usable only inside its callback. A release of the frame
/// and an abandon of its session are refused while it is open, and neither a
/// second view of the same frame nor a sibling frame that the host drops
/// inside the view disturbs it.
@Test func aFrameViewExpiresWithItsScopeAndHoldsOffAbandon() async throws {
  try await withOwnedTextureSession(ringDepth: 2) { rendered in
    let session = rendered.session
    try await session.awaitRenderedFrame()
    let first = try #require(try session.acquireFrame())
    try await session.awaitRenderedFrame()
    var sibling = try Optional(#require(try session.acquireFrame()))
    weak let weakSibling = sibling

    let escaped = try rendered.graphics.withTextureView(of: first) { view in
      sibling = nil
      #expect(weakSibling == nil)
      #expect(try view.width == 32)
      #expect(try view.height == 32)
      #expect(try rendered.graphics
        .withTextureView(of: first) { try $0.width } == 32)
      do {
        try first.release(consumerCompletion: .default)
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
    #expect(throws: MaplibreError.self) { try escaped.width }

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

/// A frame the host drops without releasing is discarded by its finalizer,
/// which hands the frame back to its session without consumer
/// synchronization.
@Test func droppingAnUnreleasedFrameDiscardsIt() async throws {
  try await withOwnedTextureSession { rendered in
    let session = rendered.session
    try await session.awaitRenderedFrame()
    do {
      let frame = try #require(try session.acquireFrame())
      #expect(try session.getSnapshot().acquiredFrameCount == 1)
      withExtendedLifetime(frame) {}
    }
    try await awaitCondition("the abandoned frame to retire") {
      try session.getSnapshot().acquiredFrameCount == 0
    }
  }
}

/// A synchronous dispose of an attached session reports nothing, and the
/// release of its map completes once the session has retired, after which the
/// host destroys the session's graphics device.
@Test func disposingAnAttachedSessionLetsItsMapReleaseBeforeItsGraphicsGo(
) async throws {
  let fixture: MapFixture
  do {
    let rendered = try await attachOwnedTextureSession(style: redStyle)
    fixture = rendered.fixture
    #expect(try await rendered.session.awaitRenderedFrame() != nil)
    try rendered.session.dispose()
    #expect(rendered.session.isClosed)
    // A second dispose is a no-op.
    try rendered.session.dispose()
    try await fixture.map.close()
    rendered.thread?.stop()
    // The device goes with `rendered`, at the end of this scope.
  }
  await fixture.close()
}

/// Closing an attached session completes once the session has retired, after
/// which the host destroys the session's graphics device while the map is
/// still live.
@Test func closingAnAttachedSessionCompletesBeforeItsGraphicsGo(
) async throws {
  let fixture: MapFixture
  do {
    let rendered = try await attachOwnedTextureSession(style: redStyle)
    fixture = rendered.fixture
    #expect(try await rendered.session.awaitRenderedFrame() != nil)
    try await rendered.session.close()
    rendered.thread?.stop()
    // The device goes with `rendered`, at the end of this scope.
  }
  await fixture.close()
}

/// A caller-driven session runs on a thread the host owns: the thread
/// services driver work whenever the session's driver-work wake fires, which
/// completes the attach and renders frames.
@Test func aCallerDrivenSessionIsServicedOnAHostThread() async throws {
  let fixture = try await MapFixture.make()
  let thread = RenderThread()
  let graphics: TestGraphics
  let created: NewRenderSession
  do {
    graphics = try TestGraphics()
    try await thread.perform { try graphics.makeCurrentIfNeeded() }
    created = try graphics.attachOwnedTexture(
      map: fixture.map,
      options: RenderSessionAttachOptions(
        driver: .callerGraphicsThread,
        frameWake: pulsingFrameWake,
        driverWorkWake: thread.driverWorkWake
      )
    )
  } catch {
    thread.stop()
    await fixture.close()
    throw error
  }
  let attached = OwnedTextureSession(
    fixture: fixture,
    graphics: graphics,
    session: created.session,
    thread: thread
  )
  try await attached.use { rendered in
    let session = rendered.session
    #expect(try session.getSnapshot().state == .attaching)
    thread.service(session)
    try await created.ready.value
    #expect(try session.getCapabilities().driver == .callerGraphicsThread)

    // A demand that waits for the style's first update wakes the thread when
    // the update arrives.
    try session.requestFrame(demand: FrameDemand(flags: [.ifNeeded], token: 1))
    try await fixture.map.setStyleJson(redStyle)
    #expect(try await session.awaitRenderedFrame()?.disposition == .rendered)
    let readback = try await session.readTexture()
    #expect(Array(readback.data.prefix(4)) == [255, 0, 0, 255])

    // Detaching needs the driver serviced; closing needs the thread done with
    // the session.
    try await session.detach()
    thread.stop()
    try await session.close()
  }
}
