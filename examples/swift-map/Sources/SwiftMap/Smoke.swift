import Foundation
import MaplibreNativeFFI
import QuartzCore

/// A style with only a background layer, so the smoke run needs no network.
private let smokeStyleJSON = Data(##"""
{"version":8,"sources":{},"layers":[{"id":"background","type":"background",
"paint":{"background-color":"#2a6f97"}}]}
"""##.utf8)

/// Renders through the same render loop the window uses, with a Metal layer
/// that no window shows, until one frame reaches the layer, then shuts down.
/// Returns the process exit status.
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
    let loop = try await RenderLoop.start(
      mode: mode,
      graphics: graphics,
      viewport: viewport,
      styleJSON: smokeStyleJSON
    )
    let rendered = await firstFrame(of: loop, within: .seconds(60))
    try await loop.close()
    guard rendered else {
      print("smoke: no \(mode.rawValue) frame rendered before the deadline")
      return 1
    }
    print("smoke: rendered a frame")
    return 0
  } catch {
    print("smoke: \(mode.rawValue) failed: \(error)")
    return 1
  }
}

/// Waits for the loop's first presented frame, reporting false on failure or
/// when `timeout` passes first.
@MainActor
private func firstFrame(
  of loop: RenderLoop,
  within timeout: Duration
) async -> Bool {
  await withCheckedContinuation { continuation in
    let outcome = SmokeOutcome(continuation)
    loop.onPresented = { outcome.finish(true) }
    loop.onFailure = {
      print("smoke: render loop failed: \($0)")
      outcome.finish(false)
    }
    Task { @MainActor in
      try? await Task.sleep(for: timeout)
      outcome.finish(false)
    }
  }
}

/// Resumes the smoke wait once, with whichever outcome arrives first.
@MainActor
private final class SmokeOutcome {
  private var continuation: CheckedContinuation<Bool, Never>?

  init(_ continuation: CheckedContinuation<Bool, Never>) {
    self.continuation = continuation
  }

  func finish(_ rendered: Bool) {
    continuation?.resume(returning: rendered)
    continuation = nil
  }
}
