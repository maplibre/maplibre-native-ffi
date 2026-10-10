import Foundation
import MaplibreNativeFFI

/// The render loop for one map and its surface. Native wakes reach it on the
/// main actor: a runtime event drain demands a frame for each map update, and
/// a frame-result drain re-arms demand. The session renders on its own core
/// worker. Gestures submit camera commands to `mapState` directly.
@MainActor
final class RenderLoop {
  /// How long the loop waits before it retries a target that was not ready,
  /// about one display refresh. No map-update event prompts that retry.
  private static let frameRetryNanoseconds: UInt64 = 16_000_000

  let mapState: MapState
  private let target: MetalRenderTarget
  private var isClosed = false
  /// Counts resizes, so a queued resize that a newer one supersedes is
  /// skipped.
  private var resizeCount = 0
  /// Whether the view is visible with the app in the foreground. Frame demand
  /// pauses otherwise, while the runtime keeps loading.
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

  /// Follows a new viewport. The resize queues behind the map's commands, and
  /// the session carries the extent to the map.
  func resize(_ viewport: Viewport) {
    guard !isClosed else { return }
    resizeCount += 1
    let count = resizeCount
    mapState.submit { [weak self] in
      guard let self, count == self.resizeCount else { return }
      try await self.target.resize(viewport)
    }
  }

  /// The session driver, for the startup log.
  var driver: RenderDriverKind {
    target.driver
  }

  /// Waits for the frames demanded before demand paused, so none renders
  /// once the app is in the background.
  func awaitRenderBarrier() async {
    guard !isClosed else { return }
    do {
      try await target.barrier()
    } catch {
      fail(error)
    }
  }

  /// Detaches the session, then releases the map and the runtime; a map with
  /// an attached session cannot be released.
  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    do {
      try await target.close()
    } catch {
      try? await mapState.close()
      throw error
    }
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
        // No map-update event follows a target that was not ready, so the
        // retry waits about one display refresh. The map update stays pending.
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
