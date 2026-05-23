import CMaplibreNativeC

public enum CAPI {
  public static func cVersion() -> UInt32 {
    mln_c_version()
  }

  public static func supportedRenderBackendMask() -> UInt32 {
    mln_supported_render_backend_mask()
  }

  public static func networkStatus() throws -> UInt32 {
    let output = try NativeMemory.withTemporary(UInt32(0)) { rawStatus in
      try checkStatus(mln_network_status_get(rawStatus))
    }
    return output.value
  }

  public static func setNetworkStatus(_ rawStatus: UInt32) throws {
    try checkStatus(mln_network_status_set(rawStatus))
  }

  public static func setLogCallback(
    _ callback: mln_log_callback?,
    userData: UnsafeMutableRawPointer?
  ) throws {
    try checkStatus(mln_log_set_callback(callback, userData))
  }

  public static func clearLogCallback() throws {
    try checkStatus(mln_log_clear_callback())
  }

  public static func setAsyncLogSeverityMask(_ mask: UInt32) throws {
    try checkStatus(mln_log_set_async_severity_mask(mask))
  }
}
