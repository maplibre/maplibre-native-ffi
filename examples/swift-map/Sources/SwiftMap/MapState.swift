import Foundation
import MaplibreNative

struct Viewport: Equatable {
  var logicalWidth: UInt32
  var logicalHeight: UInt32
  var physicalWidth: UInt32
  var physicalHeight: UInt32
  var scaleFactor: Double
  var isEmpty: Bool

  var extent: RenderTargetExtent {
    RenderTargetExtent(width: logicalWidth, height: logicalHeight, scaleFactor: scaleFactor)
  }

  func log(_ label: String) {
    let scale = String(format: "%.2f", scaleFactor)
    let emptyLabel = isEmpty ? " empty=true" : ""
    print(
      "\(label): logical=\(logicalWidth)x\(logicalHeight) physical=\(physicalWidth)x\(physicalHeight) scale=\(scale)\(emptyLabel)"
    )
  }
}

@MainActor
final class MapState {
  private nonisolated(unsafe) let runtime: RuntimeHandle
  nonisolated(unsafe) let map: MapHandle
  private var renderTarget: MetalRenderTarget?
  private let mode: RenderTargetMode
  private var isClosed = false

  init(mode: RenderTargetMode, viewport: Viewport, graphics: MetalGraphicsContext) throws {
    precondition(!viewport.isEmpty, "cannot create MapState with an empty viewport")
    let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
    var createdMap: MapHandle?
    var createdRenderTarget: MetalRenderTarget?
    var didInitialize = false
    defer {
      if !didInitialize {
        try? createdRenderTarget?.close()
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
    let renderTarget = try MetalRenderTarget.attach(
      mode: mode,
      map: map,
      graphics: graphics,
      viewport: viewport
    )
    createdRenderTarget = renderTarget

    self.runtime = runtime
    self.map = map
    self.renderTarget = renderTarget
    self.mode = mode
    didInitialize = true
  }

  func resize(_ viewport: Viewport, graphics: MetalGraphicsContext) throws {
    guard !viewport.isEmpty else { return }
    guard let renderTarget else { return }
    if renderTarget.needsReattachOnResize {
      try renderTarget.close()
      self.renderTarget = nil
      self.renderTarget = try MetalRenderTarget.attach(
        mode: mode,
        map: map,
        graphics: graphics,
        viewport: viewport
      )
    } else {
      try renderTarget.resize(viewport)
    }
    try map.requestRepaint()
  }

  func close() throws {
    guard !isClosed else { return }
    isClosed = true
    var firstError: Error?
    do {
      try renderTarget?.close()
      renderTarget = nil
    } catch {
      firstError = firstError ?? error
    }
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

  func drainEvents() throws -> Bool {
    var renderPending = false
    while let event = try runtime.pollEvent() {
      guard map.isSource(of: event) else { continue }
      switch event.type {
      case .mapRenderUpdateAvailable:
        renderPending = true
      case .mapRenderFrameFinished:
        if case .renderFrame(let frame) = event.payload, frame.needsRepaint {
          renderPending = true
        }
      default:
        break
      }
    }
    return renderPending
  }

  func finishFrame() throws {
    // Metal surface and texture paths do not need per-tick host upkeep here.
  }

  func render() throws -> Bool {
    guard let renderTarget else { return false }
    do {
      try renderTarget.renderUpdate()
      return true
    } catch let error as MaplibreError where error.kind == .invalidState {
      return false
    }
  }
}
