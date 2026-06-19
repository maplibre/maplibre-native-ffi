import CMaplibreNativeC
import Foundation
@testable import MaplibreNative
import Testing

private final class RenderCounter: @unchecked Sendable {
  private let lock = NSLock()
  private var count = 0

  func increment() {
    lock.withLock { count += 1 }
  }

  func value() -> Int {
    lock.withLock { count }
  }
}

private final class RenderLeakBox: @unchecked Sendable {
  private let lock = NSLock()
  private var leaks: [NativeHandleLeak] = []

  func append(_ leak: NativeHandleLeak) {
    lock.withLock { leaks.append(leak) }
  }

  func value() -> [NativeHandleLeak] {
    lock.withLock { leaks }
  }
}

@Test func renderTargetDescriptorsMaterializeNativePointersAndExtents() throws {
  let extent = RenderTargetExtent(width: 640, height: 480, scaleFactor: 2)
  let metalSurface = MetalSurfaceDescriptor(
    extent: extent,
    context: MetalContextDescriptor(device: NativePointer(bitPattern: 0x10)),
    layer: NativePointer(bitPattern: 0x20)
  )
  try metalSurface.nativeInput.withNativeDescriptor { descriptor in
    #expect(descriptor.pointee.extent.width == 640)
    #expect(descriptor.pointee.extent.height == 480)
    #expect(descriptor.pointee.extent.scale_factor == 2)
    #expect(UInt(bitPattern: descriptor.pointee.context.device) == 0x10)
    #expect(UInt(bitPattern: descriptor.pointee.layer) == 0x20)
  }

  let vulkanContext = VulkanContextDescriptor(
    instance: NativePointer(bitPattern: 0x30),
    physicalDevice: NativePointer(bitPattern: 0x40),
    device: NativePointer(bitPattern: 0x50),
    graphicsQueue: NativePointer(bitPattern: 0x60),
    graphicsQueueFamilyIndex: 7,
    getInstanceProcAddr: NativePointer(bitPattern: 0x90),
    getDeviceProcAddr: NativePointer(bitPattern: 0xA0)
  )
  let vulkanTexture = VulkanBorrowedTextureDescriptor(
    extent: extent,
    context: vulkanContext,
    image: NativePointer(bitPattern: 0x70),
    imageView: NativePointer(bitPattern: 0x80),
    format: 44,
    initialLayout: 1,
    finalLayout: 2
  )
  try vulkanTexture.nativeInput.withNativeDescriptor { descriptor in
    #expect(UInt(bitPattern: descriptor.pointee.context.instance) == 0x30)
    #expect(UInt(bitPattern: descriptor.pointee.context.physical_device) ==
      0x40)
    #expect(UInt(bitPattern: descriptor.pointee.context.device) == 0x50)
    #expect(UInt(bitPattern: descriptor.pointee.context.graphics_queue) == 0x60)
    #expect(descriptor.pointee.context.graphics_queue_family_index == 7)
    #expect(UInt(
      bitPattern: descriptor.pointee.context.get_instance_proc_addr
    ) ==
      0x90)
    #expect(UInt(bitPattern: descriptor.pointee.context.get_device_proc_addr) ==
      0xA0)
    #expect(UInt(bitPattern: descriptor.pointee.image) == 0x70)
    #expect(UInt(bitPattern: descriptor.pointee.image_view) == 0x80)
    #expect(descriptor.pointee.format == 44)
    #expect(descriptor.pointee.initial_layout == 1)
    #expect(descriptor.pointee.final_layout == 2)
  }

  let wgl = OpenGLContextDescriptor.wgl(
    WglContextDescriptor(
      deviceContext: NativePointer(bitPattern: 0x110),
      shareContext: NativePointer(bitPattern: 0x120),
      getProcAddress: NativePointer(bitPattern: 0x130)
    )
  )
  let openGLSurface = OpenGLSurfaceDescriptor(
    extent: extent,
    context: wgl,
    surface: NativePointer(bitPattern: 0x140)
  )
  try openGLSurface.nativeInput.withNativeDescriptor { descriptor in
    #expect(descriptor.pointee.extent.width == 640)
    #expect(descriptor.pointee.context
      .platform == MLN_OPENGL_CONTEXT_PLATFORM_WGL)
    #expect(UInt(bitPattern: descriptor.pointee.context.data.wgl
        .device_context) == 0x110)
    #expect(UInt(
      bitPattern: descriptor.pointee.context.data.wgl.share_context
    ) ==
      0x120)
    #expect(UInt(bitPattern: descriptor.pointee.context.data.wgl
        .get_proc_address) == 0x130)
    #expect(UInt(bitPattern: descriptor.pointee.surface) == 0x140)
  }

  let egl = OpenGLContextDescriptor.egl(
    EglContextDescriptor(
      display: NativePointer(bitPattern: 0x210),
      config: NativePointer(bitPattern: 0x220),
      shareContext: NativePointer(bitPattern: 0x230),
      getProcAddress: NativePointer(bitPattern: 0x240)
    )
  )
  let openGLTexture = OpenGLBorrowedTextureDescriptor(
    extent: extent,
    context: egl,
    texture: 33,
    target: 0x0DE1
  )
  try openGLTexture.nativeInput.withNativeDescriptor { descriptor in
    #expect(descriptor.pointee.context
      .platform == MLN_OPENGL_CONTEXT_PLATFORM_EGL)
    #expect(UInt(bitPattern: descriptor.pointee.context.data.egl.display) ==
      0x210)
    #expect(UInt(bitPattern: descriptor.pointee.context.data.egl.config) ==
      0x220)
    #expect(UInt(
      bitPattern: descriptor.pointee.context.data.egl.share_context
    ) ==
      0x230)
    #expect(UInt(bitPattern: descriptor.pointee.context.data.egl
        .get_proc_address) == 0x240)
    #expect(descriptor.pointee.texture == 33)
    #expect(descriptor.pointee.target == 0x0DE1)
  }
}

@Test func metalOwnedTextureFrameInvalidatesAfterClose() throws {
  let releases = RenderCounter()
  var raw = mln_metal_owned_texture_frame()
  raw.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
  raw.texture = UnsafeMutableRawPointer(bitPattern: 0x1234)
  raw.device = UnsafeMutableRawPointer(bitPattern: 0x5678)
  let frame =
    MetalOwnedTextureFrameHandle(frame: NativeMetalOwnedTextureFrame(
      raw
    )) { _ in
      releases.increment()
    }

  var capturedView: MetalOwnedTextureFrameView?
  var escapedTexture: FrameNativePointer?
  try frame.withBackendPointers { view in
    capturedView = view
    let texture = try view.texture
    let device = try view.device
    escapedTexture = texture
    #expect(try texture.addressBitPattern == 0x1234)
    #expect(try device.addressBitPattern == 0x5678)
  }
  do {
    _ = try capturedView?.texture
    Issue.record("frame view access after scope should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }
  do {
    _ = try escapedTexture?.addressBitPattern
    Issue.record("escaped frame pointer access after scope should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }

  try frame.close()
  try frame.close()

  #expect(frame.isClosed)
  #expect(releases.value() == 1)
  do {
    try frame.withBackendPointers { _ in }
    Issue.record("closed frame access should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }
}

@Test func vulkanOwnedTextureFrameInvalidatesAfterClose() throws {
  let releases = RenderCounter()
  var raw = mln_vulkan_owned_texture_frame()
  raw.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
  raw.image = UnsafeMutableRawPointer(bitPattern: 0x1234)
  raw.image_view = UnsafeMutableRawPointer(bitPattern: 0x5678)
  let frame =
    VulkanOwnedTextureFrameHandle(frame: NativeVulkanOwnedTextureFrame(
      raw
    )) { _ in
      releases.increment()
    }

  var capturedView: VulkanOwnedTextureFrameView?
  var escapedImage: FrameNativePointer?
  try frame.withBackendPointers { view in
    capturedView = view
    let image = try view.image
    let imageView = try view.imageView
    escapedImage = image
    #expect(try image.addressBitPattern == 0x1234)
    #expect(try imageView.addressBitPattern == 0x5678)
  }
  do {
    _ = try capturedView?.image
    Issue.record("frame view access after scope should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }
  do {
    _ = try escapedImage?.addressBitPattern
    Issue.record("escaped frame pointer access after scope should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }

  try frame.close()
  try frame.close()

  #expect(frame.isClosed)
  #expect(releases.value() == 1)
  do {
    try frame.withBackendPointers { _ in }
    Issue.record("closed frame access should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }
}

@Test func openGLOwnedTextureFrameInvalidatesAfterClose() throws {
  let releases = RenderCounter()
  var raw = mln_opengl_owned_texture_frame()
  raw.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
  raw.texture = 77
  raw.target = 0x0DE1
  let frame =
    OpenGLOwnedTextureFrameHandle(frame: NativeOpenGLOwnedTextureFrame(
      raw
    )) { _ in
      releases.increment()
    }

  var capturedView: OpenGLOwnedTextureFrameView?
  var escapedTexture: FrameOpenGLTextureName?
  try frame.withBackendPointers { view in
    capturedView = view
    let texture = try view.texture
    let target = try view.target
    escapedTexture = texture
    #expect(try texture.value == 77)
    #expect(target == 0x0DE1)
  }
  do {
    _ = try capturedView?.texture
    Issue.record("frame view access after scope should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }
  do {
    _ = try escapedTexture?.value
    Issue.record("escaped OpenGL texture access after scope should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }

  try frame.close()
  try frame.close()

  #expect(frame.isClosed)
  #expect(releases.value() == 1)
  do {
    try frame.withBackendPointers { _ in }
    Issue.record("closed frame access should throw")
  } catch let error as MaplibreError {
    #expect(error.kind == .invalidState)
    #expect(error.rawStatus == nil)
  }
}

@Test func ownedTextureFrameAllowsRetryAfterFailedRelease() throws {
  struct ReleaseFailure: Error {}

  let releases = RenderCounter()
  var raw = mln_opengl_owned_texture_frame()
  raw.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
  raw.texture = 77
  raw.target = 0x0DE1
  let frame =
    OpenGLOwnedTextureFrameHandle(frame: NativeOpenGLOwnedTextureFrame(
      raw
    )) { _ in
      releases.increment()
      if releases.value() == 1 {
        throw ReleaseFailure()
      }
    }

  do {
    try frame.close()
    Issue.record("failed release should throw")
  } catch is ReleaseFailure {}

  #expect(!frame.isClosed)
  try frame.close()
  #expect(frame.isClosed)
  #expect(releases.value() == 2)
}

@Test func textureFrameDeinitReportsLeakWithoutRelease() {
  let releases = RenderCounter()
  let leaks = RenderLeakBox()

  NativeHandleLeakTestSupport.withHandler({ leak in
    leaks.append(leak)
  }) {
    do {
      var raw = mln_metal_owned_texture_frame()
      raw.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
      raw.texture = UnsafeMutableRawPointer(bitPattern: 0x1234)
      raw.device = UnsafeMutableRawPointer(bitPattern: 0x5678)
      _ =
        MetalOwnedTextureFrameHandle(frame: NativeMetalOwnedTextureFrame(
          raw
        )) { _ in
          releases.increment()
        }
    }

    #expect(releases.value() == 0)
    #expect(leaks.value() == [NativeHandleLeak(
      typeName: "MetalOwnedTextureFrameHandle",
      address: 0x1234
    )])
  }
}
