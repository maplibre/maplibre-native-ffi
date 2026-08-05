import Foundation
import MaplibreNativeFFI
import os

/// The runtime loop, owning the runtime and the map for their whole lifetime.
/// The render loop attaches its own session against the reference published
/// here. It keeps running across view and app lifecycle transitions, so loading
/// continues while the view is off screen.
///
/// This must be a dedicated `Thread`: native owner-thread checks are keyed on
/// the OS thread, and a serial `DispatchQueue`, an `actor`, or a `Task` may run
/// successive blocks on different threads. The view and its window belong to
/// the main thread, which is therefore the render loop.
final class RuntimeLoopThread: Thread {
  /// Backstop for this loop's park; the render loop's wake source normally
  /// releases it.
  private static let parkTimeout = 0.1

  /// Bound on waiting for the render loop to close its session during teardown.
  private static let shutdownWaitTimeout = 5.0

  private let channels: Channels
  private let initialViewport: Viewport
  private let log = Logger(
    subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
    category: "RuntimeLoop"
  )

  init(channels: Channels, viewport: Viewport) {
    self.channels = channels
    initialViewport = viewport
    super.init()
    name = "org.maplibre.nativeffi.examples.swift-map-ios.runtime-loop"
  }

  override func main() {
    do {
      try run()
    } catch {
      channels.fail(error)
    }
    channels.markRuntimeLoopFinished()
  }

  private func run() throws {
    let state = try MapState(viewport: initialViewport)
    // A map with an attached session cannot be destroyed, so wait for the
    // render loop to close its session. Failures are published before this
    // wait runs, so the render loop sees them and releases the wait.
    defer {
      channels.waitForShutdown(timeout: Self.shutdownWaitTimeout)
      do {
        try state.close()
      } catch {
        log.error("\(String(describing: error), privacy: .public)")
      }
    }

    do {
      try pumpUntilShutdown(state: state)
    } catch {
      channels.fail(error)
    }
  }

  private func pumpUntilShutdown(state: MapState) throws {
    let attachRef = try state.attachRef()
    let wake = try state.wakeSource()
    // A wake source is its own native handle: closing the runtime does not
    // release it.
    defer { try? wake.close() }
    channels.publish(attachRef: attachRef, wake: wake)

    var batch: [CameraCommand] = []

    while !channels.isShutdownRequested, channels.failureMessage == nil {
      channels.drainCommands(into: &batch)
      for command in batch {
        try state.apply(command)
      }
      // No display paces this thread, so it parks in the pump until the
      // runtime has work or the render loop signals the wake source.
      try state.pump(timeout: Self.parkTimeout)
      if try state.drainEvents() {
        channels.setRenderRequest()
      }
    }
  }
}
