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

  /// Follows a new viewport. The session sets the layer's drawable size, so
  /// the host sets only the contents scale.
  func resize(_ viewport: Viewport) {
    guard !viewport.isEmpty else { return }
    layer.contentsScale = viewport.scaleFactor
  }

  private func configureLayer() {
    layer.device = device
    layer.pixelFormat = .bgra8Unorm
    layer.framebufferOnly = false
  }
}

/// What one drain of the frame-result queue found.
struct FrameResults {
  /// A demand rendered and presented a frame.
  var rendered = false
  /// The map asked for another frame while it rendered one.
  var needsRepaint = false
  /// The target could not produce a frame, so the loop retries later.
  var targetNotReady = false
}

/// The render session for the host `CAMetalLayer`. A Metal surface accepts a
/// core worker, which renders and presents on its own thread, so the host runs
/// nothing for the session.
@MainActor
final class MetalRenderTarget {
  private let session: RenderSessionHandle
  let driver = RenderDriverKind.coreWorker
  private var nextToken: UInt64 = 0

  private init(session: RenderSessionHandle) {
    self.session = session
  }

  /// Attaches a session against the map. `frameWake` reports frame results.
  static func attach(
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport,
    frameWake: Wake
  ) async throws -> MetalRenderTarget {
    let attachment = try map.metalSurfaceAttach(
      descriptor: MetalSurfaceDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor,
        layer: graphics.layerPointer
      ),
      options: .init(driver: .coreWorker, frameWake: frameWake)
    )
    let session = attachment.session
    do {
      try await attachment.completion.value
      return MetalRenderTarget(session: session)
    } catch {
      _ = try? session.abandon()
      try? session.close()
      throw error
    }
  }

  /// Carries a new viewport to the session, which carries the extent to the
  /// map.
  func resize(_ viewport: Viewport) async throws {
    try await session.resize(extent: viewport.extent)
  }

  /// Demands a presented frame. A forced demand renders even without a newer
  /// map update.
  func requestFrame(force: Bool = false) throws {
    nextToken += 1
    try session.requestFrame(demand: FrameDemand(
      flags: force ? [.present] : [.ifNeeded, .present],
      token: nextToken
    ))
  }

  /// Waits until every demand accepted so far has its result, so no frame
  /// renders after the app enters the background.
  func barrier() async throws {
    try await session.barrier()
  }

  /// Drains every queued frame result.
  func drainResults() throws -> FrameResults {
    guard let batch = try session.drainFrameResults() else {
      return FrameResults()
    }
    defer { try? batch.close() }
    var results = FrameResults()
    // No update and size pending wait for the map's next update, superseded
    // demands have a newer one behind them, and no demand carries a timeout.
    for index in try 0 ..< (batch.count()) {
      let result = try batch.get(index: index)
      if result.disposition == .rendered {
        results.rendered = true
        results.needsRepaint = result.needsRepaint
      } else if result.disposition == .targetNotReady {
        results.targetNotReady = true
      }
    }
    return results
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
