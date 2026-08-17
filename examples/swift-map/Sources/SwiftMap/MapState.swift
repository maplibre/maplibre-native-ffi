import Foundation
import MaplibreNativeFFI

struct Viewport: Equatable {
  var logicalWidth: UInt32
  var logicalHeight: UInt32
  var physicalWidth: UInt32
  var physicalHeight: UInt32
  var scaleFactor: Double
  var isEmpty: Bool

  var extent: RenderTargetExtent {
    RenderTargetExtent(
      width: logicalWidth,
      height: logicalHeight,
      scaleFactor: scaleFactor
    )
  }

  func log(_ label: String) {
    let scale = String(format: "%.2f", scaleFactor)
    let emptyLabel = isEmpty ? " empty=true" : ""
    print(
      "\(label): logical=\(logicalWidth)x\(logicalHeight) physical=\(physicalWidth)x\(physicalHeight) scale=\(scale)\(emptyLabel)"
    )
  }
}

/// Runtime and map state owned by the main render loop.
@MainActor
final class MapState {
  private let runtime: RuntimeHandle
  private let map: MapHandle
  private var isClosed = false

  init(viewport: Viewport) async throws {
    precondition(
      !viewport.isEmpty,
      "cannot create MapState with an empty viewport"
    )
    let runtime = try await RuntimeHandle(
      options: RuntimeOptions(cachePath: ":memory:")
    )
    let map: MapHandle
    do {
      map = try await MapHandle(
        runtime: runtime,
        options: MapOptions(
          width: viewport.logicalWidth,
          height: viewport.logicalHeight,
          scaleFactor: viewport.scaleFactor,
          mode: .continuous
        )
      )
    } catch {
      try? await runtime.close()
      throw error
    }

    self.runtime = runtime
    self.map = map
    try map.setEventMask([.mapRenderUpdateAvailable, .mapRenderFrameFinished])
    try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
    _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 13.0,
      bearing: 12.0,
      pitch: 30.0
    )))
    _ = try map.requestRepaint()
  }

  var mapHandle: MapHandle {
    map
  }

  func scheduleEventDrains(
    onRenderRequested: @escaping @MainActor @Sendable () -> Void,
    onFailure: @escaping @MainActor @Sendable (Error) -> Void
  ) {
    runtime.setEventReadyHandler { [weak self] in
      Task { @MainActor in
        guard let self, !self.isClosed else { return }
        do {
          if try self.drainEvents() { onRenderRequested() }
        } catch {
          onFailure(error)
        }
      }
    }
  }

  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    runtime.setEventReadyHandler(nil)
    try await map.close()
    try await runtime.close()
  }

  private func drainEvents() throws -> Bool {
    var renderPending = false
    for event in try runtime.drainEvents().events
      where map.isSource(of: event)
    {
      switch event.type {
      case .mapRenderUpdateAvailable:
        renderPending = true
      case .mapRenderFrameFinished:
        if case let .renderFrame(frame) = event.payload, frame.needsRepaint {
          renderPending = true
        }
      default:
        break
      }
    }
    return renderPending
  }

  func resize(_ extent: MapLogicalExtent) throws {
    _ = try map.resize(to: extent)
  }

  func setGestureInProgress(_ inProgress: Bool) throws {
    _ = try map.updateCamera(CameraUpdate(
      camera: CameraOptions(),
      gesturePhase: inProgress ? .begin : .end
    ))
  }

  func cancelTransitions() throws {
    _ = try map.updateCamera(CameraUpdate(camera: CameraOptions()))
  }

  func moveBy(
    dx: Double,
    dy: Double,
    animation: AnimationOptions? = nil
  ) throws {
    _ = try map.moveBy(ScreenPoint(x: dx, y: dy), animation: animation)
  }

  func scaleBy(
    _ scale: Double,
    anchor: ScreenPoint,
    animation: AnimationOptions? = nil
  ) throws {
    _ = try map.scaleBy(scale, anchor: anchor, animation: animation)
  }

  func adjustBearing(
    delta: Double,
    animation: AnimationOptions? = nil
  ) throws {
    _ = try map.bearingBy(delta, animation: animation)
  }

  func adjustPitch(
    delta: Double,
    animation: AnimationOptions? = nil
  ) throws {
    _ = try map.pitchBy(delta, animation: animation)
  }

  func resetOrientation(animation: AnimationOptions) throws {
    _ = try map.updateCamera(CameraUpdate(
      mode: .ease,
      camera: CameraOptions(bearing: 0, pitch: 0),
      animation: animation
    ))
  }
}
