package org.maplibre.nativeffi.runtime

/** Ambient cache maintenance operation for a runtime. */
public enum class AmbientCacheOperation(public val nativeValue: UInt) {
  RESET_DATABASE(1U),
  PACK_DATABASE(2U),
  INVALIDATE(3U),
  CLEAR(4U),
}
