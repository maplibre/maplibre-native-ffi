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

/// The render session and its mode-specific resources. The host cadence
/// services driver work on the graphics thread that owns the Metal objects.
/// Every operation remains isolated to the main actor.
@MainActor
enum MetalRenderTarget {
  case ownedTexture(
    session: RenderSessionHandle,
    compositor: MetalTextureCompositor
  )
  case borrowedTexture(
    session: RenderSessionHandle,
    compositor: MetalTextureCompositor,
    texture: MetalBorrowedTexture,
    map: MapHandle
  )
  case nativeSurface(session: RenderSessionHandle)

  /// Attaches a session against the map owned by the view.
  static func attach(
    mode: RenderTargetMode,
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws -> MetalRenderTarget {
    switch mode {
    case .ownedTexture:
      return try await attachOwnedTexture(
        map: map,
        graphics: graphics,
        viewport: viewport
      )
    case .borrowedTexture:
      return try await attachBorrowedTexture(
        map: map,
        graphics: graphics,
        viewport: viewport
      )
    case .nativeSurface:
      return try await attachNativeSurface(
        map: map,
        graphics: graphics,
        viewport: viewport
      )
    }
  }

  /// Resizes without closing the session; a caller-owned texture is replaced
  /// with one at the new size and handed over.
  mutating func resize(
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws {
    switch self {
    case let .ownedTexture(session, compositor):
      try await session.resize(extent: viewport.extent)
      compositor.resize(viewport)
    case let .borrowedTexture(session, compositor, _, map):
      let replacement = try MetalBorrowedTexture(
        graphics: graphics,
        viewport: viewport
      )
      try await session
        .metalBorrowedTextureSetTarget(
          descriptor: MetalBorrowedTextureDescriptor(
            extent: viewport.extent,
            physicalWidth: viewport.physicalWidth,
            physicalHeight: viewport.physicalHeight,
            texture: replacement.pointer
          )
        )
      compositor.resize(viewport)
      // A handover replaces only the graphics resource, so the map still needs
      // the new extent.
      _ = try await map.resize(extent: LogicalExtent(
        width: viewport.logicalWidth,
        height: viewport.logicalHeight,
        scaleFactor: viewport.scaleFactor
      ))
      self = .borrowedTexture(
        session: session,
        compositor: compositor,
        texture: replacement,
        map: map
      )
    case let .nativeSurface(session):
      try await session.resize(extent: viewport.extent)
    }
  }

  /// Services caller-driver work, submits one host-paced frame demand, and
  /// reports whether the render loop may rest. It reports false when no frame
  /// reached the screen and when the map asked for another frame while this one
  /// rendered, so the loop demands one more.
  func renderFrame() async throws -> Bool {
    let session: RenderSessionHandle
    switch self {
    case let .ownedTexture(value, _),
         let .borrowedTexture(value, _, _, _),
         let .nativeSurface(value):
      session = value
    }
    try session.requestFrame(demand: FrameDemand(flags: [.ifNeeded, .present]))
    _ = try session.serviceDriverWork(maxWork: 0)
    let batch: RenderFrameBatchHandle
    do { batch = try session.drainFrameResults() }
    catch let error as MaplibreError
      where error.kind == .notReady { return false }
    defer { try? batch.close() }
    let count = try batch.count()
    guard count > 0 else { return false }
    let result = try batch.get(index: count - 1)
    guard result.disposition == .rendered else { return false }

    switch self {
    case let .ownedTexture(session, compositor):
      // An empty ring keeps the previously composited frame on screen.
      let frame: AcquiredFrameHandle
      do { frame = try session.acquireFrame() }
      catch let error as MaplibreError
        where error.kind == .notReady { return false }
      do {
        let presented = try compositor.draw(frame: frame)
        try frame.release(consumerCompletion: .default)
        return presented && !result.needsRepaint
      } catch {
        try? frame.release(consumerCompletion: .default)
        throw error
      }
    case let .borrowedTexture(_, compositor, texture, _):
      return try compositor.draw(texture: texture.texture) && !result
        .needsRepaint
    case .nativeSurface:
      return !result.needsRepaint
    }
  }

  func close() async throws {
    let session: RenderSessionHandle
    switch self {
    case let .ownedTexture(value, _),
         let .borrowedTexture(value, _, _, _),
         let .nativeSurface(value):
      session = value
    }
    do {
      try await session.detach()
      try session.close()
    } catch {
      _ = try? session.abandon()
      try? session.close()
      throw error
    }
  }

  private static func attachOwnedTexture(
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws -> MetalRenderTarget {
    let driverRelay = DriverRelay()
    let attachment = try map.metalOwnedTextureAttach(
      descriptor: MetalOwnedTextureDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor
      ),
      options: .init(
        driver: .callerGraphicsThread,
        requestedTextureRingDepth: 3,
        driverWorkWake: driverRelay.wake
      )
    )
    let session = try await finishAttachment(attachment, relay: driverRelay)
    do {
      let compositor = try MetalTextureCompositor(graphics: graphics)
      return .ownedTexture(session: session, compositor: compositor)
    } catch {
      try? await session.detach()
      try? session.close()
      throw error
    }
  }

  private static func attachBorrowedTexture(
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) async throws -> MetalRenderTarget {
    let texture = try MetalBorrowedTexture(
      graphics: graphics,
      viewport: viewport
    )
    let driverRelay = DriverRelay()
    let attachment = try map.metalBorrowedTextureAttach(
      descriptor: MetalBorrowedTextureDescriptor(
        extent: viewport.extent,
        physicalWidth: viewport.physicalWidth,
        physicalHeight: viewport.physicalHeight,
        texture: texture.pointer
      ),
      options: .init(
        driver: .callerGraphicsThread,
        driverWorkWake: driverRelay.wake
      )
    )
    let session = try await finishAttachment(attachment, relay: driverRelay)
    do {
      let compositor = try MetalTextureCompositor(graphics: graphics)
      return .borrowedTexture(
        session: session,
        compositor: compositor,
        texture: texture,
        map: map
      )
    } catch {
      try? await session.detach()
      try? session.close()
      throw error
    }
  }

  private static func attachNativeSurface(
    map: MapHandle,
    graphics: MetalGraphicsContext,
    viewport: Viewport
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
        driverWorkWake: driverRelay.wake
      )
    )
    return try .nativeSurface(session: await finishAttachment(
      attachment,
      relay: driverRelay
    ))
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

  func resize(_ viewport: Viewport) {
    guard !viewport.isEmpty else { return }
    layer.drawableSize = CGSize(
      width: Int(viewport.physicalWidth),
      height: Int(viewport.physicalHeight)
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

  var pointer: NativePointer {
    nativePointer(texture as AnyObject)
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
