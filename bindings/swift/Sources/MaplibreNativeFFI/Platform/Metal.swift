#if canImport(Metal)
  import Metal

  // Typed forms of the Metal objects that records carry as pointers. An input
  // keeps its object alive while the record exists, which covers the attach
  // or target call that retains it; an output borrows the object that native
  // lends until the frame's release.

  public extension MetalContextDescriptor {
    /// A context on `device`. An attach that needs the device retains it
    /// before it returns.
    init(device: any MTLDevice) {
      self.init(device: NativePointer(retaining: device))
    }
  }

  public extension MetalOwnedTextureDescriptor {
    init(
      extent: LogicalExtent = MetalOwnedTextureDescriptor.default.extent,
      context: MetalContextDescriptor,
      pixelFormat: MTLPixelFormat
    ) {
      self.init(
        extent: extent,
        context: context,
        pixelFormat: UInt64(pixelFormat.rawValue)
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
  }
#endif

#if canImport(QuartzCore) && canImport(Metal)
  import QuartzCore

  public extension MetalSurfaceDescriptor {
    /// A surface that draws into `layer`, which an accepted attach retains
    /// before it returns.
    init(
      extent: LogicalExtent = MetalSurfaceDescriptor.default.extent,
      context: MetalContextDescriptor = MetalSurfaceDescriptor.default.context,
      layer: CAMetalLayer
    ) {
      self.init(
        extent: extent,
        context: context,
        layer: NativePointer(retaining: layer)
      )
    }
  }
#endif
