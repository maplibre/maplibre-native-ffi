package org.maplibre.nativeffi.error

import org.maplibre.nativeffi.generated.Status

/** Error for a loaded native library with an unsupported C ABI version. */
public class AbiVersionMismatchException(
  public val actualVersion: Long,
  public val expectedVersion: Long,
) :
  MaplibreException(
    Status.NATIVE_ERROR,
    "Unsupported Maplibre C ABI version $actualVersion; expected $expectedVersion",
  )
