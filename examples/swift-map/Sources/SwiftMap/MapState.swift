import MaplibreNative
import QuartzCore

struct Viewport: Equatable {
  var logicalWidth: UInt32
  var logicalHeight: UInt32
  var physicalWidth: UInt32
  var physicalHeight: UInt32
  var scaleFactor: Double

  var extent: RenderTargetExtent {
    RenderTargetExtent(width: logicalWidth, height: logicalHeight, scaleFactor: scaleFactor)
  }
}

@MainActor
final class MapState {
  private nonisolated(unsafe) let runtime: RuntimeHandle
  nonisolated(unsafe) let map: MapHandle
  private nonisolated(unsafe) let renderSession: RenderSessionHandle

  init(viewport: Viewport, layer: CAMetalLayer) throws {
    let runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
    var createdMap: MapHandle?
    var createdRenderSession: RenderSessionHandle?
    var didInitialize = false
    defer {
      if !didInitialize {
        try? createdRenderSession?.close()
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
    let renderSession = try map.attachMetalSurface(MetalSurfaceDescriptor(
      extent: viewport.extent,
      layer: NativePointer(bitPattern: UInt(bitPattern: Unmanaged.passUnretained(layer).toOpaque()))
    ))
    createdRenderSession = renderSession

    self.runtime = runtime
    self.map = map
    self.renderSession = renderSession
    didInitialize = true
  }

  func resize(_ viewport: Viewport) throws {
    try renderSession.resize(width: viewport.logicalWidth, height: viewport.logicalHeight, scaleFactor: viewport.scaleFactor)
  }

  func close() throws {
    var firstError: Error?
    do {
      try renderSession.close()
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

  func runOnce() {
    try? runtime.runOnce()
  }

  func drainEvents() throws -> Bool {
    var renderUpdateAvailable = false
    while let event = try runtime.pollEvent() {
      if event.type == .mapRenderUpdateAvailable {
        renderUpdateAvailable = true
      }
    }
    return renderUpdateAvailable
  }

  func render() throws -> Bool {
    do {
      try renderSession.renderUpdate()
      return true
    } catch let error as MaplibreError where error.kind == .invalidState {
      return false
    }
  }
}
