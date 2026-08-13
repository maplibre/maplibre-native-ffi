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

/// Runtime and map, owned for their whole lifetime by the runtime loop thread.
/// The render loop thread owns the view, the Metal objects, and the session.
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
    // The two event types the runtime loop reads. A map queues no event of an
    // unselected type, so this runs before the style load.
    try map.setEventMask([.mapRenderUpdateAvailable, .mapRenderFrameFinished])
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
  /// `MapHandle` itself stays on this thread.
  func attachRef() throws -> MapAttachRef {
    try map.attachRef()
  }

  /// Closes the map and then the runtime. The render session must already be
  /// closed; a map with an attached session cannot be destroyed.
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

  /// Pumps the runtime, parking up to `timeout` when there is nothing to do.
  func pump(timeout: TimeInterval) throws {
    try runtime.pump(timeout: timeout)
  }

  /// Acquires the wake source the render loop uses to release this loop's park.
  func wakeSource() throws -> WakeSource {
    try runtime.wakeSource()
  }

  /// Drains one batch of runtime events, reporting whether the map wants
  /// another frame.
  func drainEvents() throws -> Bool {
    var renderPending = false
    // One drain takes every event the pump produced.
    for event in try runtime.drainEvents().events {
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

  /// Applies one decoded camera command on the map's owner thread, where
  /// read-modify-write commands also read the current camera.
  func apply(_ command: CameraCommand) throws {
    switch command {
    case .cancelTransitions:
      try map.cancelTransitions()
    case let .setGestureInProgress(inProgress):
      try map.setGestureInProgress(inProgress)
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
