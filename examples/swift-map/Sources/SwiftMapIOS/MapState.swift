import Foundation
import MaplibreNative
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

/// Runtime and map, owned for their whole lifetime by the runtime loop thread.
///
/// The render target is not here: it belongs to the render loop thread, which
/// owns the view, the Metal objects, and the render session.
final class MapState {
  private let runtime: RuntimeHandle
  private let map: MapHandle
  private var isClosed = false

  init(viewport: Viewport) throws {
    precondition(
      !viewport.isEmpty,
      "cannot create MapState with an empty viewport"
    )
    let runtime =
      try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
    var createdMap: MapHandle?
    var didInitialize = false
    defer {
      if !didInitialize {
        try? createdMap?.close()
        try? runtime.close()
      }
    }

    let map = try MapHandle(
      runtime: runtime,
      options: MapOptions(
        width: viewport.logicalWidth,
        height: viewport.logicalHeight,
        scaleFactor: viewport.scaleFactor,
        mode: .continuous
      )
    )
    createdMap = map
    try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
    try map.jump(to: CameraOptions(
      center: LatLng(latitude: 37.7749, longitude: -122.4194),
      zoom: 13.0,
      bearing: 12.0,
      pitch: 30.0
    ))
    try map.requestRepaint()

    self.runtime = runtime
    self.map = map
    didInitialize = true
  }

  /// The `Sendable` reference the render loop attaches its own session against.
  ///
  /// `MapHandle` stays on this thread; the reference is the only part of the
  /// map that crosses.
  func attachRef() throws -> MapAttachRef {
    try map.attachRef()
  }

  /// Closes the map and then the runtime.
  ///
  /// The render loop closes the render session before it asks this loop to
  /// stop, because native refuses to destroy a map that still has a session
  /// attached.
  func close() throws {
    guard !isClosed else { return }
    isClosed = true
    var firstError: Error?
    do {
      try map.close()
    } catch {
      firstError = firstError ?? error
    }
    do {
      try runtime.close()
    } catch {
      firstError = firstError ?? error
    }
    if let firstError {
      throw firstError
    }
  }

  func runOnce() throws {
    try runtime.runOnce()
  }

  /// Drains runtime events, reporting whether the map wants another frame.
  func drainEvents() throws -> Bool {
    var renderPending = false
    while let event = try runtime.pollEvent() {
      guard map.isSource(of: event) else { continue }
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

  /// Applies one decoded camera command.
  ///
  /// This runs on the map's owner thread, which is why the read-modify-write
  /// commands read the current camera here rather than on the render loop that
  /// produced them.
  func apply(_ command: CameraCommand) throws {
    switch command {
    case .cancelTransitions:
      try map.cancelTransitions()
    case let .moveBy(dx, dy):
      try map.moveBy(deltaX: dx, deltaY: dy)
    case let .scaleBy(scale, anchor):
      try map.scaleBy(scale, anchor: anchor)
    case let .adjustBearing(delta, anchor):
      let camera = try map.camera()
      try map.jump(to: CameraOptions(
        bearing: (camera.bearing ?? 0) + delta,
        anchor: anchor
      ))
    case let .adjustPitch(delta):
      let camera = try map.camera()
      try map.jump(
        to: CameraOptions(pitch: clampedPitch((camera.pitch ?? 0) + delta))
      )
    case let .zoomToNextStep(anchor, animation):
      let camera = try map.camera()
      let zoom = camera.zoom ?? 0
      let targetZoom = round(zoom) + 1.0
      try map.scaleBy(
        pow(2.0, targetZoom - zoom),
        anchor: anchor,
        animation: animation
      )
    }
  }
}

private func clampedPitch(_ pitch: Double) -> Double {
  min(max(pitch, 0.0), 60.0)
}
