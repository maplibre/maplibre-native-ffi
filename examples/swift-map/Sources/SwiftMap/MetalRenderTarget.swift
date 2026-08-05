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

/// The render session and its mode-specific resources. Attach records the
/// calling thread as the session's owner, so every call here runs on the render
/// loop thread that owns the Metal objects.
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

  /// Attaches a session against the map the runtime loop published.
  static func attach(
    mode: RenderTargetMode,
    attachRef: MapAttachRef,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) throws -> MetalRenderTarget {
    switch mode {
    case .ownedTexture:
      return try attachOwnedTexture(
        attachRef: attachRef,
        graphics: graphics,
        viewport: viewport
      )
    case .borrowedTexture:
      return try attachBorrowedTexture(
        attachRef: attachRef,
        graphics: graphics,
        viewport: viewport
      )
    case .nativeSurface:
      return try attachNativeSurface(
        attachRef: attachRef,
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
  ) throws {
    switch self {
    case let .ownedTexture(session, compositor):
      compositor.resize(viewport)
      try session.resize(
        width: viewport.logicalWidth,
        height: viewport.logicalHeight,
        scaleFactor: viewport.scaleFactor
      )
    case let .borrowedTexture(session, compositor, _):
      let replacement = try MetalBorrowedTexture(
        graphics: graphics,
        viewport: viewport
      )
      do {
        try session
          .setMetalBorrowedTextureTarget(MetalBorrowedTextureDescriptor(
            extent: viewport.extent,
            physicalWidth: viewport.physicalWidth,
            physicalHeight: viewport.physicalHeight,
            texture: replacement.pointer
          ))
      } catch {
        // The session may hold either texture, and both are released as this
        // scope unwinds, so detach first.
        try? session.detach()
        throw error
      }
      compositor.resize(viewport)
      self = .borrowedTexture(
        session: session,
        compositor: compositor,
        texture: replacement
      )
    case let .nativeSurface(session):
      try session.resize(
        width: viewport.logicalWidth,
        height: viewport.logicalHeight,
        scaleFactor: viewport.scaleFactor
      )
    }
  }

  /// Renders the latest map update, reporting whether a frame was presented.
  /// This reports false for a few iterations after attach or resize, because
  /// the map applies a new logical size on the runtime loop's next pump.
  func renderUpdate() throws -> Bool {
    switch self {
    case let .ownedTexture(session, compositor):
      guard try session.renderUpdate() else { return false }
      let frame = try session.acquireMetalOwnedTextureFrame()
      var presented = false
      var firstError: Error?
      do {
        presented = try compositor.draw(frame: frame)
      } catch {
        firstError = error
      }
      do {
        try frame.close()
      } catch {
        firstError = firstError ?? error
      }
      if let firstError {
        throw firstError
      }
      return presented
    case let .borrowedTexture(session, compositor, texture):
      guard try session.renderUpdate() else { return false }
      return try compositor.draw(texture: texture.texture)
    case let .nativeSurface(session):
      return try session.renderUpdate()
    }
  }

  func finishFrame() throws {
    // Metal surface and texture paths need no per-iteration host upkeep here.
  }

  func close() throws {
    switch self {
    case let .ownedTexture(session, _):
      try session.close()
    case let .borrowedTexture(session, _, _):
      try session.close()
    case let .nativeSurface(session):
      try session.close()
    }
  }

  private static func attachOwnedTexture(
    attachRef: MapAttachRef,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) throws -> MetalRenderTarget {
    let session =
      try attachRef.attachMetalOwnedTexture(MetalOwnedTextureDescriptor(
        extent: viewport.extent,
        context: graphics.contextDescriptor
      ))
    do {
      let compositor = try MetalTextureCompositor(graphics: graphics)
      return .ownedTexture(session: session, compositor: compositor)
    } catch {
      try? session.close()
      throw error
    }
  }

  private static func attachBorrowedTexture(
    attachRef: MapAttachRef,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) throws -> MetalRenderTarget {
    let texture = try MetalBorrowedTexture(
      graphics: graphics,
      viewport: viewport
    )
    let session = try attachRef
      .attachMetalBorrowedTexture(MetalBorrowedTextureDescriptor(
        extent: viewport.extent,
        physicalWidth: viewport.physicalWidth,
        physicalHeight: viewport.physicalHeight,
        texture: texture.pointer
      ))
    do {
      let compositor = try MetalTextureCompositor(graphics: graphics)
      return .borrowedTexture(
        session: session,
        compositor: compositor,
        texture: texture
      )
    } catch {
      try? session.close()
      throw error
    }
  }

  private static func attachNativeSurface(
    attachRef: MapAttachRef,
    graphics: MetalGraphicsContext,
    viewport: Viewport
  ) throws -> MetalRenderTarget {
    let session = try attachRef.attachMetalSurface(MetalSurfaceDescriptor(
      extent: viewport.extent,
      context: graphics.contextDescriptor,
      layer: graphics.layerPointer
    ))
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

  func draw(frame: MetalOwnedTextureFrameHandle) throws -> Bool {
    var presented = false
    try frame.withBackendPointers { view in
      let address = try view.texture.addressBitPattern
      let texture = try metalTexture(address: address)
      presented = try draw(texture: texture)
    }
    return presented
  }

  /// Samples the texture into the layer's next drawable, reporting whether the
  /// frame was presented. An occluded window or an empty drawable pool yields
  /// no drawable, which is reported as false rather than failing the frame.
  func draw(texture: any MTLTexture) throws -> Bool {
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
