import Foundation
import MaplibreNative

/// The runtime loop.
///
/// Owns the runtime and the map for their whole lifetime, on a thread that is
/// not the one presenting. It never touches the render session: the render loop
/// attaches its own against the reference published here.
///
/// This is a dedicated `Thread` on purpose. Native owner-thread checks are
/// keyed on the OS thread, and a serial `DispatchQueue` guarantees
/// serialization but not thread affinity, so a queue, an `actor`, or a `Task`
/// here would produce nondeterministic `MLN_STATUS_WRONG_THREAD` failures. On
/// Apple platforms the view and its window belong to the main thread, so main
/// is the render loop and this is the thread that gets spawned.
final class RuntimeLoopThread: Thread {
  /// Backstop for this loop's park. The render loop's wake source is what
  /// normally releases it, so this only bounds a pump that nothing signals.
  private static let parkTimeout = 0.1

  /// Bound on waiting for the render loop to close its session during teardown.
  private static let shutdownWaitTimeout = 5.0

  private let channels: Channels
  private let initialViewport: Viewport

  init(channels: Channels, viewport: Viewport) {
    self.channels = channels
    initialViewport = viewport
    super.init()
    name = "org.maplibre.nativeffi.examples.swift-map.runtime-loop"
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
    // However this loop exits, the render loop still owns the session, and a
    // map
    // with an attached session cannot be destroyed. On the failure path the
    // render loop has not closed it yet, so wait for the shutdown signal before
    // closing rather than failing and swallowing the error.
    //
    // Everything below publishes its failure before this wait runs, so the
    // render loop sees it, closes its session, and releases us. Waiting first
    // would stall until the bound expires.
    defer {
      channels.waitForShutdown(timeout: Self.shutdownWaitTimeout)
      do {
        try state.close()
      } catch {
        print(error)
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
    // The render loop signals this to release the parked pump, so a queued
    // command or a shutdown
    // request lands without waiting out the bound below.
    let wake = try state.wakeSource()
    // A wake source is its own native handle and outlives the runtime, so
    // closing the runtime does not release it.
    defer { try? wake.close() }
    channels.publish(attachRef: attachRef, wake: wake)

    while !channels.isShutdownRequested, channels.failureMessage == nil {
      for command in channels.drainCommands() {
        try state.apply(command)
      }
      // This thread has no display to pace it, so it takes its cadence from the
      // runtime's own
      // work and parks in between. The render loop signals the wake source, so
      // the bound is a
      // backstop rather than the cadence.
      try state.pump(timeout: Self.parkTimeout)
      if try state.drainEvents() {
        channels.setRenderRequest()
      }
    }
  }
}
