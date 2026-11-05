package org.maplibre.nativeffi

import org.maplibre.nativeffi.internal.JvmSynthetic

/**
 * Error for a loaded native library with an unsupported C ABI version. The status is
 * [Status.UNSUPPORTED].
 */
public class AbiVersionMismatchException
private constructor(
  public val actualVersion: Long,
  public val expectedVersion: Long,
) :
  MaplibreException(
    Status.UNSUPPORTED,
    "unsupported MapLibre Native C ABI version $actualVersion; expected $expectedVersion",
  ) {
  internal companion object {
    @JvmSynthetic
    internal fun create(actualVersion: Long, expectedVersion: Long): AbiVersionMismatchException =
      AbiVersionMismatchException(actualVersion, expectedVersion)
  }
}
