import Foundation
import MaplibreNativeFFI

/// Owns runtime and map lifecycle in an asynchronous task. Rendering remains on
/// the main AppKit thread through `MetalMapView` and its render session.
final class MapTask: @unchecked Sendable {
  private static let shutdownWaitTimeout = 5.0

  private let channels: Channels
  private let initialViewport: Viewport
  private var task: Task<Void, Never>?

  init(channels: Channels, viewport: Viewport) {
    self.channels = channels
    initialViewport = viewport
  }

  func start() {
    task = Task.detached { [self] in
      await run()
    }
  }

  private func run() async {
    do {
      let state = try await MapState(viewport: initialViewport)
      channels.publish(map: state.mapHandle())
      state.scheduleEventDrains(on: channels)

      for await command in channels.cameraCommands {
        if channels.failureMessage != nil {
          break
        }
        try await state.apply(command)
      }

      channels.waitForShutdown(timeout: Self.shutdownWaitTimeout)
      try await state.close()
    } catch {
      channels.fail(error)
    }
    channels.markMapTaskFinished()
  }
}
