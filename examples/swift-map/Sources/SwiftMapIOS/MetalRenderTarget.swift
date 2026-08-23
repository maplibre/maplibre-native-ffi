import MaplibreNativeFFI
import Metal
import QuartzCore

@MainActor
final class MetalGraphicsContext {
  let device: any MTLDevice
  let layer: CAMetalLayer

  init(layer: CAMetalLayer) throws {
    guard let device = MTLCreateSystemDefaultDevice() else {
      throw metalError("MTLCreateSystemDefaultDevice returned nil")
    }
    self.device = device
    self.layer = layer
    configureLayer()
  }

  var contextDescriptor: MetalContextDescriptor {
    MetalContextDescriptor(device: nativePointer(device as AnyObject))
  }

  var layerPointer: NativePointer {
    nativePointer(layer)
  }

  func resize(_ viewport: Viewport) {
    guard !viewport.isEmpty else { return }
    layer.contentsScale = viewport.scaleFactor
    layer.drawableSize = CGSize(
      width: Int(viewport.physicalWidth),
      height: Int(viewport.physicalHeight)
    )
  }

  private func configureLayer() {
    layer.device = device
    layer.pixelFormat = .bgra8Unorm
    layer.framebufferOnly = false
  }
}

/// The render session for the host `CAMetalLayer`. The display link services
/// driver work on the graphics thread. Every operation remains isolated to the
/// main actor.
@MainActor
final class MetalRenderTarget {
  private let session: RenderSessionHandle

  private init(session: RenderSessionHandle) {
    self.session = session
  }

  /// Attaches a session against the map owned by the view.
  static func attach(
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws -> MetalRenderTarget {
    let attachment = try map.attachMetalSurface(
      MetalSurfaceDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor,
        layer: graphics.layerPointer
      ),
      options: .init(driver: .callerGraphicsThread)
    )
    let session = attachment.session
    do {
      try session.setDriverWorkReadyHandler { [weak session] in
        Task { @MainActor in
          _ = try? session?.serviceDriverWork(maxWork: 0)
        }
      }
      try session.serviceDriverWork(maxWork: 0)
      try await attachment.completion.value
      return MetalRenderTarget(session: session)
    } catch {
      _ = try? session.abandon()
      try? session.close()
      throw error
    }
  }

  func resize(_ viewport: Viewport) async throws {
    try await session.resize(viewport.extent)
  }

  /// Services graphics work and submits one display-link-paced frame demand.
  func renderFrame() throws -> Bool {
    try session.requestFrame(FrameDemand(options: [.ifNeeded, .present]))
    try session.serviceDriverWork()
    let results = try session.drainFrameResults()
    guard let result = results.last else { return false }
    return result.result != .sizePending &&
      result.result != .targetNotReady
  }

  func close() async throws {
    do {
      try await session.detach()
      try session.close()
    } catch {
      abandon()
      throw error
    }
  }

  func abandon() {
    _ = try? session.abandon()
    try? session.close()
  }
}

private func nativePointer(_ object: AnyObject) -> NativePointer {
  NativePointer(
    bitPattern: UInt(bitPattern: Unmanaged.passUnretained(object).toOpaque())
  )
}

private func metalError(_ message: String) -> MaplibreError {
  MaplibreError(kind: .nativeError, rawStatus: nil, diagnostic: message)
}
