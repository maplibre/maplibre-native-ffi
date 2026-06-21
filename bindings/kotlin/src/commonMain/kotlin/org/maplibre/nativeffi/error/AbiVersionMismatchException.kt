package org.maplibre.nativeffi.error

/** Error for a loaded native library with an unsupported C ABI version. */
public class AbiVersionMismatchException(
  public val actualVersion: Long,
  public val expectedVersion: Long,
) :
  NativeErrorException(
    MaplibreStatus.NATIVE_ERROR.nativeCode,
    "Unsupported Maplibre C ABI version $actualVersion; expected $expectedVersion",
  )
