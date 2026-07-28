import Foundation
import MaplibreNative
import os

/// The runtime loop.
///
/// Owns the runtime and the map for their whole lifetime, on a thread that is
/// not the one presenting. It never touches the render session: the render loop
/// attaches its own against the reference published here. It keeps running
/// across view and app lifecycle transitions, so loading continues while the
/// view is off screen.
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
    // The render loop closes the session before it requests shutdown, so by the
    // time this runs the map has no session attached and can be destroyed.
    defer {
      do {
        try state.close()
      } catch {
        log.error("\(String(describing: error), privacy: .public)")
      }
    }

    let attachRef = try state.attachRef()
    // The render loop signals this to release the parked pump, so a queued
    // command or a shutdown
    // request lands without waiting out the bound below.
    let wake = try state.wakeSource()
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
