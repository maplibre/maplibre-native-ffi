import MaplibreNativeSupport

public enum Maplibre {
  public static func cVersion() -> UInt32 {
    CAPI.cVersion()
  }

  public static func supportedRenderBackends() -> RenderBackend {
    RenderBackend(rawValue: CAPI.supportedRenderBackendMask())
  }

  public static func networkStatus() throws -> NetworkStatus {
    try mapNativeFailure {
      NetworkStatus.fromNative(try CAPI.networkStatus())
    }
  }

  public static func setNetworkStatus(_ status: NetworkStatus) throws {
    let rawStatus = try status.nativeValue
    try mapNativeFailure {
      try CAPI.setNetworkStatus(rawStatus)
    }
  }
}
