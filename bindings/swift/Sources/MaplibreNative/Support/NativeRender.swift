internal import CMaplibreNativeC

enum NativeRender {
  static func metalSurfaceAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_metal_surface_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_metal_surface_attach returned a null session") { session in
      try checkStatus(mln_metal_surface_attach(map, descriptor, session))
    }
  }

  static func vulkanSurfaceAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_vulkan_surface_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_vulkan_surface_attach returned a null session") { session in
      try checkStatus(mln_vulkan_surface_attach(map, descriptor, session))
    }
  }

  static func openGLSurfaceAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_opengl_surface_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_opengl_surface_attach returned a null session") { session in
      try checkStatus(mln_opengl_surface_attach(map, descriptor, session))
    }
  }

  static func textureReadPremultipliedRGBA8(
    session: OpaquePointer,
    data: UnsafeMutablePointer<UInt8>?,
    capacity: Int
  ) throws -> mln_texture_image_info {
    var info = mln_texture_image_info_default()
    try checkStatus(mln_texture_read_premultiplied_rgba8(session, data, capacity, &info))
    return info
  }

  static func metalOwnedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_metal_owned_texture_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_metal_owned_texture_attach returned a null session") { session in
      try checkStatus(mln_metal_owned_texture_attach(map, descriptor, session))
    }
  }

  static func metalBorrowedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_metal_borrowed_texture_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_metal_borrowed_texture_attach returned a null session") { session in
      try checkStatus(mln_metal_borrowed_texture_attach(map, descriptor, session))
    }
  }

  static func vulkanOwnedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_vulkan_owned_texture_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_vulkan_owned_texture_attach returned a null session") { session in
      try checkStatus(mln_vulkan_owned_texture_attach(map, descriptor, session))
    }
  }

  static func vulkanBorrowedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_vulkan_borrowed_texture_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_vulkan_borrowed_texture_attach returned a null session") { session in
      try checkStatus(mln_vulkan_borrowed_texture_attach(map, descriptor, session))
    }
  }

  static func openGLOwnedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_opengl_owned_texture_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_opengl_owned_texture_attach returned a null session") { session in
      try checkStatus(mln_opengl_owned_texture_attach(map, descriptor, session))
    }
  }

  static func openGLBorrowedTextureAttach(
    map: OpaquePointer,
    descriptor: UnsafePointer<mln_opengl_borrowed_texture_descriptor>
  ) throws -> OpaquePointer {
    try NativeHandleFactory.create(nullDiagnostic: "mln_opengl_borrowed_texture_attach returned a null session") { session in
      try checkStatus(mln_opengl_borrowed_texture_attach(map, descriptor, session))
    }
  }

  static func metalOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_metal_owned_texture_frame {
    var frame = mln_metal_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_metal_owned_texture_frame>.size)
    try checkStatus(mln_metal_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  static func vulkanOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_vulkan_owned_texture_frame {
    var frame = mln_vulkan_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_vulkan_owned_texture_frame>.size)
    try checkStatus(mln_vulkan_owned_texture_acquire_frame(session, &frame))
    return frame
  }

  static func openGLOwnedTextureAcquireFrame(_ session: OpaquePointer) throws -> mln_opengl_owned_texture_frame {
    var frame = mln_opengl_owned_texture_frame()
    frame.size = UInt32(MemoryLayout<mln_opengl_owned_texture_frame>.size)
    try checkStatus(mln_opengl_owned_texture_acquire_frame(session, &frame))
    return frame
  }
}
