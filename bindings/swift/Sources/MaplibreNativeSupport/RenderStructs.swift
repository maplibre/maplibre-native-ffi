import CMaplibreNativeC

public struct NativeRenderTargetExtent: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let scaleFactor: Double

  public init(width: UInt32, height: UInt32, scaleFactor: Double) {
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

public struct NativeMetalContextDescriptor: Equatable, Sendable {
  public let deviceAddress: UInt

  public init(deviceAddress: UInt = 0) {
    self.deviceAddress = deviceAddress
  }

  var native: mln_metal_context_descriptor {
    mln_metal_context_descriptor(
      size: UInt32(MemoryLayout<mln_metal_context_descriptor>.size),
      device: UnsafeMutableRawPointer(bitPattern: deviceAddress)
    )
  }
}

public struct NativeVulkanContextDescriptor: Equatable, Sendable {
  public let instanceAddress: UInt
  public let physicalDeviceAddress: UInt
  public let deviceAddress: UInt
  public let graphicsQueueAddress: UInt
  public let graphicsQueueFamilyIndex: UInt32

  public init(
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

public struct NativeMetalSurfaceDescriptorInput: Equatable, Sendable {
  public let extent: NativeRenderTargetExtent
  public let context: NativeMetalContextDescriptor
  public let layerAddress: UInt

  public init(extent: NativeRenderTargetExtent, context: NativeMetalContextDescriptor, layerAddress: UInt) {
    self.extent = extent
    self.context = context
    self.layerAddress = layerAddress
  }

  public func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.metalSurfaceDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.layer = UnsafeMutableRawPointer(bitPattern: layerAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

public struct NativeVulkanSurfaceDescriptorInput: Equatable, Sendable {
  public let extent: NativeRenderTargetExtent
  public let context: NativeVulkanContextDescriptor
  public let surfaceAddress: UInt

  public init(extent: NativeRenderTargetExtent, context: NativeVulkanContextDescriptor, surfaceAddress: UInt) {
    self.extent = extent
    self.context = context
    self.surfaceAddress = surfaceAddress
  }

  public func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.vulkanSurfaceDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.surface = UnsafeMutableRawPointer(bitPattern: surfaceAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

public struct NativeTextureImageInfo: Equatable, Sendable {
  public let width: UInt32
  public let height: UInt32
  public let stride: UInt32
  public let byteLength: Int

  public init(_ raw: mln_texture_image_info) {
    width = raw.width
    height = raw.height
    stride = raw.stride
    byteLength = raw.byte_length
  }
}

public struct NativeMetalOwnedTextureDescriptorInput: Equatable, Sendable {
  public let extent: NativeRenderTargetExtent
  public let context: NativeMetalContextDescriptor

  public init(extent: NativeRenderTargetExtent, context: NativeMetalContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  public func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_owned_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.metalOwnedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    return try withUnsafePointer(to: &descriptor, body)
  }
}

public struct NativeMetalBorrowedTextureDescriptorInput: Equatable, Sendable {
  public let extent: NativeRenderTargetExtent
  public let textureAddress: UInt

  public init(extent: NativeRenderTargetExtent, textureAddress: UInt) {
    self.extent = extent
    self.textureAddress = textureAddress
  }

  public func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_borrowed_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.metalBorrowedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.texture = UnsafeMutableRawPointer(bitPattern: textureAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

public struct NativeVulkanOwnedTextureDescriptorInput: Equatable, Sendable {
  public let extent: NativeRenderTargetExtent
  public let context: NativeVulkanContextDescriptor

  public init(extent: NativeRenderTargetExtent, context: NativeVulkanContextDescriptor) {
    self.extent = extent
    self.context = context
  }

  public func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_owned_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = CAPI.vulkanOwnedTextureDescriptorDefault()
    descriptor.extent = extent.native
    descriptor.context = context.native
    return try withUnsafePointer(to: &descriptor, body)
  }
}

public struct NativeVulkanBorrowedTextureDescriptorInput: Equatable, Sendable {
  public let extent: NativeRenderTargetExtent
  public let context: NativeVulkanContextDescriptor
  public let imageAddress: UInt
  public let imageViewAddress: UInt
  public let format: UInt32
  public let initialLayout: UInt32
  public let finalLayout: UInt32

  public init(
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

  public func withNativeDescriptor<Result>(
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

public struct NativeMetalOwnedTextureFrame {
  public var raw: mln_metal_owned_texture_frame

  public init(_ raw: mln_metal_owned_texture_frame) {
    self.raw = raw
  }
}

public struct NativeVulkanOwnedTextureFrame {
  public var raw: mln_vulkan_owned_texture_frame

  public init(_ raw: mln_vulkan_owned_texture_frame) {
    self.raw = raw
  }
}
