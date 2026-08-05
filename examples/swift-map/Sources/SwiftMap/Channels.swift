import Foundation
import MaplibreNativeFFI

/// A camera change decoded on the render loop and applied on the map's owner
/// thread. Commands carry deltas wherever the current camera is an input,
/// because the read and write have to happen together on the owner thread.
enum CameraCommand {
  case cancelTransitions
  case setGestureInProgress(Bool)
  case moveBy(dx: Double, dy: Double)
  case moveByAnimated(dx: Double, dy: Double, animation: AnimationOptions)
  case scaleBy(scale: Double, anchor: ScreenPoint)
  case scaleByAnimated(
    scale: Double,
    anchor: ScreenPoint,
    animation: AnimationOptions
  )
  case adjustBearing(delta: Double)
  case adjustBearingAnimated(delta: Double, animation: AnimationOptions)
  case adjustPitch(delta: Double)
  case adjustPitchAnimated(delta: Double, animation: AnimationOptions)
  case resetOrientation(animation: AnimationOptions)
}

/// The cross-thread surface between the render loop, which owns the view, the
/// Metal objects, and the render session, and the runtime loop, which owns the
/// runtime and the map.
///
/// The runtime loop that reads these channels MUST be a dedicated `Thread`.
/// Native owner-thread checks are keyed on the OS thread, and a serial
/// `DispatchQueue`, an `actor`, or a `Task` may run successive blocks on
/// different threads, producing `MLN_STATUS_WRONG_THREAD` failures.
final class Channels: @unchecked Sendable {
  private let condition = NSCondition()
  private var commands: [CameraCommand] = []
  private var renderRequested = true
  private var publishedAttachRef: MapAttachRef?
  /// Releases the runtime loop's parked pump. Set once the loop has published.
  private var wake: WakeSource?
  private var shutdown = false
  private var failure: String?
  private var runtimeLoopFinished = false

  // MARK: - Camera commands (render loop to runtime loop)

  /// Render loop: queues a decoded camera change and wakes the runtime loop.
  /// The buffer grows rather than dropping, because deltas and gesture brackets
  /// are not recoverable once discarded.
  func push(_ command: CameraCommand) {
    condition.lock()
    commands.append(command)
    let source = wake
    condition.unlock()
    // The runtime loop parks inside the native pump, not on this condition.
    // Signal outside the lock so a native call never runs under it.
    try? source?.signal()
  }

  /// Runtime loop: swaps `batch` in for the pending commands, keeping the
  /// locked section to the swap alone.
  func drainCommands(into batch: inout [CameraCommand]) {
    // Clearing releases the elements just applied, so do it outside the lock.
    batch.removeAll(keepingCapacity: true)
    condition.lock()
    defer { condition.unlock() }
    swap(&commands, &batch)
  }

  // MARK: - Render request (runtime loop to render loop)

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

  // MARK: - Attach reference (runtime loop to render loop)

  /// Runtime loop: announces the map it just created.
  func publish(attachRef: MapAttachRef, wake: WakeSource) {
    condition.lock()
    defer { condition.unlock() }
    self.wake = wake
    publishedAttachRef = attachRef
  }

  /// Render loop: the reference to attach against, once the runtime loop has a
  /// map.
  func attachRef() -> MapAttachRef? {
    condition.lock()
    defer { condition.unlock() }
    return publishedAttachRef
  }

  // MARK: - Shutdown and failure

  /// Render loop: asks the runtime loop to stop. Called only after the render
  /// session is closed, because the map cannot be destroyed before then.
  func requestShutdown() {
    condition.lock()
    shutdown = true
    condition.broadcast()
    let source = wake
    condition.unlock()
    // Release the pump so shutdown is observed now.
    try? source?.signal()
  }

  /// Runtime loop: blocks until the render loop has closed its session. The map
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

  var isShutdownRequested: Bool {
    condition.lock()
    defer { condition.unlock() }
    return shutdown
  }

  /// Records the first failure from either loop. It crosses as text because
  /// `any Error` carries no `Sendable` guarantee.
  func fail(_ error: Error) {
    condition.lock()
    defer { condition.unlock() }
    if failure == nil {
      failure = String(describing: error)
    }
    condition.broadcast()
  }

  var failureMessage: String? {
    condition.lock()
    defer { condition.unlock() }
    return failure
  }

  /// Runtime loop: reports that it has closed the map and the runtime and is
  /// about to exit.
  func markRuntimeLoopFinished() {
    condition.lock()
    defer { condition.unlock() }
    runtimeLoopFinished = true
    condition.broadcast()
  }

  /// Render loop: waits for the runtime loop to finish. Returns false when the
  /// deadline passed first.
  func waitForRuntimeLoopExit(timeout: TimeInterval) -> Bool {
    let deadline = Date(timeIntervalSinceNow: timeout)
    condition.lock()
    defer { condition.unlock() }
    while !runtimeLoopFinished {
      if !condition.wait(until: deadline) {
        return runtimeLoopFinished
      }
    }
    return true
  }

  /// Render loop: releases the runtime loop's parked pump.
  func wakeRuntimeLoop() {
    condition.lock()
    let source = wake
    condition.unlock()
    try? source?.signal()
  }
}
