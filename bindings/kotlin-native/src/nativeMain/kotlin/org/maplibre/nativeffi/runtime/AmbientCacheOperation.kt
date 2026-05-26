package org.maplibre.nativeffi.runtime

/** Ambient cache maintenance operation for a runtime. */
public enum class AmbientCacheOperation(public val nativeValue: Int) {
  RESET_DATABASE(1),
  PACK_DATABASE(2),
  INVALIDATE(3),
  CLEAR(4),
}
