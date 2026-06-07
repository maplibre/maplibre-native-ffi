
internal import CMaplibreNativeC

public enum Maplibre {
  public static func cVersion() -> UInt32 {
    mln_c_version()
  }

  public static func supportedRenderBackends() -> RenderBackend {
    RenderBackend(rawValue: mln_supported_render_backend_mask())
  }

  public static func supportedOpenGLContextProviders() -> OpenGLContextProvider {
    OpenGLContextProvider(rawValue: mln_opengl_supported_context_provider_mask())
  }

  public static func networkStatus() throws -> NetworkStatus {
    try mapNativeFailure {
      let rawStatus = try NativeMemory.withTemporary(UInt32(0)) { rawStatus in
        try checkStatus(mln_network_status_get(rawStatus))
      }
      return NetworkStatus.fromNative(rawStatus.value)
    }
  }

  public static func setNetworkStatus(_ status: NetworkStatus) throws {
    let rawStatus = try status.nativeValue
    try mapNativeFailure {
      try checkStatus(mln_network_status_set(rawStatus))
    }
  }
}
