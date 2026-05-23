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
  nonisolated(unsafe) private(set) var runtime: RuntimeHandle
  nonisolated(unsafe) private(set) var map: MapHandle
  nonisolated(unsafe) private(set) var renderSession: RenderSessionHandle

  init(viewport: Viewport, layer: CAMetalLayer) throws {
    runtime = try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
    do {
      map = try MapHandle(
        runtime: runtime,
        options: MapOptions(
          width: viewport.logicalWidth,
          height: viewport.logicalHeight,
          scaleFactor: viewport.scaleFactor,
          mode: .continuous
        )
      )
      try map.setStyleURL("https://tiles.openfreemap.org/styles/bright")
      try map.jump(to: CameraOptions(
        center: LatLng(latitude: 37.7749, longitude: -122.4194),
        zoom: 13.0,
        bearing: 12.0,
        pitch: 30.0
      ))
      renderSession = try map.attachMetalSurface(MetalSurfaceDescriptor(
        extent: viewport.extent,
        layer: NativePointer(bitPattern: UInt(bitPattern: Unmanaged.passUnretained(layer).toOpaque()))
      ))
    } catch {
      try? runtime.close()
      throw error
    }
  }

  deinit {
    try? renderSession.close()
    try? map.close()
    try? runtime.close()
  }

  func resize(_ viewport: Viewport) throws {
    try renderSession.resize(width: viewport.logicalWidth, height: viewport.logicalHeight, scaleFactor: viewport.scaleFactor)
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
