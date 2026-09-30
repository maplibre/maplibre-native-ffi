import Foundation
import GraphicsSupport
import MaplibreNativeFFI
import Testing

/// A device or context from tests/graphics for the build's render backend.
final class OwnedTextureFixture {
  private let graphics: OpaquePointer
  private let context: mln_test_graphics_context

  init() throws {
    let backends = try Maplibre.supportedRenderBackendMask()
    let backend = if backends.contains(.metal) {
      MLN_TEST_GRAPHICS_BACKEND_METAL
    } else if backends.contains(.vulkan) {
      MLN_TEST_GRAPHICS_BACKEND_VULKAN
    } else {
      MLN_TEST_GRAPHICS_BACKEND_EGL
    }
    guard let graphics = mln_test_graphics_create(UInt32(backend)) else {
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
    var context = mln_test_graphics_context()
    guard mln_test_graphics_get_context(graphics, &context) else {
      mln_test_graphics_destroy(graphics)
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
    self.graphics = graphics
    self.context = context
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
    switch Int(context.backend) {
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      return try map.metalOwnedTextureAttach(
        descriptor: MetalOwnedTextureDescriptor(
          extent: extent,
          context: MetalContextDescriptor(device: pointer(context.metal_device))
        ), options: options
      )
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      return try map.vulkanOwnedTextureAttach(
        descriptor: VulkanOwnedTextureDescriptor(
          extent: extent,
          context: VulkanContextDescriptor(
            instance: pointer(context.vulkan_instance),
            physicalDevice: pointer(context.vulkan_physical_device),
            device: pointer(context.vulkan_device),
            graphicsQueue: pointer(context.vulkan_queue),
            graphicsQueueFamilyIndex: context.vulkan_queue_family_index,
            getInstanceProcAddr: pointer(context.vulkan_get_instance_proc_addr),
            getDeviceProcAddr: pointer(context.vulkan_get_device_proc_addr)
          )
        ), options: options
      )
    default:
      // A core worker cannot share the host's context, so the session owns a
      // dedicated one on the fixture's display.
      return try map.openglOwnedTextureAttach(
        descriptor: OpenglOwnedTextureDescriptor(
          extent: extent,
          context: OpenglContextDescriptor(
            ownership: .dedicated,
            data: .egl(EglContextDescriptor(
              display: pointer(context.egl_display),
              config: pointer(context.egl_config),
              clientApi: .gles
            ))
          )
        ), options: options
      )
    }
  }
}

/// EGL display initialization is shared by a driver process.
@Suite(.serialized)
struct OwnedTextureTests {}
