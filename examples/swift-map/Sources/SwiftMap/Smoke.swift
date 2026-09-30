import Foundation
import MaplibreNativeFFI
import QuartzCore

/// A style with only a background layer, so the smoke run needs no network.
private let smokeStyleJSON = Data(##"""
{"version":8,"sources":{},"layers":[{"id":"background","type":"background",
"paint":{"background-color":"#2a6f97"}}]}
"""##.utf8)

/// Renders frames headless through the same render target the window uses,
/// with a Metal layer that no window shows, until one frame reaches the layer,
/// then shuts down. Returns the process exit status.
@MainActor
func runSmoke(mode: RenderTargetMode) async -> Int32 {
  let viewport = Viewport(
    logicalWidth: 256,
    logicalHeight: 256,
    physicalWidth: 256,
    physicalHeight: 256,
    scaleFactor: 1,
    isEmpty: false
  )
  do {
    let graphics = try MetalGraphicsContext(layer: CAMetalLayer())
    graphics.resize(viewport)
    let state = try await MapState(
      viewport: viewport,
      styleJSON: smokeStyleJSON
    )
    let target: MetalRenderTarget
    do {
      target = try await MetalRenderTarget.attach(
        mode: mode,
        map: state.mapHandle,
        graphics: graphics,
        viewport: viewport
      )
    } catch {
      try? await state.close()
      throw error
    }
    let deadline = Date().addingTimeInterval(60)
    var rendered = false
    while !rendered, Date() < deadline {
      rendered = try await target.renderFrame()
      if !rendered {
        // The window's loop runs at the display rate, and so does this one.
        try await Task.sleep(for: .milliseconds(16))
      }
    }
    try await target.close()
    try await state.close()
    guard rendered else {
      print("smoke: no \(mode.rawValue) frame rendered before the deadline")
      return 1
    }
    print("smoke: rendered one \(mode.rawValue) frame")
    return 0
  } catch {
    print("smoke: \(mode.rawValue) failed: \(error)")
    return 1
  }
}
