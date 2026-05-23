internal import CMaplibreNativeC

struct NativeRenderTargetExtent: Equatable, Sendable {
  let width: UInt32
  let height: UInt32
  let scaleFactor: Double

  init(width: UInt32, height: UInt32, scaleFactor: Double) {
    self.width = width
    self.height = height
    self.scaleFactor = scaleFactor
  }

  var native: mln_render_target_extent {
    mln_render_target_extent(
      size: UInt32(MemoryLayout<mln_render_target_extent>.size),
      width: width,
      height: height,
      scale_factor: scaleFactor
    )
  }
}

struct NativeMetalContextDescriptor: Equatable, Sendable {
  let deviceAddress: UInt

  init(deviceAddress: UInt = 0) {
    self.deviceAddress = deviceAddress
  }

  var native: mln_metal_context_descriptor {
    mln_metal_context_descriptor(
      size: UInt32(MemoryLayout<mln_metal_context_descriptor>.size),
      device: UnsafeMutableRawPointer(bitPattern: deviceAddress)
    )
  }
}

struct NativeVulkanContextDescriptor: Equatable, Sendable {
  let instanceAddress: UInt
  let physicalDeviceAddress: UInt
  let deviceAddress: UInt
  let graphicsQueueAddress: UInt
  let graphicsQueueFamilyIndex: UInt32

  init(
    instanceAddress: UInt,
    physicalDeviceAddress: UInt,
    deviceAddress: UInt,
    graphicsQueueAddress: UInt,
    graphicsQueueFamilyIndex: UInt32
  ) {
    self.instanceAddress = instanceAddress
    self.physicalDeviceAddress = physicalDeviceAddress
    self.deviceAddress = deviceAddress
    self.graphicsQueueAddress = graphicsQueueAddress
    self.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex
  }

  var native: mln_vulkan_context_descriptor {
    mln_vulkan_context_descriptor(
      size: UInt32(MemoryLayout<mln_vulkan_context_descriptor>.size),
      instance: UnsafeMutableRawPointer(bitPattern: instanceAddress),
      physical_device: UnsafeMutableRawPointer(bitPattern: physicalDeviceAddress),
      device: UnsafeMutableRawPointer(bitPattern: deviceAddress),
      graphics_queue: UnsafeMutableRawPointer(bitPattern: graphicsQueueAddress),
      graphics_queue_family_index: graphicsQueueFamilyIndex
    )
  }
}

struct NativeMetalSurfaceDescriptorInput: Equatable, Sendable {
  let extent: NativeRenderTargetExtent
  let context: NativeMetalContextDescriptor
  let layerAddress: UInt

  init(extent: NativeRenderTargetExtent, context: NativeMetalContextDescriptor, layerAddress: UInt) {
    self.extent = extent
    self.context = context
    self.layerAddress = layerAddress
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.metalSurfaceDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.layer = UnsafeMutableRawPointer(bitPattern: layerAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeVulkanSurfaceDescriptorInput: Equatable, Sendable {
  let extent: NativeRenderTargetExtent
  let context: NativeVulkanContextDescriptor
  let surfaceAddress: UInt

  init(extent: NativeRenderTargetExtent, context: NativeVulkanContextDescriptor, surfaceAddress: UInt) {
    self.extent = extent
    self.context = context
    self.surfaceAddress = surfaceAddress
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.vulkanSurfaceDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.surface = UnsafeMutableRawPointer(bitPattern: surfaceAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeTextureImageInfo: Equatable, Sendable {
  let width: UInt32
  let height: UInt32
  let stride: UInt32
  let byteLength: Int

  init(_ raw: mln_texture_image_info) {
    width = raw.width
    height = raw.height
    stride = raw.stride
    byteLength = raw.byte_length
  }
}

struct NativeMetalOwnedTextureDescriptorInput: Equatable, Sendable {
  let extent: NativeRenderTargetExtent
  let context: NativeMetalContextDescriptor

  init(extent: NativeRenderTargetExtent, context: NativeMetalContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_owned_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.metalOwnedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeMetalBorrowedTextureDescriptorInput: Equatable, Sendable {
  let extent: NativeRenderTargetExtent
  let textureAddress: UInt

  init(extent: NativeRenderTargetExtent, textureAddress: UInt) {
    self.extent = extent
    self.textureAddress = textureAddress
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_borrowed_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.metalBorrowedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.texture = UnsafeMutableRawPointer(bitPattern: textureAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeVulkanOwnedTextureDescriptorInput: Equatable, Sendable {
  let extent: NativeRenderTargetExtent
  let context: NativeVulkanContextDescriptor

  init(extent: NativeRenderTargetExtent, context: NativeVulkanContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_owned_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.vulkanOwnedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeVulkanBorrowedTextureDescriptorInput: Equatable, Sendable {
  let extent: NativeRenderTargetExtent
  let context: NativeVulkanContextDescriptor
  let imageAddress: UInt
  let imageViewAddress: UInt
  let format: UInt32
  let initialLayout: UInt32
  let finalLayout: UInt32

  init(
    extent: NativeRenderTargetExtent,
    context: NativeVulkanContextDescriptor,
    imageAddress: UInt,
    imageViewAddress: UInt,
    format: UInt32,
    initialLayout: UInt32,
    finalLayout: UInt32
  ) {
    self.extent = extent
    self.context = context
    self.imageAddress = imageAddress
    self.imageViewAddress = imageViewAddress
    self.format = format
    self.initialLayout = initialLayout
    self.finalLayout = finalLayout
  }

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_borrowed_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.vulkanBorrowedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.image = UnsafeMutableRawPointer(bitPattern: imageAddress)
    descriptor.image_view = UnsafeMutableRawPointer(bitPattern: imageViewAddress)
    descriptor.format = format
    descriptor.initial_layout = initialLayout
    descriptor.final_layout = finalLayout
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeMetalOwnedTextureFrame {
  var raw: mln_metal_owned_texture_frame

  init(_ raw: mln_metal_owned_texture_frame) {
    self.raw = raw
  }
}

struct NativeVulkanOwnedTextureFrame {
  var raw: mln_vulkan_owned_texture_frame

  init(_ raw: mln_vulkan_owned_texture_frame) {
    self.raw = raw
  }
}
