internal import CMaplibreNativeC

enum NativeRender {
  static func metalSurfaceAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_surface_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_metal_surface_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_metal_surface_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func vulkanSurfaceAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_surface_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_vulkan_surface_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_vulkan_surface_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func openGLSurfaceAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_surface_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_opengl_surface_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_opengl_surface_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func textureReadPremultipliedRGBA8(
    session: NativeRenderSessionHandle,
    data: UnsafeMutablePointer<UInt8>?,
    capacity: Int
  ) throws -> mln_texture_image_info {
    var info = mln_texture_image_info_default()
    try checkStatus(mln_texture_read_premultiplied_rgba8(
      session.raw,
      data,
      capacity,
      &info
    ))
    return info
  }

  static func metalOwnedTextureAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_owned_texture_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_metal_owned_texture_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_metal_owned_texture_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func metalBorrowedTextureAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_borrowed_texture_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_metal_borrowed_texture_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_metal_borrowed_texture_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func vulkanOwnedTextureAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_owned_texture_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_vulkan_owned_texture_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_vulkan_owned_texture_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func vulkanBorrowedTextureAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_borrowed_texture_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_vulkan_borrowed_texture_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_vulkan_borrowed_texture_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func openGLOwnedTextureAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_owned_texture_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_opengl_owned_texture_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_opengl_owned_texture_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func openGLBorrowedTextureAttach(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_borrowed_texture_descriptor>
  ) throws -> NativeRenderSessionHandle {
    try NativeHandleFactory
      .create(
        nullDiagnostic: "mln_opengl_borrowed_texture_attach returned a null session"
      ) { outHandle in
        try checkStatus(mln_opengl_borrowed_texture_attach(
          map.raw,
          descriptor,
          outHandle
        ))
      }
  }

  static func metalOwnedTextureAcquireFrame(
    _ session: NativeRenderSessionHandle
  ) throws
    -> mln_metal_owned_texture_frame
  {
    var frame = mln_metal_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
    try checkStatus(mln_metal_owned_texture_acquire_frame(session.raw, &frame))
    return frame
  }

  static func vulkanOwnedTextureAcquireFrame(
    _ session: NativeRenderSessionHandle
  ) throws
    -> mln_vulkan_owned_texture_frame
  {
    var frame = mln_vulkan_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
    try checkStatus(mln_vulkan_owned_texture_acquire_frame(session.raw, &frame))
    return frame
  }

  static func openGLOwnedTextureAcquireFrame(
    _ session: NativeRenderSessionHandle
  ) throws
    -> mln_opengl_owned_texture_frame
  {
    var frame = mln_opengl_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
    try checkStatus(mln_opengl_owned_texture_acquire_frame(session.raw, &frame))
    return frame
  }
}
