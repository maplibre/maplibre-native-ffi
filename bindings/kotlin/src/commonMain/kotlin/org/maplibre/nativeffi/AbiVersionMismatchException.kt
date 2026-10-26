package org.maplibre.nativeffi

/**
 * Error for a loaded native library with an unsupported C ABI version. The status is
 * [Status.UNSUPPORTED].
 */
public class AbiVersionMismatchException
internal constructor(
  public val actualVersion: Long,
  public val expectedVersion: Long,
) :
  MaplibreException(
    Status.UNSUPPORTED,
    "unsupported MapLibre Native C ABI version $actualVersion; expected $expectedVersion",
  )
