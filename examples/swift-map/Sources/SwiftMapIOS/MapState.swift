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
    let runtime = try RuntimeHandle(
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
    try await map.setEventMask([.mapRenderUpdateAvailable])
    _ = try await map.setStyleURL(
      "https://tiles.openfreemap.org/styles/bright"
    )
    _ = try await map.updateCamera(CameraUpdate(camera: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 13,
      bearing: 12,
      pitch: 30
    )))
    _ = try await map.requestRepaint()
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
    // Awaiting both release completions keeps process exit ordered after native
    // teardown.
    try await map.close()
    try await runtime.close()
  }

  private func drainEvents() throws -> Bool {
    var renderPending = false
    for event in try runtime.drainEvents() where map.isSource(of: event) {
      if event.type == .mapRenderUpdateAvailable {
        renderPending = true
      }
    }
    return renderPending
  }

  func resize(_ extent: MapLogicalExtent) async throws {
    _ = try await map.resize(to: extent)
  }

  func setGestureInProgress(_ inProgress: Bool) async throws {
    _ = try await map.updateCamera(CameraUpdate(
      camera: CameraOptions(),
      gesturePhase: inProgress ? .begin : .end
    ))
  }

  func cancelTransitions() async throws {
    _ = try await map.cancelTransitions()
  }

  func moveBy(dx: Double, dy: Double) async throws {
    _ = try await map.applyCameraDelta(
      CameraDelta(offset: ScreenPoint(x: dx, y: dy))
    )
  }

  func scaleBy(_ scale: Double, anchor: ScreenPoint) async throws {
    _ = try await map.applyCameraDelta(CameraDelta(
      kind: .scale,
      amount: scale,
      anchor: anchor
    ))
  }

  func adjustBearing(delta: Double, anchor: ScreenPoint) async throws {
    _ = try await map.applyCameraDelta(CameraDelta(
      kind: .bearing,
      amount: delta,
      anchor: anchor
    ))
  }

  func adjustPitch(delta: Double) async throws {
    _ = try await map.applyCameraDelta(CameraDelta(
      kind: .pitch,
      amount: delta
    ))
  }

  /// Eases to the next whole zoom level, `round(zoom) + 1`, about `anchor`.
  func zoomToNextStep(
    anchor: ScreenPoint,
    animation: AnimationOptions
  ) async throws {
    let zoom = try map.cameraSnapshot().camera.zoom ?? 0
    _ = try await map.applyCameraDelta(CameraDelta(
      kind: .scale,
      amount: pow(2.0, (zoom.rounded() + 1) - zoom),
      anchor: anchor,
      animation: animation
    ))
  }
}
