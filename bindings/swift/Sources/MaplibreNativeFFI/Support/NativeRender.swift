internal import CMaplibreNativeC

enum NativeRender {
  typealias Attachment = (
    session: NativeRenderSessionHandle,
    operation: NativeOperationHandle
  )

  private static func attachment(
    _ body: (UnsafeMutablePointer<mln_render_session>,
             UnsafeMutablePointer<mln_operation>) throws -> Void
  ) throws -> Attachment {
    var session: mln_render_session = 0
    var operation: mln_operation = 0
    try body(&session, &operation)
    guard session != 0, operation != 0 else {
      if operation != 0 { mln_operation_release(operation) }
      if session != 0 { _ = mln_render_session_destroy(session) }
      throw NativeStatusFailure.swiftNativeError(
        "render attachment returned a null session or operation"
      )
    }
    return (
      NativeRenderSessionHandle(raw: session),
      NativeOperationHandle(raw: operation)
    )
  }

  static func metalSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_metal_surface_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func vulkanSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_vulkan_surface_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func openGLSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_opengl_surface_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func metalOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_metal_owned_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func metalBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_metal_borrowed_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func vulkanOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_vulkan_owned_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func vulkanBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_vulkan_borrowed_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func openGLOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_opengl_owned_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func openGLBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_opengl_borrowed_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func webGPUSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_webgpu_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_webgpu_surface_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func webGPUOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_webgpu_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_webgpu_owned_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }

  static func webGPUBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_webgpu_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, operation in
      try checkStatus(mln_webgpu_borrowed_texture_attach_start(
        map.raw, descriptor, options, session, operation
      ))
    }
  }
}
