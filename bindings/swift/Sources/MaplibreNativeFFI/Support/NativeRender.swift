internal import CMaplibreNativeC

enum NativeRender {
  typealias Attachment = (
    session: NativeRenderSessionHandle,
    completion: NativeFuture<Void>
  )

  /// Starts one render-session attachment. The C API publishes the session
  /// synchronously and reports the attachment itself through the completion.
  static func attachment(
    _ body: (UnsafeMutablePointer<mln_render_session>,
             UnsafePointer<mln_completion>) -> mln_status
  ) throws -> Attachment {
    var session: mln_render_session = 0
    let completion = try NativeCompletion.startUnit { completion in
      body(&session, completion)
    }
    guard session != 0 else {
      throw NativeStatusFailure.swiftNativeError(
        "render attachment returned a null session"
      )
    }
    return (NativeRenderSessionHandle(raw: session), completion)
  }
}
