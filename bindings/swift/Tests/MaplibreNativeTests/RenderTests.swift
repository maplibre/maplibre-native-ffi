import CMaplibreNativeC
import Foundation
import Testing

@testable import MaplibreNative

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
    graphicsQueueFamilyIndex: 7
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
    #expect(UInt(bitPattern: descriptor.pointee.context.physical_device) == 0x40)
    #expect(UInt(bitPattern: descriptor.pointee.context.device) == 0x50)
    #expect(UInt(bitPattern: descriptor.pointee.context.graphics_queue) == 0x60)
    #expect(descriptor.pointee.context.graphics_queue_family_index == 7)
    #expect(UInt(bitPattern: descriptor.pointee.image) == 0x70)
    #expect(UInt(bitPattern: descriptor.pointee.image_view) == 0x80)
    #expect(descriptor.pointee.format == 44)
    #expect(descriptor.pointee.initial_layout == 1)
    #expect(descriptor.pointee.final_layout == 2)
  }
}

@Test func metalOwnedTextureFrameInvalidatesAfterClose() throws {
  let releases = RenderCounter()
  var raw = mln_metal_owned_texture_frame()
  raw.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
  raw.texture = UnsafeMutableRawPointer(bitPattern: 0x1234)
  raw.device = UnsafeMutableRawPointer(bitPattern: 0x5678)
  let frame = MetalOwnedTextureFrameHandle(frame: NativeMetalOwnedTextureFrame(raw)) { _ in
    releases.increment()
  }

  var capturedView: MetalOwnedTextureFrameView?
  try frame.withBackendPointers { view in
    capturedView = view
    let texture = try view.texture
    let device = try view.device
    #expect(texture == NativePointer(bitPattern: 0x1234))
    #expect(device == NativePointer(bitPattern: 0x5678))
  }
  do {
    _ = try capturedView?.texture
    Issue.record("frame view access after scope should throw")
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
  let frame = VulkanOwnedTextureFrameHandle(frame: NativeVulkanOwnedTextureFrame(raw)) { _ in
    releases.increment()
  }

  var capturedView: VulkanOwnedTextureFrameView?
  try frame.withBackendPointers { view in
    capturedView = view
    let image = try view.image
    let imageView = try view.imageView
    #expect(image == NativePointer(bitPattern: 0x1234))
    #expect(imageView == NativePointer(bitPattern: 0x5678))
  }
  do {
    _ = try capturedView?.image
    Issue.record("frame view access after scope should throw")
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

@Test func textureFrameDeinitReportsLeakWithoutRelease() throws {
  let releases = RenderCounter()
  let leaks = RenderLeakBox()
  NativeHandleLeakReporter.setHandler { leak in
    leaks.append(leak)
  }
  defer { NativeHandleLeakReporter.resetHandler() }

  do {
    var raw = mln_metal_owned_texture_frame()
    raw.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
    raw.texture = UnsafeMutableRawPointer(bitPattern: 0x1234)
    raw.device = UnsafeMutableRawPointer(bitPattern: 0x5678)
    _ = MetalOwnedTextureFrameHandle(frame: NativeMetalOwnedTextureFrame(raw)) { _ in
      releases.increment()
    }
  }

  #expect(releases.value() == 0)
  #expect(leaks.value() == [NativeHandleLeak(typeName: "MetalOwnedTextureFrameHandle", address: 0x1234)])
}
