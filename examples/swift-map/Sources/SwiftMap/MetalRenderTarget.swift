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
  /// A demand rendered a frame.
  var rendered = false
  /// The map asked for another frame while it rendered one.
  var needsRepaint = false
  /// The target could not produce a frame, so the loop retries later.
  var targetNotReady = false
}

/// The render session and its mode-specific resources. A driver wake services
/// caller-driver work on the main thread that owns the Metal objects.
@MainActor
final class MetalRenderTarget {
  private enum Kind {
    case ownedTexture(MetalTextureCompositor)
    case borrowedTexture(MetalTextureCompositor)
    case nativeSurface
  }

  let session: RenderSessionHandle
  private let kind: Kind
  private let driverRelay: DriverRelay
  /// The caller-owned texture the session renders into as far as completed
  /// replacements show.
  private var borrowedTexture: MetalBorrowedTexture?
  private var replacementCount = 0
  private var shownReplacement = 0
  private var nextToken: UInt64 = 0

  private init(
    session: RenderSessionHandle,
    kind: Kind,
    driverRelay: DriverRelay,
    borrowedTexture: MetalBorrowedTexture? = nil
  ) {
    self.session = session
    self.kind = kind
    self.driverRelay = driverRelay
    self.borrowedTexture = borrowedTexture
  }

  /// Attaches a session against the map. `frameWake` reports frame results.
  static func attach(
    mode: RenderTargetMode,
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport,
    frameWake: Wake
  ) async throws -> MetalRenderTarget {
    let driverRelay = DriverRelay()
    let options = RenderSessionAttachOptions(
      driver: .callerGraphicsThread,
      requestedTextureRingDepth: mode == .ownedTexture ? 3 : 0,
      frameWake: frameWake,
      driverWorkWake: driverRelay.wake
    )
    switch mode {
    case .ownedTexture:
      let session = try await finishAttachment(
        map.metalOwnedTextureAttach(
          descriptor: MetalOwnedTextureDescriptor(
            extent: viewport.extent,
            context: graphics.contextDescriptor
          ),
          options: options
        ),
        relay: driverRelay
      )
      return try withCompositor(session, graphics: graphics) {
        MetalRenderTarget(
          session: session,
          kind: .ownedTexture($0),
          driverRelay: driverRelay
        )
      }
    case .borrowedTexture:
      let texture = try MetalBorrowedTexture(
        graphics: graphics,
        viewport: viewport
      )
      let session = try await finishAttachment(
        map.metalBorrowedTextureAttach(
          descriptor: texture.descriptor(viewport),
          options: options
        ),
        relay: driverRelay
      )
      return try withCompositor(session, graphics: graphics) {
        MetalRenderTarget(
          session: session,
          kind: .borrowedTexture($0),
          driverRelay: driverRelay,
          borrowedTexture: texture
        )
      }
    case .nativeSurface:
      let session = try await finishAttachment(
        map.metalSurfaceAttach(
          descriptor: MetalSurfaceDescriptor(
            extent: viewport.extent,
            context: graphics.contextDescriptor,
            layer: graphics.layerPointer
          ),
          options: options
        ),
        relay: driverRelay
      )
      return MetalRenderTarget(
        session: session,
        kind: .nativeSurface,
        driverRelay: driverRelay
      )
    }
  }

  /// Demands a frame. A forced demand renders even without a newer map
  /// update, which a retry after an undrawn frame needs.
  func requestFrame(force: Bool = false) throws {
    nextToken += 1
    var flags: FrameDemandFlag = force ? [] : [.ifNeeded]
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

  /// Shows the newest rendered frame, reporting false when no frame reached
  /// the layer.
  func present() throws -> Bool {
    switch kind {
    case let .ownedTexture(compositor):
      // Without a new frame, the layer keeps the one it already shows.
      guard let frame = try acquireNewestFrame() else { return true }
      defer { try? frame.release(consumerCompletion: .default) }
      return try compositor.draw(frame: frame)
    case let .borrowedTexture(compositor):
      return try compositor.draw(texture: borrowedTexture!.texture)
    case .nativeSurface:
      // The driver already presented the frame.
      return true
    }
  }

  /// Starts the session resize or target replacement a new viewport needs,
  /// reporting an asynchronous failure through `onFailure`. A later resize
  /// supersedes an earlier one that has not applied yet.
  func resize(
    graphics: MetalGraphicsContext,
    viewport: Viewport,
    mapState: MapState,
    onFailure: @escaping @MainActor (Error) -> Void
  ) throws {
    guard case .borrowedTexture = kind else {
      let session = session
      Task { @MainActor in
        do { try await session.resize(extent: viewport.extent) }
        catch { onFailure(error) }
      }
      return
    }
    // The session renders into the outgoing texture until the replacement
    // completes, and this task keeps the replacement alive until then.
    let replacement = try MetalBorrowedTexture(
      graphics: graphics,
      viewport: viewport
    )
    replacementCount += 1
    let count = replacementCount
    let session = session
    Task { @MainActor [weak self] in
      do {
        try await session.metalBorrowedTextureSetTarget(
          descriptor: replacement.descriptor(viewport)
        )
        guard let self, count > self.shownReplacement else { return }
        self.shownReplacement = count
        self.borrowedTexture = replacement
      } catch {
        onFailure(error)
      }
    }
    // A handover replaces only the graphics resource, so the map still needs
    // the new extent.
    mapState.resize(viewport)
  }

  func close() async throws {
    do {
      try await session.detach()
      try session.close()
    } catch {
      _ = try? session.abandon()
      try? session.close()
      throw error
    }
  }

  private func acquireNewestFrame() throws -> AcquiredFrameHandle? {
    var newest: AcquiredFrameHandle?
    while true {
      do {
        let frame = try session.acquireFrame()
        try newest?.release(consumerCompletion: .default)
        newest = frame
      } catch let error as MaplibreError where error.kind == .notReady {
        return newest
      }
    }
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
      try? session.close()
      throw error
    }
  }

  private static func finishAttachment(
    _ attachment: RenderSessionAttachment, relay: DriverRelay
  ) async throws -> RenderSessionHandle {
    let session = attachment.session
    do {
      relay.session = session
      _ = try session.serviceDriverWork(maxWork: 0)
      try await attachment.completion.value
      return session
    } catch {
      _ = try? session.abandon()
      try? session.close()
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
        let texture = try metalTexture(address: value.texture.addressBitPattern)
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
  func draw(
    texture: any MTLTexture,
    producerSynchronization: GpuSyncView? = nil
  ) throws -> Bool {
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
    switch try producerSynchronization?.kind ?? .cpuComplete {
    case .cpuComplete:
      break
    case .metalSharedEvent:
      let event =
        try metalSharedEvent(address: UInt(producerSynchronization!.object))
      try commandBuffer.encodeWaitForEvent(
        event,
        value: producerSynchronization!.value
      )
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
      pixelFormat: .rgba8Unorm,
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

  func descriptor(_ viewport: Viewport) -> MetalBorrowedTextureDescriptor {
    MetalBorrowedTextureDescriptor(
      extent: viewport.extent,
      physicalWidth: viewport.physicalWidth,
      physicalHeight: viewport.physicalHeight,
      texture: nativePointer(texture as AnyObject)
    )
  }
}

private func metalTexture(address: UInt) throws -> any MTLTexture {
  guard let pointer = UnsafeRawPointer(bitPattern: address) else {
    throw metalError("Metal texture frame has a null texture")
  }
  let object = Unmanaged<AnyObject>.fromOpaque(pointer).takeUnretainedValue()
  guard let texture = object as? any MTLTexture else {
    throw metalError(
      "Metal texture frame pointer did not contain an MTLTexture"
    )
  }
  return texture
}

private func metalSharedEvent(address: UInt) throws -> any MTLSharedEvent {
  guard let pointer = UnsafeRawPointer(bitPattern: address) else {
    throw metalError("Metal frame has a null shared event")
  }
  let object = Unmanaged<AnyObject>.fromOpaque(pointer).takeUnretainedValue()
  guard let event = object as? any MTLSharedEvent else {
    throw metalError(
      "Metal frame pointer did not contain an MTLSharedEvent"
    )
  }
  return event
}

private func nativePointer(_ object: AnyObject) -> NativePointer {
  NativePointer(
    bitPattern: UInt(bitPattern: Unmanaged.passUnretained(object).toOpaque())
  )
}

private func metalError(_ message: String) -> MaplibreError {
  MaplibreError(kind: .nativeError, rawStatus: nil, diagnostic: message)
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

@MainActor
private final class DriverRelay {
  weak var session: RenderSessionHandle?
  var wake: Wake {
    Wake(callback: { [self] in Task { @MainActor in
      _ = try? session?.serviceDriverWork(maxWork: 0)
    } })
  }
}
