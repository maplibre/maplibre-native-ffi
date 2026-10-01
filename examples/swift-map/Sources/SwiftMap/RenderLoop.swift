import Foundation
import MaplibreNativeFFI

/// The render loop for one map and render target. Native wakes reach it on the
/// main actor: a runtime event drain demands a frame for each map update, a
/// driver wake services the session, and a frame-result drain shows what
/// rendered. Input submits camera commands to `mapState` directly.
@MainActor
final class RenderLoop {
  /// How long a frame that did not reach the layer waits before it retries,
  /// about one display refresh.
  private static let frameRetry = Duration.milliseconds(16)

  let mapState: MapState
  private let graphics: MetalGraphicsContext
  private let target: MetalRenderTarget
  private var isClosed = false
  /// Counts resizes, so a queued resize that a newer one supersedes is
  /// skipped.
  private var resizeCount = 0
  /// Runs after each frame that reached the layer.
  var onPresented: (@MainActor () -> Void)?
  /// Runs when the loop can no longer render.
  var onFailure: (@MainActor (Error) -> Void)?

  private init(
    mapState: MapState,
    graphics: MetalGraphicsContext,
    target: MetalRenderTarget
  ) {
    self.mapState = mapState
    self.graphics = graphics
    self.target = target
  }

  /// Creates the map at `viewport`, loading `styleJSON` or the network style
  /// when it is nil, and attaches the render target.
  static func start(
    mode: RenderTargetMode,
    graphics: MetalGraphicsContext,
    viewport: Viewport,
    styleJSON: Data? = nil
  ) async throws -> RenderLoop {
    // The wakes exist before the loop does, so a relay forwards them.
    let relay = WakeRelay()
    let mapState = try await MapState(
      viewport: viewport,
      eventWake: relay.wake { $0.drainEvents() },
      styleJSON: styleJSON
    )
    let target: MetalRenderTarget
    do {
      target = try await MetalRenderTarget.attach(
        mode: mode,
        map: mapState.map,
        graphics: graphics,
        viewport: viewport,
        frameWake: relay.wake { $0.showFrameResults() }
      )
    } catch {
      try? await mapState.close()
      throw error
    }
    let loop = RenderLoop(
      mapState: mapState,
      graphics: graphics,
      target: target
    )
    mapState.onFailure = { [weak loop] in loop?.fail($0) }
    target.onFailure = { [weak loop] in loop?.fail($0) }
    relay.loop = loop
    // Wakes that arrived before the relay knew the loop found nothing to
    // forward to, so drain and demand once now.
    loop.drainEvents()
    loop.showFrameResults()
    loop.requestFrame()
    return loop
  }

  /// Follows a new viewport. The resize queues behind the map's commands, and
  /// a live session carries the extent to the map.
  func resize(_ viewport: Viewport) {
    guard !isClosed else { return }
    resizeCount += 1
    let count = resizeCount
    mapState.submit { [weak self] in
      guard let self, count == self.resizeCount else { return }
      try await self.target.resize(
        graphics: self.graphics,
        viewport: viewport,
        map: self.mapState.map
      )
    }
  }

  /// Detaches the session, then releases the map and the runtime; a map with
  /// an attached session cannot be released.
  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    defer { onPresented = nil }
    do {
      try await target.close()
    } catch {
      try? await mapState.close()
      throw error
    }
    try await mapState.close()
  }

  private func drainEvents() {
    guard !isClosed else { return }
    do {
      if try mapState.drainEvents() {
        requestFrame()
      }
    } catch {
      fail(error)
    }
  }

  private func showFrameResults() {
    guard !isClosed else { return }
    do {
      let results = try target.drainResults()
      let presented = try results.rendered && target.present()
      if presented {
        onPresented?()
      }
      if results.targetNotReady || (results.rendered && !presented) {
        // The map update was consumed without reaching the layer, so the
        // retry forces a frame rather than waiting for another update.
        Task { @MainActor [weak self] in
          try? await Task.sleep(for: Self.frameRetry)
          self?.requestFrame(force: true)
        }
      } else if results.needsRepaint {
        requestFrame()
      }
    } catch {
      fail(error)
    }
  }

  private func requestFrame(force: Bool = false) {
    guard !isClosed else { return }
    do {
      try target.requestFrame(force: force)
    } catch {
      fail(error)
    }
  }

  private func fail(_ error: Error) {
    guard !isClosed else { return }
    onFailure?(error)
  }
}

/// Forwards native wakes to the loop once it exists. A wake only schedules the
/// loop's work on the main actor and returns. The wakes keep the relay alive,
/// and the relay holds the loop weakly, so no cycle forms.
@MainActor
private final class WakeRelay {
  weak var loop: RenderLoop?

  func wake(
    _ work: @escaping @MainActor @Sendable (RenderLoop) -> Void
  ) -> Wake {
    Wake(callback: { [self] in
      Task { @MainActor in
        if let loop { work(loop) }
      }
    })
  }
}
