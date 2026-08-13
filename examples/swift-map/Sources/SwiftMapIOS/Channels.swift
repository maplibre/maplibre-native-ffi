import Foundation
import MaplibreNativeFFI

/// A camera change decoded on the main render receiver and submitted by an
/// asynchronous map task.
enum CameraCommand {
  case resize(MapLogicalExtent)
  case setGestureInProgress(Bool)
  case moveBy(dx: Double, dy: Double)
  case scaleBy(scale: Double, anchor: ScreenPoint)
  /// Adds `delta` degrees to the current bearing, pivoting on `anchor`.
  case adjustBearing(delta: Double, anchor: ScreenPoint)
  /// Adds `delta` degrees to the current pitch, clamped to `[0, 60]`.
  case adjustPitch(delta: Double)
  /// Zooms to `round(zoom) + 1` about `anchor`, the double-tap step.
  case zoomToNextStep(anchor: ScreenPoint, animation: AnimationOptions)
}

/// Thread-safe coordination between the main render receiver and async map
/// tasks.
final class Channels: @unchecked Sendable {
  private let condition = NSCondition()
  private let commandStream: AsyncStream<CameraCommand>
  private let commandContinuation: AsyncStream<CameraCommand>.Continuation
  private var renderRequested = true
  private var publishedMap: MapHandle?
  private var shutdown = false
  private var failure: String?
  private var mapTaskFinished = false

  init() {
    (commandStream, commandContinuation) = AsyncStream.makeStream()
  }

  var cameraCommands: AsyncStream<CameraCommand> {
    commandStream
  }

  /// Queues a decoded camera change without polling or a custom command buffer.
  func push(_ command: CameraCommand) {
    commandContinuation.yield(command)
  }

  // MARK: - Render request (map task to render loop)

  func setRenderRequest() {
    condition.lock()
    defer { condition.unlock() }
    renderRequested = true
  }

  /// Render loop: takes the request, if any.
  func consumeRenderRequest() -> Bool {
    condition.lock()
    defer { condition.unlock() }
    let wasRequested = renderRequested
    renderRequested = false
    return wasRequested
  }

  // MARK: - Map publication (map task to render loop)

  /// Announces the map after the map task creates it.
  func publish(map: MapHandle) {
    condition.lock()
    defer { condition.unlock() }
    publishedMap = map
  }

  /// Returns the map once the map task has created it.
  func map() -> MapHandle? {
    condition.lock()
    defer { condition.unlock() }
    return publishedMap
  }

  // MARK: - Shutdown and failure

  /// Render loop: asks the map task to stop. Called only after the render
  /// session is closed, because the map cannot be destroyed before then.
  func requestShutdown() {
    condition.lock()
    shutdown = true
    condition.broadcast()
    condition.unlock()
    commandContinuation.finish()
  }

  /// Map task: blocks until the render loop has closed its session. The map
  /// cannot be destroyed before then. Bounded so a render loop that died
  /// without signalling cannot wedge teardown.
  func waitForShutdown(timeout: TimeInterval) {
    let deadline = Date(timeIntervalSinceNow: timeout)
    condition.lock()
    defer { condition.unlock() }
    while !shutdown {
      if !condition.wait(until: deadline) { return }
    }
  }

  /// Records the first failure from either loop. It crosses as text because
  /// `any Error` carries no `Sendable` guarantee.
  func fail(_ error: Error) {
    condition.lock()
    if failure == nil {
      failure = String(describing: error)
    }
    condition.broadcast()
    condition.unlock()
    commandContinuation.finish()
  }

  var failureMessage: String? {
    condition.lock()
    defer { condition.unlock() }
    return failure
  }

  /// Reports that the map task has closed the map and runtime.
  func markMapTaskFinished() {
    condition.lock()
    defer { condition.unlock() }
    mapTaskFinished = true
    condition.broadcast()
  }

  /// Waits for the map task to finish, up to the supplied deadline.
  func waitForMapTaskExit(timeout: TimeInterval) -> Bool {
    let deadline = Date(timeIntervalSinceNow: timeout)
    condition.lock()
    defer { condition.unlock() }
    while !mapTaskFinished {
      if !condition.wait(until: deadline) {
        return mapTaskFinished
      }
    }
    return true
  }
}
