package org.maplibre.nativeffi.error

/** Base unchecked exception for errors reported by the native MapLibre C ABI. */
public open class MaplibreException
internal constructor(
  public val status: MaplibreStatus,
  public val nativeStatusCode: Int,
  public val diagnostic: String = "",
) : RuntimeException(message(status, nativeStatusCode, diagnostic)) {
  internal companion object {
    /** Creates the stable exception subtype for a native status category. */
    fun forStatus(
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
        else -> MaplibreException(status, nativeStatusCode, diagnostic)
      }
  }
}

private fun message(status: MaplibreStatus, nativeStatusCode: Int, diagnostic: String): String {
  val detail = diagnostic.ifBlank { "No native diagnostic available." }
  return "${statusLabel(status)} ($nativeStatusCode): $detail"
}

private fun statusLabel(status: MaplibreStatus): String =
  when (status) {
    MaplibreStatus.OK -> "OK"
    MaplibreStatus.INVALID_ARGUMENT -> "INVALID_ARGUMENT"
    MaplibreStatus.INVALID_STATE -> "INVALID_STATE"
    MaplibreStatus.WRONG_THREAD -> "WRONG_THREAD"
    MaplibreStatus.UNSUPPORTED -> "UNSUPPORTED"
    MaplibreStatus.NATIVE_ERROR -> "NATIVE_ERROR"
    else -> "MaplibreStatus(${status.nativeCode})"
  }
