package org.maplibre.nativeffi.error

/** Base unchecked exception for errors reported by the native MapLibre C ABI. */
public open class MaplibreException(
  public val status: MaplibreStatus,
  public val nativeStatusCode: Int,
  public val diagnostic: String = "",
) : RuntimeException(message(status, nativeStatusCode, diagnostic)) {
  public companion object {
    /** Creates the stable exception subtype for a native status category. */
    public fun forStatus(
      status: MaplibreStatus,
      nativeStatusCode: Int,
      diagnostic: String = "",
    ): MaplibreException =
      when (status) {
        MaplibreStatus.INVALID_ARGUMENT -> InvalidArgumentException(nativeStatusCode, diagnostic)
        MaplibreStatus.INVALID_STATE -> InvalidStateException(nativeStatusCode, diagnostic)
        MaplibreStatus.WRONG_THREAD -> WrongThreadException(nativeStatusCode, diagnostic)
        MaplibreStatus.UNSUPPORTED -> UnsupportedFeatureException(nativeStatusCode, diagnostic)
        MaplibreStatus.NATIVE_ERROR -> NativeErrorException(nativeStatusCode, diagnostic)
        MaplibreStatus.OK,
        MaplibreStatus.UNKNOWN -> MaplibreException(status, nativeStatusCode, diagnostic)
      }
  }
}

private fun message(status: MaplibreStatus, nativeStatusCode: Int, diagnostic: String): String {
  val detail = diagnostic.ifBlank { "No native diagnostic available." }
  return "$status ($nativeStatusCode): $detail"
}
