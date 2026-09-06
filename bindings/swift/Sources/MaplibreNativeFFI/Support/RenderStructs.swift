internal import CMaplibreNativeC

struct NativeRenderTargetExtent: Equatable {
  let width: UInt32
  let height: UInt32
  let scaleFactor: Double

  var native: mln_render_target_extent {
    mln_render_target_extent(
      size: UInt32(MemoryLayout<mln_render_target_extent>.size),
      width: width,
      height: height,
      scale_factor: scaleFactor
    )
  }
}

struct NativeMetalContextDescriptor: Equatable {
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

struct NativeVulkanContextDescriptor: Equatable {
  let instanceAddress: UInt
  let physicalDeviceAddress: UInt
  let deviceAddress: UInt
  let graphicsQueueAddress: UInt
  let graphicsQueueFamilyIndex: UInt32
  let getInstanceProcAddrAddress: UInt
  let getDeviceProcAddrAddress: UInt

  init(
    instanceAddress: UInt,
    physicalDeviceAddress: UInt,
    deviceAddress: UInt,
    graphicsQueueAddress: UInt,
    graphicsQueueFamilyIndex: UInt32,
    getInstanceProcAddrAddress: UInt = 0,
    getDeviceProcAddrAddress: UInt = 0
  ) {
    self.instanceAddress = instanceAddress
    self.physicalDeviceAddress = physicalDeviceAddress
    self.deviceAddress = deviceAddress
    self.graphicsQueueAddress = graphicsQueueAddress
    self.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex
    self.getInstanceProcAddrAddress = getInstanceProcAddrAddress
    self.getDeviceProcAddrAddress = getDeviceProcAddrAddress
  }

  var native: mln_vulkan_context_descriptor {
    mln_vulkan_context_descriptor(
      size: UInt32(MemoryLayout<mln_vulkan_context_descriptor>.size),
      instance: UnsafeMutableRawPointer(bitPattern: instanceAddress),
      physical_device: UnsafeMutableRawPointer(
        bitPattern: physicalDeviceAddress
      ),
      device: UnsafeMutableRawPointer(bitPattern: deviceAddress),
      graphics_queue: UnsafeMutableRawPointer(bitPattern: graphicsQueueAddress),
      graphics_queue_family_index: graphicsQueueFamilyIndex,
      get_instance_proc_addr: UnsafeMutableRawPointer(
        bitPattern: getInstanceProcAddrAddress
      ),
      get_device_proc_addr: UnsafeMutableRawPointer(
        bitPattern: getDeviceProcAddrAddress
      )
    )
  }
}

struct NativeWglContextDescriptor: Equatable {
  let deviceContextAddress: UInt
  let shareContextAddress: UInt
  let getProcAddressAddress: UInt

  init(
    deviceContextAddress: UInt,
    shareContextAddress: UInt,
    getProcAddressAddress: UInt = 0
  ) {
    self.deviceContextAddress = deviceContextAddress
    self.shareContextAddress = shareContextAddress
    self.getProcAddressAddress = getProcAddressAddress
  }

  var native: mln_wgl_context_descriptor {
    mln_wgl_context_descriptor(
      size: UInt32(MemoryLayout<mln_wgl_context_descriptor>.size),
      device_context: UnsafeMutableRawPointer(bitPattern: deviceContextAddress),
      share_context: UnsafeMutableRawPointer(bitPattern: shareContextAddress),
      get_proc_address: UnsafeMutableRawPointer(
        bitPattern: getProcAddressAddress
      )
    )
  }
}

struct NativeEglContextDescriptor: Equatable {
  let displayAddress: UInt
  let configAddress: UInt
  let shareContextAddress: UInt
  let clientAPIRawValue: UInt32
  let getProcAddressAddress: UInt

  init(
    displayAddress: UInt,
    configAddress: UInt,
    shareContextAddress: UInt,
    clientAPIRawValue: UInt32 = MLN_OPENGL_CLIENT_API_UNSPECIFIED.rawValue,
    getProcAddressAddress: UInt = 0
  ) {
    self.displayAddress = displayAddress
    self.configAddress = configAddress
    self.shareContextAddress = shareContextAddress
    self.clientAPIRawValue = clientAPIRawValue
    self.getProcAddressAddress = getProcAddressAddress
  }

  var native: mln_egl_context_descriptor {
    mln_egl_context_descriptor(
      size: UInt32(MemoryLayout<mln_egl_context_descriptor>.size),
      display: UnsafeMutableRawPointer(bitPattern: displayAddress),
      config: UnsafeMutableRawPointer(bitPattern: configAddress),
      share_context: UnsafeMutableRawPointer(bitPattern: shareContextAddress),
      client_api: mln_opengl_client_api(rawValue: clientAPIRawValue),
      get_proc_address: UnsafeMutableRawPointer(
        bitPattern: getProcAddressAddress
      )
    )
  }
}

struct NativeWebGLContextDescriptor: Equatable {
  enum Kind: UInt32, Equatable {
    case existing = 0
    case transferredCanvas = 1
  }

  let kind: Kind
  let context: Int32
  let canvasSelector: String

  func withNative<Result>(
    _ body: (mln_webgl_context_descriptor) throws -> Result
  ) throws -> Result {
    try NativeString.withStringView(canvasSelector) { selector in
      try body(mln_webgl_context_descriptor(
        size: UInt32(MemoryLayout<mln_webgl_context_descriptor>.size),
        kind: kind.rawValue,
        context: context,
        canvas_selector: selector
      ))
    }
  }
}

struct NativeOpenGLContextDescriptor: Equatable {
  enum Platform: Equatable {
    case wgl(NativeWglContextDescriptor)
    case egl(NativeEglContextDescriptor)
    case webGL(NativeWebGLContextDescriptor)
  }

  let platform: Platform
  let ownershipRawValue: UInt32

  init(
    platform: Platform,
    ownershipRawValue: UInt32 = MLN_OPENGL_CONTEXT_OWNERSHIP_SHARED.rawValue
  ) {
    self.platform = platform
    self.ownershipRawValue = ownershipRawValue
  }

  func withNative<Result>(
    _ body: (mln_opengl_context_descriptor) throws -> Result
  ) throws -> Result {
    var descriptor = mln_opengl_context_descriptor()
    descriptor.size = UInt32(MemoryLayout<mln_opengl_context_descriptor>.size)
    descriptor.ownership = mln_opengl_context_ownership(
      rawValue: ownershipRawValue
    )
    switch platform {
    case let .wgl(context):
      descriptor.platform = MLN_OPENGL_CONTEXT_PLATFORM_WGL
      descriptor.data.wgl = context.native
      return try body(descriptor)
    case let .egl(context):
      descriptor.platform = MLN_OPENGL_CONTEXT_PLATFORM_EGL
      descriptor.data.egl = context.native
      return try body(descriptor)
    case let .webGL(context):
      return try context.withNative { context in
        descriptor.platform = MLN_OPENGL_CONTEXT_PLATFORM_WEBGL
        descriptor.data.webgl = context
        return try body(descriptor)
      }
    }
  }
}

struct NativeMetalSurfaceDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeMetalContextDescriptor
  let layerAddress: UInt

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = mln_metal_surface_descriptor_default()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.layer = UnsafeMutableRawPointer(bitPattern: layerAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeVulkanSurfaceDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeVulkanContextDescriptor
  let surfaceBits: UInt64

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = mln_vulkan_surface_descriptor_default()
    descriptor.extent = extent.native
    descriptor.context = context.native
    descriptor.surface = surfaceBits
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeOpenGLSurfaceDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeOpenGLContextDescriptor
  let surfaceAddress: UInt

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_opengl_surface_descriptor>) throws -> Result
  ) throws -> Result {
    try context.withNative { context in
      var descriptor = mln_opengl_surface_descriptor_default()
      descriptor.extent = extent.native
      descriptor.context = context
      descriptor.surface = UnsafeMutableRawPointer(bitPattern: surfaceAddress)
      return try withUnsafePointer(to: &descriptor, body)
    }
  }
}

struct NativeTextureImageInfo: Equatable {
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

struct NativeMetalOwnedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeMetalContextDescriptor

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_owned_texture_descriptor>) throws -> Result
  ) throws -> Result {
    var descriptor = mln_metal_owned_texture_descriptor_default()
    descriptor.extent = extent.native
    descriptor.context = context.native
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeMetalBorrowedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let physicalWidth: UInt32
  let physicalHeight: UInt32
  let textureAddress: UInt

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_metal_borrowed_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    var descriptor = mln_metal_borrowed_texture_descriptor_default()
    descriptor.extent = extent.native
    descriptor.physical_width = physicalWidth
    descriptor.physical_height = physicalHeight
    descriptor.texture = UnsafeMutableRawPointer(bitPattern: textureAddress)
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeVulkanOwnedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeVulkanContextDescriptor

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_owned_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    var descriptor = mln_vulkan_owned_texture_descriptor_default()
    descriptor.extent = extent.native
    descriptor.context = context.native
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeVulkanBorrowedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let physicalWidth: UInt32
  let physicalHeight: UInt32
  let context: NativeVulkanContextDescriptor
  let imageBits: UInt64
  let imageViewBits: UInt64
  let format: UInt32
  let initialLayout: UInt32
  let finalLayout: UInt32

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_vulkan_borrowed_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    var descriptor = mln_vulkan_borrowed_texture_descriptor_default()
    descriptor.extent = extent.native
    descriptor.physical_width = physicalWidth
    descriptor.physical_height = physicalHeight
    descriptor.context = context.native
    descriptor.image = imageBits
    descriptor.image_view = imageViewBits
    descriptor.format = format
    descriptor.initial_layout = initialLayout
    descriptor.final_layout = finalLayout
    return try withUnsafePointer(to: &descriptor, body)
  }
}

struct NativeOpenGLOwnedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeOpenGLContextDescriptor

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_opengl_owned_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    try context.withNative { context in
      var descriptor = mln_opengl_owned_texture_descriptor_default()
      descriptor.extent = extent.native
      descriptor.context = context
      return try withUnsafePointer(to: &descriptor, body)
    }
  }
}

struct NativeOpenGLBorrowedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let physicalWidth: UInt32
  let physicalHeight: UInt32
  let context: NativeOpenGLContextDescriptor
  let texture: UInt32
  let target: UInt32

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_opengl_borrowed_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    try context.withNative { context in
      var descriptor = mln_opengl_borrowed_texture_descriptor_default()
      descriptor.extent = extent.native
      descriptor.physical_width = physicalWidth
      descriptor.physical_height = physicalHeight
      descriptor.context = context
      descriptor.texture = texture
      descriptor.target = target
      return try withUnsafePointer(to: &descriptor, body)
    }
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

struct NativeOpenGLOwnedTextureFrame {
  var raw: mln_opengl_owned_texture_frame

  init(_ raw: mln_opengl_owned_texture_frame) {
    self.raw = raw
  }
}

struct NativeWebGPUContextDescriptor: Equatable {
  let instanceAddress: UInt
  let deviceAddress: UInt
  let queueAddress: UInt

  var native: mln_webgpu_context_descriptor {
    mln_webgpu_context_descriptor(
      size: UInt32(MemoryLayout<mln_webgpu_context_descriptor>.size),
      instance: UnsafeMutableRawPointer(bitPattern: instanceAddress),
      device: UnsafeMutableRawPointer(bitPattern: deviceAddress),
      queue: UnsafeMutableRawPointer(bitPattern: queueAddress)
    )
  }
}

struct NativeWebGPUSurfaceDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeWebGPUContextDescriptor
  let surfaceAddress: UInt
  let format: UInt32

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_webgpu_surface_descriptor>) throws -> Result
  ) throws -> Result {
    var value = mln_webgpu_surface_descriptor_default()
    value.extent = extent.native
    value.context = context.native
    value.surface = UnsafeMutableRawPointer(bitPattern: surfaceAddress)
    value.format = format
    return try withUnsafePointer(to: &value, body)
  }
}

struct NativeWebGPUOwnedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let context: NativeWebGPUContextDescriptor

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_webgpu_owned_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    var value = mln_webgpu_owned_texture_descriptor_default()
    value.extent = extent.native
    value.context = context.native
    return try withUnsafePointer(to: &value, body)
  }
}

struct NativeWebGPUBorrowedTextureDescriptorInput: Equatable {
  let extent: NativeRenderTargetExtent
  let physicalWidth: UInt32
  let physicalHeight: UInt32
  let context: NativeWebGPUContextDescriptor
  let textureAddress: UInt
  let textureViewAddress: UInt
  let format: UInt32

  func withNativeDescriptor<Result>(
    _ body: (UnsafePointer<mln_webgpu_borrowed_texture_descriptor>) throws
      -> Result
  ) throws -> Result {
    var value = mln_webgpu_borrowed_texture_descriptor_default()
    value.extent = extent.native
    value.physical_width = physicalWidth
    value.physical_height = physicalHeight
    value.context = context.native
    value.texture = UnsafeMutableRawPointer(bitPattern: textureAddress)
    value.texture_view = UnsafeMutableRawPointer(bitPattern: textureViewAddress)
    value.format = format
    return try withUnsafePointer(to: &value, body)
  }
}
