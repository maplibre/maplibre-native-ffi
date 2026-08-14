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
    texture: MetalBorrowedTexture
  )
  case nativeSurface(session: RenderSessionHandle)

  /// Attaches a session against the map the map task published.
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
      try await session.resize(viewport.extent)
      compositor.resize(viewport)
    case let .borrowedTexture(session, compositor, _):
      let replacement = try MetalBorrowedTexture(
        graphics: graphics,
        viewport: viewport
      )
      try await session.setMetalBorrowedTextureTarget(
        MetalBorrowedTextureDescriptor(
          extent: viewport.extent,
          physicalWidth: viewport.physicalWidth,
          physicalHeight: viewport.physicalHeight,
          texture: replacement.pointer
        )
      )
      compositor.resize(viewport)
      self = .borrowedTexture(
        session: session,
        compositor: compositor,
        texture: replacement
      )
    case let .nativeSurface(session):
      try await session.resize(viewport.extent)
    }
  }

  /// Services caller-driver work and submits one host-paced frame demand.
  func renderFrame() async throws -> Bool {
    let session: RenderSessionHandle
    switch self {
    case let .ownedTexture(value, _),
         let .borrowedTexture(value, _, _),
         let .nativeSurface(value):
      session = value
    }
    try session.requestFrame(FrameDemand(options: [.ifNeeded, .present]))
    try session.serviceDriverWork()
    let results = try session.drainFrameResults()
    guard let result = results.last else { return false }
    if !results.contains(where: { $0.result == .rendered }) {
      return result.result != .sizePending &&
        result.result != .targetNotReady
    }

    switch self {
    case let .ownedTexture(session, compositor):
      guard let frame = try session.acquireFrame() else { return false }
      do {
        let presented = try compositor.draw(frame: frame)
        try await frame.release()
        return presented
      } catch {
        try? await frame.release()
        throw error
      }
    case let .borrowedTexture(_, compositor, texture):
      return try compositor.draw(texture: texture.texture)
    case .nativeSurface:
      return true
    }
  }

  func close() async throws {
    let session: RenderSessionHandle
    switch self {
    case let .ownedTexture(value, _),
         let .borrowedTexture(value, _, _),
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
    let session = try await map.attachMetalOwnedTexture(
      MetalOwnedTextureDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor
      ),
      options: .init(
        driver: .callerGraphicsThread,
        requestedTextureRingDepth: 3
      )
    )
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
    let session = try await map.attachMetalBorrowedTexture(
      MetalBorrowedTextureDescriptor(
        extent: viewport.extent,
        physicalWidth: viewport.physicalWidth,
        physicalHeight: viewport.physicalHeight,
        texture: texture.pointer
      ),
      options: .init(driver: .callerGraphicsThread)
    )
    do {
      let compositor = try MetalTextureCompositor(graphics: graphics)
      return .borrowedTexture(
        session: session,
        compositor: compositor,
        texture: texture
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
    let session = try await map.attachMetalSurface(
      MetalSurfaceDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor,
        layer: graphics.layerPointer
      ),
      options: .init(driver: .callerGraphicsThread)
    )
    return .nativeSurface(session: session)
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
    let synchronization = try frame.producerSynchronization()
    var presented = false
    try frame.withMetalTexture { value in
      let texture = try metalTexture(address: value.texture.addressBitPattern)
      presented = try draw(
        texture: texture,
        producerSynchronization: synchronization
      )
    }
    return presented
  }

  /// Samples the texture into the layer's next drawable, reporting whether the
  /// frame was presented. An occluded window or an empty drawable pool yields
  /// no drawable, which is reported as false rather than failing the frame.
  func draw(
    texture: any MTLTexture,
    producerSynchronization: GPUSynchronization = .cpuComplete
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
    switch producerSynchronization {
    case .cpuComplete:
      break
    case let .metalSharedEvent(pointer, value):
      let event = try metalSharedEvent(address: pointer.addressBitPattern)
      commandBuffer.encodeWaitForEvent(event, value: value)
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
