import Foundation
import MaplibreNativeFFI
import os

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
    let logger = Logger(
      subsystem: "org.maplibre.nativeffi.examples.swift-map-ios",
      category: "Viewport"
    )
    let scale = String(format: "%.2f", scaleFactor)
    logger.info(
      "\(label, privacy: .public): logical=\(logicalWidth)x\(logicalHeight) physical=\(physicalWidth)x\(physicalHeight) scale=\(scale, privacy: .public) empty=\(isEmpty)"
    )
  }
}

/// The runtime and map, driven by the native scheduler thread the runtime
/// owns. Camera calls submit commands without waiting for their completions.
@MainActor
final class MapState {
  private let runtime: RuntimeHandle
  let map: MapHandle
  private var isClosed = false
  /// Reports a command that failed after native code accepted it.
  var onFailure: (@MainActor (Error) -> Void)?

  /// Creates the map. `eventWake` reports queued runtime events.
  init(viewport: Viewport, eventWake: Wake) async throws {
    precondition(
      !viewport.isEmpty,
      "cannot create MapState with an empty viewport"
    )
    let runtime = try Maplibre.runtimeCreate(
      options: RuntimeOptions(cachePath: ":memory:", eventWake: eventWake)
    )
    let map: MapHandle
    do {
      map = try await runtime.mapCreate(options: MapOptions(
        initialExtent: LogicalExtent(
          width: viewport.logicalWidth,
          height: viewport.logicalHeight,
          scaleFactor: viewport.scaleFactor
        ),
        mapMode: .continuous,
        eventMask: [.mapRenderUpdateAvailable]
      ))
    } catch {
      try? await runtime.close()
      throw error
    }

    self.runtime = runtime
    self.map = map
    _ = try await map.setStyleUrl(url:
      "https://tiles.openfreemap.org/styles/bright")
    _ = try await map.updateCamera(update: CameraUpdate(camera: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 13,
      bearing: 12,
      pitch: 30
    )))
  }

  func close() async throws {
    guard !isClosed else { return }
    isClosed = true
    // Awaiting both release completions lets native teardown finish before the
    // app tears down state that the callbacks use.
    try await map.close()
    try await runtime.close()
  }

  /// Drains every queued runtime event and reports whether the map published
  /// a render update.
  func drainEvents() throws -> Bool {
    guard !isClosed else { return false }
    let batch = try runtime.drainEvents()
    defer { try? batch.close() }
    return try batch.get().events.contains {
      $0.sourceType == .map && $0.source == map.id &&
        $0.type == .mapRenderUpdateAvailable
    }
  }

  func setGestureInProgress(_ inProgress: Bool) {
    submit { [map] in _ = try await map.updateCamera(update: CameraUpdate(
      camera: CameraOptions(),
      gesturePhase: inProgress ? .begin : .end
    )) }
  }

  func cancelTransitions() {
    submit { [map] in _ = try await map.cancelTransitions() }
  }

  func moveBy(dx: Double, dy: Double) {
    submit { [map] in _ = try await map.applyCameraDelta(delta:
      CameraDelta(offset: ScreenPoint(x: dx, y: dy))) }
  }

  func scaleBy(_ scale: Double, anchor: ScreenPoint) {
    submit { [map] in _ = try await map.applyCameraDelta(delta: CameraDelta(
      scale: scale,
      anchor: anchor
    )) }
  }

  func adjustBearing(delta: Double, anchor: ScreenPoint) {
    submit { [map] in _ = try await map.applyCameraDelta(delta: CameraDelta(
      bearing: delta,
      anchor: anchor
    )) }
  }

  func adjustPitch(delta: Double) {
    submit { [map] in _ = try await map.applyCameraDelta(delta: CameraDelta(
      pitch: delta
    )) }
  }

  /// Eases to the next whole zoom level, `round(zoom) + 1`, about `anchor`,
  /// from the zoom of the latest published camera snapshot.
  func zoomToNextStep(anchor: ScreenPoint, animation: AnimationOptions) {
    submit { [map] in
      let zoom = try map.cameraSnapshotGet().camera.zoom ?? 0
      _ = try await map.applyCameraDelta(delta: CameraDelta(
        scale: pow(2.0, (zoom.rounded() + 1) - zoom),
        anchor: anchor,
        animation: animation
      ))
    }
  }

  /// Starts `command` in a task on the main actor. The main actor runs its
  /// tasks in the order that it enqueued them, and a generated operation
  /// submits to native before it first suspends, so a gesture's begin, deltas,
  /// and end reach native in input order. A command that starts after the
  /// map closes does nothing.
  func submit(
    _ command: @escaping @MainActor @Sendable () async throws -> Void
  ) {
    guard !isClosed else { return }
    Task { @MainActor [weak self] in
      guard let self, !self.isClosed else { return }
      do { try await command() }
      catch { onFailure?(error) }
    }
  }
}
