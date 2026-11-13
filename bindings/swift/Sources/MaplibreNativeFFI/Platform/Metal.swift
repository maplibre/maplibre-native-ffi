#if canImport(Metal)
  import Metal

  // Typed forms of the Metal objects that records carry as pointers. An input
  // keeps its object alive while the record exists, which covers the call that
  // retains it; an output borrows the object that native lends until the
  // frame's release.

  public extension MetalDeviceDescriptor {
    /// Your device, and optionally your command queue on it, for
    /// ``GpuDeviceHandle/wrapMetal(descriptor:)``. The wrapped device retains
    /// both before the call returns.
    init(device: any MTLDevice, commandQueue: (any MTLCommandQueue)? = nil) {
      self.init(
        device: NativePointer(retaining: device),
        commandQueue: commandQueue.map { NativePointer(retaining: $0) } ?? .null
      )
    }
  }

  public extension MetalBorrowedTexture {
    /// One texture of a borrowed ring. Native doesn't retain it, so the host
    /// keeps it alive until a target replacement replaces it, a detach
    /// completes, or an abandon returns.
    init(texture: any MTLTexture) {
      self.init(texture: NativePointer(retaining: texture))
    }
  }

  public extension MetalTextureFrameView {
    /// The frame's texture, valid until the frame's release.
    var mtlTexture: any MTLTexture {
      get throws { try texture.unretainedObject() as! any MTLTexture }
    }

    /// The device of the frame's texture.
    var mtlDevice: any MTLDevice {
      get throws { try device.unretainedObject() as! any MTLDevice }
    }

    var mtlPixelFormat: MTLPixelFormat {
      MTLPixelFormat(rawValue: UInt(pixelFormat))!
    }

    /// The texture that holds the frame's depth and stencil, valid until the
    /// frame's release, or nil when the frame has none.
    var mtlDepthTexture: (any MTLTexture)? {
      get throws { try depthTexture.unretainedObject() as? any MTLTexture }
    }

    /// The depth texture's pixel format, or `.invalid` when the frame has
    /// none.
    var mtlDepthPixelFormat: MTLPixelFormat {
      MTLPixelFormat(rawValue: UInt(depthPixelFormat)) ?? .invalid
    }
  }
#endif

#if canImport(QuartzCore) && canImport(Metal)
  import QuartzCore

  public extension MetalLayerTarget {
    /// A target that presents to `layer`. The target keeps the layer alive
    /// through the attach or target replacement that takes it; keep it alive
    /// yourself while the session presents to it.
    init(logicalSize: LogicalSize, layer: CAMetalLayer) {
      self.init(
        logicalSize: logicalSize,
        layer: NativePointer(retaining: layer)
      )
    }
  }
#endif
