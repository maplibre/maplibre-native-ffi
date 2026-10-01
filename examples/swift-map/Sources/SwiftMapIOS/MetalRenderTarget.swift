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

/// What one drain of the frame-result queue found.
struct FrameResults {
  /// A demand rendered and presented a frame.
  var rendered = false
  /// The map asked for another frame while it rendered one.
  var needsRepaint = false
  /// The target could not produce a frame, so the loop retries later.
  var targetNotReady = false
}

/// The render session for the host `CAMetalLayer`. A driver wake services
/// caller-driver work on the main thread that owns the Metal objects.
@MainActor
final class MetalRenderTarget {
  private let session: RenderSessionHandle
  private let driverRelay: DriverRelay
  private var nextToken: UInt64 = 0

  private init(session: RenderSessionHandle, driverRelay: DriverRelay) {
    self.session = session
    self.driverRelay = driverRelay
  }

  /// Receives a failure to service driver work. The session is abandoned by
  /// then, so it renders nothing more.
  var onFailure: (@MainActor (Error) -> Void)? {
    get { driverRelay.onFailure }
    set { driverRelay.onFailure = newValue }
  }

  /// Attaches a session against the map. `frameWake` reports frame results.
  static func attach(
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport,
    frameWake: Wake
  ) async throws -> MetalRenderTarget {
    let driverRelay = DriverRelay()
    let attachment = try map.metalSurfaceAttach(
      descriptor: MetalSurfaceDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor,
        layer: graphics.layerPointer
      ),
      options: .init(
        driver: .callerGraphicsThread,
        frameWake: frameWake,
        driverWorkWake: driverRelay.wake
      )
    )
    let session = attachment.session
    do {
      driverRelay.session = session
      driverRelay.service()
      try await attachment.completion.value
      return MetalRenderTarget(session: session, driverRelay: driverRelay)
    } catch {
      _ = try? session.abandon()
      try? session.close()
      // A service failure abandons the session, which fails the attachment.
      throw driverRelay.failure ?? error
    }
  }

  /// Carries a new viewport to the session, which carries the extent to the
  /// map.
  func resize(_ viewport: Viewport) async throws {
    try await session.resize(extent: viewport.extent)
  }

  /// Demands a presented frame. A forced demand renders even without a newer
  /// map update, which a retry after an undrawn frame needs.
  func requestFrame(force: Bool = false) throws {
    nextToken += 1
    try session.requestFrame(demand: FrameDemand(
      flags: force ? [.present] : [.ifNeeded, .present],
      token: nextToken
    ))
  }

  /// Drains every queued frame result.
  func drainResults() throws -> FrameResults {
    let batch: RenderFrameBatchHandle
    do { batch = try session.drainFrameResults() }
    catch let error as MaplibreError
      where error.kind == .notReady { return FrameResults() }
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

/// Services caller-driver work on the main actor when the driver wakes. A
/// session that cannot be serviced is abandoned, which completes its pending
/// work with target loss, and the failure is reported.
@MainActor
private final class DriverRelay {
  weak var session: RenderSessionHandle?
  var onFailure: (@MainActor (Error) -> Void)?
  private(set) var failure: Error?

  var wake: Wake {
    Wake(callback: { [self] in Task { @MainActor in service() } })
  }

  func service() {
    guard failure == nil, let session else { return }
    do {
      _ = try session.serviceDriverWork(maxWork: 0)
    } catch {
      failure = error
      _ = try? session.abandon()
      onFailure?(error)
    }
  }
}
