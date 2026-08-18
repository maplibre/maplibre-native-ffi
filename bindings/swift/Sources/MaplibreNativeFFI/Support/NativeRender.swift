internal import CMaplibreNativeC

enum NativeRender {
  typealias Attachment = (
    session: NativeRenderSessionHandle,
    completion: NativeFuture<Void>
  )

  private static func attachment(
    _ body: (UnsafeMutablePointer<mln_render_session>,
             UnsafePointer<mln_completion>) -> mln_status
  ) throws -> Attachment {
    var session: mln_render_session = 0
    let completion = try NativeCompletion.startUnit { completion in
      body(&session, completion)
    }
    guard session != 0 else {
      if session != 0 { _ = mln_render_session_destroy(session) }
      throw NativeStatusFailure.swiftNativeError(
        "render attachment returned a null session"
      )
    }
    return (
      NativeRenderSessionHandle(raw: session),
      completion
    )
  }

  static func metalSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_metal_surface_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func vulkanSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_vulkan_surface_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func openGLSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_opengl_surface_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func metalOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_metal_owned_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func metalBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_metal_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_metal_borrowed_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func vulkanOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_vulkan_owned_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func vulkanBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_vulkan_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_vulkan_borrowed_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func openGLOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_opengl_owned_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func openGLBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_opengl_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_opengl_borrowed_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func webGPUSurfaceAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_webgpu_surface_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_webgpu_surface_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func webGPUOwnedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_webgpu_owned_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_webgpu_owned_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }

  static func webGPUBorrowedTextureAttachStart(
    map: NativeMapHandle,
    descriptor: UnsafePointer<mln_webgpu_borrowed_texture_descriptor>,
    options: UnsafePointer<mln_render_session_attach_options>
  ) throws -> Attachment {
    try attachment { session, completion in
      mln_webgpu_borrowed_texture_attach(
        map.raw,
        descriptor,
        options,
        session,
        completion
      )
    }
  }
}
