import Foundation
import MaplibreNativeFFI

/// The render loop for one map and its surface. Native wakes reach it on the
/// main actor: a runtime event drain demands a frame for each map update, a
/// driver wake services the session, and a frame-result drain re-arms
/// demand. Gestures submit camera commands to `mapState` directly.
@MainActor
final class RenderLoop {
  /// How long a frame that did not reach the layer waits before it retries,
  /// about one display refresh.
  private static let frameRetryNanoseconds: UInt64 = 16_000_000

  let mapState: MapState
  private let target: MetalRenderTarget
  private var isClosed = false
  /// Whether the view is visible with the app in the foreground. Frame demand
  /// pauses otherwise, while driver work keeps being serviced.
  var isPresenting = true {
    didSet {
      if isPresenting, !oldValue { requestFrame() }
    }
  }

  /// Runs when the loop can no longer render.
  var onFailure: (@MainActor (Error) -> Void)?

  private init(mapState: MapState, target: MetalRenderTarget) {
    self.mapState = mapState
    self.target = target
  }

  /// Creates the map at `viewport` and attaches the surface.
  static func start(
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws -> RenderLoop {
    // The wakes exist before the loop does, so a relay forwards them.
    let relay = WakeRelay()
    let mapState = try await MapState(
      viewport: viewport,
      eventWake: relay.wake { $0.drainEvents() }
    )
    let target: MetalRenderTarget
    do {
      target = try await MetalRenderTarget.attach(
        map: mapState.map,
        graphics: graphics,
        viewport: viewport,
        frameWake: relay.wake { $0.showFrameResults() }
      )
    } catch {
      try? await mapState.close()
      throw error
    }
    let loop = RenderLoop(mapState: mapState, target: target)
    mapState.onFailure = { [weak loop] in loop?.fail($0) }
    relay.loop = loop
    // Wakes that arrived before the relay knew the loop found nothing to
    // forward to, so drain and demand once now.
    loop.drainEvents()
    loop.showFrameResults()
    loop.requestFrame()
    return loop
  }

  /// Follows a new viewport. The session carries the extent to the map.
  func resize(_ viewport: Viewport) {
    guard !isClosed else { return }
    target.resize(viewport, onFailure: { [weak self] in self?.fail($0) })
  }

  /// Detaches the session, then releases the map and the runtime; a map with
  /// an attached session cannot be released.
  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    try await target.close()
    try await mapState.close()
  }

  /// Abandons the session without graphics work, for a view released without
  /// leaving its window, whose deinit cannot await teardown.
  func abandon() {
    guard !isClosed else { return }
    isClosed = true
    target.abandon()
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
      if results.targetNotReady {
        // The map update was consumed without reaching the layer, so the
        // retry forces a frame rather than waiting for another update.
        Task { @MainActor [weak self] in
          try? await Task.sleep(nanoseconds: Self.frameRetryNanoseconds)
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
    guard !isClosed, isPresenting else { return }
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
