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

/// The render session for the host `CAMetalLayer`.
///
/// Everything here belongs to the render loop thread. Attach records the
/// calling thread as the session's owner for the session's lifetime, so this
/// type is created, resized, rendered, and closed on the one thread that owns
/// the view and its layer.
@MainActor
final class MetalRenderTarget {
  private let session: RenderSessionHandle

  private init(session: RenderSessionHandle) {
    self.session = session
  }

  /// Attaches a session against the map the runtime loop published.
  ///
  /// The attach reference is the only part of the map that crosses threads; the
  /// map handle itself stays on the runtime loop.
  static func attach(
    attachRef: MapAttachRef,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) throws -> MetalRenderTarget {
    let session = try attachRef.attachMetalSurface(MetalSurfaceDescriptor(
      extent: viewport.extent,
      context: graphics.contextDescriptor,
      layer: graphics.layerPointer
    ))
    return MetalRenderTarget(session: session)
  }

  func resize(_ viewport: Viewport) throws {
    try session.resize(
      width: viewport.logicalWidth,
      height: viewport.logicalHeight,
      scaleFactor: viewport.scaleFactor
    )
  }

  /// Renders the latest map update, reporting whether a frame was drawn.
  ///
  /// The map applies a new logical size on the runtime loop's next `pump`,
  /// so this reports no update for a few iterations after attach or resize.
  /// That is normal: the render loop keeps pacing and asks again.
  func renderUpdate() throws -> Bool {
    try session.renderUpdate()
  }

  func finishFrame() throws {
    // The Metal surface path needs no per-iteration host upkeep here.
  }

  func close() throws {
    try session.close()
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
