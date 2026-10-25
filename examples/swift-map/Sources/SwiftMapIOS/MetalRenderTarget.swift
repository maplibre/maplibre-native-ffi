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
    MetalContextDescriptor(device: device)
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
    let created = try map.attachMetalSurface(
      descriptor: MetalSurfaceDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor,
        layer: graphics.layer
      ),
      options: .init(driver: .coreWorker, frameWake: frameWake)
    )
    let session = created.session
    do {
      try await created.ready.value
      return MetalRenderTarget(session: session)
    } catch {
      _ = try? session.abandon()
      try? session.dispose()
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
      flags: force ? [.force, .present] : [.waitForUpdate, .present],
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
    var results = FrameResults()
    for result in try session.drainFrameResults() {
      if result.disposition == .rendered {
        results.rendered = true
      } else if result.disposition == .targetNotReady {
        results.targetNotReady = true
      }
    }
    return results
  }

  func close() async throws {
    do {
      try await session.detach()
      try await session.close()
    } catch {
      abandon()
      throw error
    }
  }

  func abandon() {
    _ = try? session.abandon()
    try? session.dispose()
  }
}

private func metalError(_ message: String) -> MaplibreError {
  MaplibreError(status: .nativeError, diagnostic: message)
}
