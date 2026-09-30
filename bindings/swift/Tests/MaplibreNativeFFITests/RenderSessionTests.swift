import Foundation
@testable import MaplibreNativeFFI
import Testing

/// A render fixture: an owned-texture session attached on the map of a map
/// fixture, with a context from tests/graphics for the build's backend.
private struct OwnedTextureSession {
  let fixture: MapFixture
  let graphics: TestGraphics
  let session: RenderSessionHandle

  /// Detaches and closes the session, then closes the map fixture.
  func close() async {
    do {
      try await session.detach()
      try session.close()
    } catch {
      _ = try? session.abandon()
      try? session.close()
    }
    await fixture.close()
  }
}

/// Attaches a core-worker owned-texture session to a map showing `style`.
private func attachOwnedTexture(
  style: Data = emptyStyle,
  ringDepth: UInt32 = RenderSessionAttachOptions.default
    .requestedTextureRingDepth
) async throws -> OwnedTextureSession {
  let fixture = try await MapFixture.make()
  do {
    try await fixture.map.setStyleJson(json: style)
    let graphics = try TestGraphics()
    let attachment = try graphics.attachOwnedTexture(
      map: fixture.map,
      options: RenderSessionAttachOptions(
        driver: .coreWorker,
        requestedTextureRingDepth: ringDepth,
        frameWake: pulsingFrameWake
      )
    )
    try await attachment.completion.value
    return OwnedTextureSession(
      fixture: fixture,
      graphics: graphics,
      session: attachment.session
    )
  } catch {
    await fixture.close()
    throw error
  }
}

/// An owned-texture session renders a frame, and the binding copies its
/// pixels out of the native readback.
@Test func anOwnedTextureSessionRendersPixelsTheBindingReadsBack(
) async throws {
  let rendered = try await attachOwnedTexture(style: redStyle)
  #expect(try await rendered.session.awaitRenderedFrame() != nil)

  let readback = try await rendered.session.textureReadPremultipliedRgba8()
  #expect(readback.info.width == 32)
  #expect(readback.info.height == 32)
  let pixels = [UInt8](readback.data)
  #expect(pixels.count >= 32 * 32 * 4)
  #expect(Array(pixels.prefix(4)) == [255, 0, 0, 255])
  #expect(stride(from: 0, to: 32 * 32 * 4, by: 4).allSatisfy {
    Array(pixels[$0 ..< $0 + 4]) == [255, 0, 0, 255]
  })
  await rendered.close()
}

/// A frame view is usable only inside its callback, a second view of the
/// same frame and an abandon of its session are refused while it is open,
/// and a sibling frame that the host drops inside the view does not disturb
/// it.
@Test func aFrameViewExpiresWithItsScopeAndHoldsOffAbandon() async throws {
  let rendered = try await attachOwnedTexture(ringDepth: 2)
  let session = rendered.session
  try await session.awaitRenderedFrame()
  let first = try session.acquireFrame()
  try await session.awaitRenderedFrame()
  var sibling: AcquiredFrameHandle? = try session.acquireFrame()
  weak let weakSibling = sibling

  let escaped = try rendered.graphics.withTextureView(of: first) { view in
    sibling = nil
    #expect(weakSibling == nil)
    #expect(try view.width == 32)
    #expect(try view.height == 32)
    #expect(throws: MaplibreError.self) {
      try rendered.graphics.withTextureView(of: first) { _ in }
    }
    do {
      _ = try session.abandon()
      Issue.record("abandon inside a frame view should be refused")
    } catch let error as MaplibreError {
      #expect(error.kind == .busy)
    }
    return view
  }
  #expect(throws: MaplibreError.self) { try escaped.width }

  try first.release(consumerCompletion: .default)
  await rendered.close()
}

/// A frame the host drops without releasing is disposed by its finalizer,
/// which abandons the session, since no consumer synchronization says the
/// host finished with the texture.
@Test func droppingAnUnreleasedFrameAbandonsItsSession() async throws {
  let rendered = try await attachOwnedTexture()
  let session = rendered.session
  try await session.awaitRenderedFrame()
  do {
    let frame = try session.acquireFrame()
    #expect(try session.getSnapshot().acquiredFrameCount == 1)
    withExtendedLifetime(frame) {}
  }
  #expect(try session.getSnapshot().acquiredFrameCount == 0)
  try await awaitCondition("the dropped frame's session to be abandoned") {
    try session.getSnapshot().state == .abandoned
  }
  try session.close()
  await rendered.fixture.close()
}

/// A caller-driven session runs on a thread the host owns: the thread
/// services driver work whenever the session's driver-work wake fires, which
/// completes the attach and renders frames.
@Test func aCallerDrivenSessionIsServicedOnAHostThread() async throws {
  let fixture = try await MapFixture.make()
  let thread = RenderThread()
  let graphics = try TestGraphics()
  try await thread.perform { try graphics.makeCurrentIfNeeded() }
  let attachment = try graphics.attachOwnedTexture(
    map: fixture.map,
    options: RenderSessionAttachOptions(
      driver: .callerGraphicsThread,
      frameWake: pulsingFrameWake,
      driverWorkWake: thread.driverWorkWake
    )
  )
  let session = attachment.session
  #expect(try session.getSnapshot().state == .attaching)
  thread.service(session)
  try await attachment.completion.value
  #expect(try session.getSnapshot().driver == .callerGraphicsThread)

  // A demand that waits for the style's first update wakes the thread when
  // the update arrives.
  try session.requestFrame(demand: FrameDemand(flags: [.ifNeeded], token: 1))
  try await fixture.map.setStyleJson(json: redStyle)
  #expect(try await session.awaitRenderedFrame()?.disposition == .rendered)
  let readback = try await session.textureReadPremultipliedRgba8()
  #expect(Array(readback.data.prefix(4)) == [255, 0, 0, 255])

  // Detaching needs the driver serviced; closing needs the thread done with
  // the session.
  try await session.detach()
  thread.stop()
  try session.close()
  withExtendedLifetime(graphics) {}
  await fixture.close()
}
