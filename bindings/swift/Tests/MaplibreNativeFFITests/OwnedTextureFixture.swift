import Foundation
import GraphicsSupport
import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
#endif

final class OwnedTextureFixture {
  private let graphics: UnsafeMutablePointer<mln_test_graphics>?
  #if canImport(Metal)
    private var metalDevice: MTLDevice?
  #endif

  init() throws {
    let backends = try Maplibre.supportedRenderBackendMask()
    #if canImport(Metal)
      if backends.contains(.metal) {
        let device: MTLDevice = try #require(MTLCreateSystemDefaultDevice())
        metalDevice = device
        graphics = nil
        return
      }
    #endif
    #expect(backends.contains(.vulkan) || backends.contains(.opengl))
    let state: UnsafeMutablePointer<mln_test_graphics> =
      try #require(mln_test_graphics_create(backends.contains(.vulkan)))
    graphics = state
  }

  deinit { mln_test_graphics_destroy(graphics) }

  func attach(
    map: MapHandle,
    extent: RenderTargetExtent = RenderTargetExtent(
      width: 32,
      height: 32,
      scaleFactor: 1
    )
  ) throws -> RenderSessionAttachment {
    let options = RenderSessionAttachOptions(driver: .coreWorker)
    func pointer(_ address: UnsafeMutableRawPointer?) -> NativePointer {
      NativePointer(bitPattern: UInt(bitPattern: address))
    }
    #if canImport(Metal)
      if let metalDevice {
        return try map.metalOwnedTextureAttach(
          descriptor: MetalOwnedTextureDescriptor(
            extent: extent,
            context: MetalContextDescriptor(device: pointer(Unmanaged
                .passUnretained(metalDevice as AnyObject).toOpaque()))
          ), options: options
        )
      }
    #endif
    let state = try #require(graphics).pointee
    if state.vulkan {
      return try map.vulkanOwnedTextureAttach(
        descriptor: VulkanOwnedTextureDescriptor(
          extent: extent,
          context: VulkanContextDescriptor(
            instance: pointer(state.instance),
            physicalDevice: pointer(state.physical_device),
            device: pointer(state.device), graphicsQueue: pointer(state.queue),
            graphicsQueueFamilyIndex: state.queue_family,
            getInstanceProcAddr: pointer(state.get_instance_proc_addr),
            getDeviceProcAddr: pointer(state.get_device_proc_addr)
          )
        ), options: options
      )
    }
    return try map.openglOwnedTextureAttach(
      descriptor: OpenglOwnedTextureDescriptor(
        extent: extent,
        context: OpenglContextDescriptor(
          ownership: .dedicated,
          data: .egl(EglContextDescriptor(
            display: pointer(state.display),
            config: pointer(state.config),
            clientApi: .gles
          ))
        )
      ), options: options
    )
  }
}

/// EGL display initialization and termination are shared by a driver process.
@Suite(.serialized)
struct OwnedTextureTests {}
