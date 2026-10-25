import Foundation
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

  /// Follows a new viewport. A surface session sets the layer's drawable
  /// size, and the compositor sizes it to each frame it draws, so the host
  /// sets only the contents scale.
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
  /// A demand rendered a frame.
  var rendered = false
  /// The target could not produce a frame, so the loop retries later.
  var targetNotReady = false
}

/// The render session and its mode-specific resources. Every Metal target
/// accepts a core worker, which renders on its own thread, so the host runs
/// nothing for the session.
@MainActor
final class MetalRenderTarget {
  private enum Kind {
    case ownedTexture(MetalTextureCompositor)
    case borrowedTexture(MetalTextureCompositor)
    case nativeSurface
  }

  /// The depth of a borrowed ring. The host holds the newest frame until a
  /// newer one arrives, and the session renders into the other texture
  /// meanwhile.
  static let borrowedRingDepth = 2

  let session: RenderSessionHandle
  let driver = RenderDriverKind.coreWorker
  private let kind: Kind
  /// The caller-owned ring the session renders into.
  private var borrowedRing: [MetalBorrowedTexture]
  /// The newest texture frame, held until a newer one replaces it.
  private var heldFrame: AcquiredFrameHandle?
  private var nextToken: UInt64 = 0

  private init(
    session: RenderSessionHandle,
    kind: Kind,
    borrowedRing: [MetalBorrowedTexture] = []
  ) {
    self.session = session
    self.kind = kind
    self.borrowedRing = borrowedRing
  }

  /// Attaches a session against the map. `frameWake` reports frame results.
  static func attach(
    mode: RenderTargetMode,
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport,
    frameWake: Wake
  ) async throws -> MetalRenderTarget {
    let options = RenderSessionAttachOptions(
      driver: .coreWorker,
      requestedTextureRingDepth: mode == .ownedTexture ? 3 : 0,
      frameWake: frameWake
    )
    switch mode {
    case .ownedTexture:
      let session = try await finishAttachment(
        map.attachMetalOwnedTexture(
          descriptor: MetalOwnedTextureDescriptor(
            extent: viewport.extent,
            context: graphics.contextDescriptor,
            pixelFormat: .bgra8Unorm
          ),
          options: options
        )
      )
      return try withCompositor(session, graphics: graphics) {
        MetalRenderTarget(session: session, kind: .ownedTexture($0))
      }
    case .borrowedTexture:
      let ring = try MetalBorrowedTexture.ring(
        graphics: graphics,
        viewport: viewport
      )
      let session = try await finishAttachment(
        map.attachMetalBorrowedTexture(
          descriptor: MetalBorrowedTexture.descriptor(ring, viewport),
          options: options
        )
      )
      return try withCompositor(session, graphics: graphics) {
        MetalRenderTarget(
          session: session,
          kind: .borrowedTexture($0),
          borrowedRing: ring
        )
      }
    case .nativeSurface:
      let session = try await finishAttachment(
        map.attachMetalSurface(
          descriptor: MetalSurfaceDescriptor(
            extent: viewport.extent,
            context: graphics.contextDescriptor,
            layer: graphics.layer
          ),
          options: options
        )
      )
      return MetalRenderTarget(session: session, kind: .nativeSurface)
    }
  }

  /// Demands a frame. A forced demand renders even without a newer map
  /// update, which a retry after a frame that missed the layer needs.
  func requestFrame(force: Bool = false) throws {
    nextToken += 1
    var flags: FrameDemandFlag = force ? [] : [.ifNeeded, .waitForUpdate]
    if case .nativeSurface = kind {
      flags.insert(.present)
    }
    try session.requestFrame(demand: FrameDemand(
      flags: flags,
      token: nextToken
    ))
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

  /// Shows the newest rendered frame, reporting false when no frame reached
  /// the layer.
  func present() throws -> Bool {
    switch kind {
    case let .ownedTexture(compositor), let .borrowedTexture(compositor):
      // Without a new frame, the layer keeps the one it already shows.
      guard let frame = try acquireNewestFrame() else { return true }
      return try compositor.draw(frame: frame)
    case .nativeSurface:
      // The driver already presented the frame.
      return true
    }
  }

  /// Carries a new viewport to the session, replacing a borrowed texture ring.
  /// The session carries the extent to the map.
  func resize(
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws {
    guard case .borrowedTexture = kind else {
      // The session rejects a resize submitted while the host holds a frame.
      // The binding submits on the main actor before it first suspends, so
      // present() acquires no frame between this release and the submission.
      // A frame that it acquires once the resize is accepted stays leased to
      // the host while the driver applies the extent.
      try releaseHeldFrame()
      try await session.resize(extent: viewport.extent)
      return
    }
    // A replacement is refused while the host holds a frame, so the held one
    // goes first, and the layer keeps what it last presented. The session
    // renders into the outgoing ring until the replacement completes, and
    // this call keeps that ring alive until then.
    try releaseHeldFrame()
    let ring = try MetalBorrowedTexture.ring(
      graphics: graphics,
      viewport: viewport
    )
    try await session.setMetalBorrowedTextureTarget(
      descriptor: MetalBorrowedTexture.descriptor(ring, viewport)
    )
    borrowedRing = ring
    // Frames of the old ring can no longer be acquired, so demand a frame
    // for the new ring.
    try requestFrame(force: true)
  }

  func close() async throws {
    do {
      try releaseHeldFrame()
      try await session.detach()
      try await session.close()
    } catch {
      _ = try? session.abandon()
      try? session.dispose()
      throw error
    }
  }

  /// Holds the newest rendered frame, releasing every older one, and returns
  /// it when it is new. The compositor waits for its reads before returning,
  /// so CPU-complete release is accurate.
  private func acquireNewestFrame() throws -> AcquiredFrameHandle? {
    var acquired = false
    while let frame = try session.acquireFrame() {
      try releaseHeldFrame()
      heldFrame = frame
      acquired = true
    }
    return acquired ? heldFrame : nil
  }

  private func releaseHeldFrame() throws {
    try heldFrame?.release(consumerCompletion: nil)
    heldFrame = nil
  }

  private static func withCompositor(
    _ session: RenderSessionHandle,
    graphics: MetalGraphicsContext,
    _ make: (MetalTextureCompositor) -> MetalRenderTarget
  ) throws -> MetalRenderTarget {
    do {
      return try make(MetalTextureCompositor(graphics: graphics))
    } catch {
      _ = try? session.abandon()
      try? session.dispose()
      throw error
    }
  }

  private static func finishAttachment(
    _ created: NewRenderSession
  ) async throws -> RenderSessionHandle {
    let session = created.session
    do {
      try await created.ready.value
      return session
    } catch {
      _ = try? session.abandon()
      try? session.dispose()
      throw error
    }
  }
}

@MainActor
final class MetalTextureCompositor {
  private let layer: CAMetalLayer
  private let queue: any MTLCommandQueue
  private let pipeline: any MTLRenderPipelineState

  init(graphics: MetalGraphicsContext) throws {
    guard let queue = graphics.device.makeCommandQueue() else {
      throw metalError("Metal command queue creation failed")
    }
    layer = graphics.layer
    self.queue = queue
    pipeline = try Self.makePipeline(
      device: graphics.device,
      pixelFormat: graphics.layer.pixelFormat
    )
  }

  func draw(frame: AcquiredFrameHandle) throws -> Bool {
    try frame.withProducerSync { synchronization in
      try frame.withMetalTexture { value in
        let texture = try value.mtlTexture
        return try draw(
          texture: texture,
          producerSynchronization: synchronization
        )
      }
    }
  }

  /// Samples the texture into the layer's next drawable, reporting whether the
  /// frame was presented. An occluded window or an empty drawable pool yields
  /// no drawable, which is reported as false rather than failing the frame.
  private func draw(
    texture: any MTLTexture,
    producerSynchronization: GpuSyncView? = nil
  ) throws -> Bool {
    // The layer's drawable matches the frame it shows.
    let size = CGSize(width: texture.width, height: texture.height)
    if layer.drawableSize != size {
      layer.drawableSize = size
    }
    guard let drawable = layer.nextDrawable() else { return false }
    let passDescriptor = MTLRenderPassDescriptor()
    guard let colorAttachment = passDescriptor.colorAttachments[0] else {
      throw metalError("Metal render pass color attachment 0 is unavailable")
    }
    colorAttachment.texture = drawable.texture
    colorAttachment.loadAction = .clear
    colorAttachment.storeAction = .store
    colorAttachment.clearColor = MTLClearColor(
      red: 0.08,
      green: 0.09,
      blue: 0.11,
      alpha: 1.0
    )

    guard let commandBuffer = queue.makeCommandBuffer() else {
      throw metalError("Metal command buffer creation failed")
    }
    switch producerSynchronization?.kind ?? .cpuComplete {
    case .cpuComplete:
      break
    default:
      throw metalError("Metal frame returned incompatible GPU synchronization")
    }
    guard let encoder = commandBuffer.makeRenderCommandEncoder(
      descriptor: passDescriptor
    ) else {
      throw metalError("Metal render command encoder creation failed")
    }
    encoder.setRenderPipelineState(pipeline)
    encoder.setFragmentTexture(texture, index: 0)
    encoder.drawPrimitives(type: .triangle, vertexStart: 0, vertexCount: 3)
    encoder.endEncoding()
    commandBuffer.present(drawable)
    commandBuffer.commit()
    // CPU-complete frame release is valid only after the compositor finishes
    // sampling the session-owned texture.
    commandBuffer.waitUntilCompleted()
    return true
  }

  private static func makePipeline(
    device: any MTLDevice,
    pixelFormat: MTLPixelFormat
  ) throws -> any MTLRenderPipelineState {
    let library = try device.makeLibrary(
      source: metalCompositorShader,
      options: nil
    )
    guard let vertex = library.makeFunction(name: "vertex_main") else {
      throw metalError("Metal vertex function lookup failed")
    }
    guard let fragment = library.makeFunction(name: "fragment_main") else {
      throw metalError("Metal fragment function lookup failed")
    }

    let descriptor = MTLRenderPipelineDescriptor()
    descriptor.vertexFunction = vertex
    descriptor.fragmentFunction = fragment
    descriptor.colorAttachments[0].pixelFormat = pixelFormat
    return try device.makeRenderPipelineState(descriptor: descriptor)
  }
}

@MainActor
final class MetalBorrowedTexture {
  let texture: any MTLTexture

  init(graphics: MetalGraphicsContext, viewport: Viewport) throws {
    let descriptor = MTLTextureDescriptor.texture2DDescriptor(
      pixelFormat: .bgra8Unorm,
      width: Int(viewport.physicalWidth),
      height: Int(viewport.physicalHeight),
      mipmapped: false
    )
    descriptor.usage = [.shaderRead, .renderTarget]
    guard let texture = graphics.device.makeTexture(descriptor: descriptor)
    else {
      throw metalError("Metal borrowed texture creation failed")
    }
    self.texture = texture
  }

  /// Allocates a ring of ``MetalRenderTarget/borrowedRingDepth`` textures.
  static func ring(
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) throws -> [MetalBorrowedTexture] {
    try (0 ..< MetalRenderTarget.borrowedRingDepth).map { _ in
      try MetalBorrowedTexture(graphics: graphics, viewport: viewport)
    }
  }

  static func descriptor(
    _ ring: [MetalBorrowedTexture],
    _ viewport: Viewport
  ) -> MetalBorrowedTextureDescriptor {
    MetalBorrowedTextureDescriptor(
      extent: viewport.extent,
      physicalWidth: viewport.physicalWidth,
      physicalHeight: viewport.physicalHeight,
      textures: ring.map {
        MaplibreNativeFFI.MetalBorrowedTexture(
          texture: $0.texture
        )
      }
    )
  }
}

private func metalError(_ message: String) -> MaplibreError {
  MaplibreError(status: .nativeError, diagnostic: message)
}

private let metalCompositorShader = """
#include <metal_stdlib>
using namespace metal;

struct VertexOut {
  float4 position [[position]];
  float2 uv;
};

vertex VertexOut vertex_main(uint vertex_id [[vertex_id]]) {
  float2 positions[3] = {
    float2(-1.0, 1.0), float2(3.0, 1.0), float2(-1.0, -3.0),
  };
  float2 uvs[3] = {
    float2(0.0, 0.0), float2(2.0, 0.0), float2(0.0, 2.0),
  };
  VertexOut out;
  out.position = float4(positions[vertex_id], 0.0, 1.0);
  out.uv = uvs[vertex_id];
  return out;
}

fragment float4 fragment_main(
  VertexOut in [[stage_in]],
  texture2d<float> map_texture [[texture(0)]]
) {
  constexpr sampler map_sampler(address::clamp_to_edge, filter::linear);
  return map_texture.sample(map_sampler, in.uv);
}
"""
