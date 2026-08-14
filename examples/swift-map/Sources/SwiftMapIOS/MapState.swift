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

/// A camera change decoded and applied on the main render loop.
enum CameraCommand {
  case resize(MapLogicalExtent)
  case setGestureInProgress(Bool)
  case moveBy(dx: Double, dy: Double)
  case scaleBy(scale: Double, anchor: ScreenPoint)
  /// Adds `delta` degrees to the current bearing, pivoting on `anchor`.
  case adjustBearing(delta: Double, anchor: ScreenPoint)
  /// Adds `delta` degrees to the current pitch, clamped to `[0, 60]`.
  case adjustPitch(delta: Double)
  /// Zooms to `round(zoom) + 1` about `anchor`, the double-tap step.
  case zoomToNextStep(anchor: ScreenPoint, animation: AnimationOptions)
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
      zoom: 13,
      bearing: 12,
      pitch: 30
    )))
    _ = try map.requestRepaint()
    try await runtime.barrier()
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

  func apply(_ command: CameraCommand) async throws {
    switch command {
    case let .resize(extent):
      _ = try map.resize(to: extent)
    case let .setGestureInProgress(inProgress):
      _ = try map.updateCamera(CameraUpdate(
        camera: CameraOptions(),
        gesturePhase: inProgress ? .begin : .end,
        gestureId: 1
      ))
    case let .moveBy(dx, dy):
      let current = try await map.queryCamera().camera
      guard let center = current.center else { return }
      let point = try await map.pixel(for: center)
      let moved = try await map.latLng(for: ScreenPoint(
        x: point.x - dx,
        y: point.y - dy
      ))
      _ = try map
        .updateCamera(CameraUpdate(camera: CameraOptions(center: moved)))
    case let .scaleBy(scale, anchor):
      let current = try await map.queryCamera().camera
      _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(
        zoom: (current.zoom ?? 0) + log2(scale),
        anchor: anchor
      )))
    case let .adjustBearing(delta, anchor):
      let current = try await map.queryCamera().camera
      _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(
        bearing: (current.bearing ?? 0) + delta,
        anchor: anchor
      )))
    case let .adjustPitch(delta):
      let current = try await map.queryCamera().camera
      _ = try map.updateCamera(CameraUpdate(camera: CameraOptions(
        pitch: clampedPitch((current.pitch ?? 0) + delta)
      )))
    case let .zoomToNextStep(anchor, animation):
      let current = try await map.queryCamera().camera
      let zoom = current.zoom ?? 0
      _ = try map.updateCamera(CameraUpdate(
        mode: .ease,
        camera: CameraOptions(zoom: round(zoom) + 1, anchor: anchor),
        animation: animation
      ))
    }
  }
}

private func clampedPitch(_ pitch: Double) -> Double {
  min(max(pitch, 0.0), 60.0)
}
