import Foundation
import MaplibreNative

/// A camera change decoded on the render loop and applied on the map's owner
/// thread.
///
/// Commands carry deltas rather than absolute targets wherever the map's
/// current camera is an input, because reading the camera and writing the new
/// one has to happen together on the thread that owns the map.
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

/// The entire cross-thread surface between the render loop, which owns the
/// view, the Metal objects, and the render session, and the runtime loop, which
/// owns the runtime and the map.
///
/// The channels match the map example specification: a camera-command queue
/// going one way, a render request coming back, an attach reference published
/// once so the render loop can attach its own session, and shutdown plus
/// first-failure.
///
/// The runtime loop that reads these channels MUST be a dedicated `Thread`.
/// Native owner-thread checks are keyed on the OS thread, and a serial
/// `DispatchQueue` guarantees serialization but not thread affinity: it may run
/// successive blocks on different threads. A queue, an `actor`, or a `Task`
/// here would produce nondeterministic `MLN_STATUS_WRONG_THREAD` failures.
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
  ///
  /// The buffer grows rather than dropping. Its commands are deltas and a
  /// gesture bracket, and neither survives being discarded: a dropped delta is
  /// motion the drag never gets back, and a dropped bracket leaves every delta
  /// after it attributed to no gesture. Growing does not block the render loop
  /// either, and only a stalled runtime loop grows it at all.
  func push(_ command: CameraCommand) {
    condition.lock()
    commands.append(command)
    let source = wake
    condition.unlock()
    // Release the parked pump so this command is applied now rather than after
    // the parking bound. The runtime loop parks inside the native pump, not on
    // this condition, so there is nothing here to signal. Signal outside the
    // lock so a native call never runs under it.
    try? source?.signal()
  }

  /// Runtime loop: hands the pending commands over and takes `batch` in
  /// exchange, so the two ping-pong and the locked section is the swap alone.
  ///
  /// Reading the array out and clearing it under the lock would not do: the
  /// read leaves the buffer shared, so the clear has to allocate a fresh one
  /// sized to the backlog, and it does that every drain.
  func drainCommands(into batch: inout [CameraCommand]) {
    // Clearing releases the elements of the batch just applied, so it happens
    // before the lock is taken. Only the runtime loop holds `batch` here; the
    // queue is still filling the other array.
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
  ///
  /// The render loop consumes before it renders and sets again when nothing was
  /// rendered, so a request the runtime loop publishes during a render is not
  /// lost.
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
    // Under the same lock as every other reader, so a render loop that wakes
    // during publication sees either no source or the published one.
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

  /// Records the first failure from either loop. The other loop stops on it.
  ///
  /// The failure crosses as text rather than as an `Error`, because `any Error`
  /// carries no `Sendable` guarantee.
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

  /// Render loop: waits for the runtime loop to finish, the way a host joins a
  /// thread. Returns false when the deadline passed first.
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

  /// Runtime loop: paces one iteration, waking early for a queued command or a
  /// shutdown request.
  /// Render loop: releases the runtime loop's parked pump.
  func wakeRuntimeLoop() {
    condition.lock()
    let source = wake
    condition.unlock()
    try? source?.signal()
  }
}
