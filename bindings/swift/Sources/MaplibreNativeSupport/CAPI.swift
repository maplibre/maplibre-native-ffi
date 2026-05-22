import CMaplibreNativeC

public enum CAPI {
  public static func cVersion() -> UInt32 {
    mln_c_version()
  }

  public static func supportedRenderBackendMask() -> UInt32 {
    mln_supported_render_backend_mask()
  }

  public static func networkStatus() throws -> UInt32 {
    var rawStatus: UInt32 = 0
    try checkStatus(mln_network_status_get(&rawStatus))
    return rawStatus
  }

  public static func setNetworkStatus(_ rawStatus: UInt32) throws {
    try checkStatus(mln_network_status_set(rawStatus))
  }
}
