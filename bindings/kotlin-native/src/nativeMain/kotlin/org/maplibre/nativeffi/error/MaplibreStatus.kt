package org.maplibre.nativeffi.error

/** Status categories reported by the native MapLibre C ABI. */
public enum class MaplibreStatus(public val nativeCode: Int) {
  OK(0),
  INVALID_ARGUMENT(-1),
  INVALID_STATE(-2),
  WRONG_THREAD(-3),
  UNSUPPORTED(-4),
  NATIVE_ERROR(-5),
  UNKNOWN(Int.MIN_VALUE);

  public companion object {
    /** Returns the Kotlin status category for a C ABI status value. */
    public fun fromNative(nativeCode: Int): MaplibreStatus =
      when (nativeCode) {
        0 -> OK
        -1 -> INVALID_ARGUMENT
        -2 -> INVALID_STATE
        -3 -> WRONG_THREAD
        -4 -> UNSUPPORTED
        -5 -> NATIVE_ERROR
        else -> UNKNOWN
      }
  }
}
