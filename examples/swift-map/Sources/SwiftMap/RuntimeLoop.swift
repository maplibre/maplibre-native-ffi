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
  /// Pacing ceiling for one iteration, one display refresh period. `runOnce`
  /// never blocks waiting for work, and a queued camera command wakes the wait
  /// early.
  private static let idleTimeout = 1.0 / 60.0

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
    // The render loop closes the session before it requests shutdown, so by the
    // time this runs the map has no session attached and can be destroyed.
    defer {
      do {
        try state.close()
      } catch {
        print(error)
      }
    }

    let attachRef = try state.attachRef()
    channels.publish(attachRef: attachRef)

    while !channels.isShutdownRequested, channels.failureMessage == nil {
      for command in channels.drainCommands() {
        try state.apply(command)
      }
      try state.runOnce()
      if try state.drainEvents() {
        channels.setRenderRequest()
      }
      channels.waitForWork(timeout: Self.idleTimeout)
    }
  }
}
