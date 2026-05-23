package org.maplibre.nativeffi.resource

/** Copied network resource request passed to a runtime resource provider. */
public class ResourceRequest(
  public val url: String,
  public val kind: ResourceKind,
  public val rawKind: UInt,
  public val loadingMethod: ResourceLoadingMethod,
  public val rawLoadingMethod: UInt,
  public val priority: ResourcePriority,
  public val rawPriority: UInt,
  public val usage: ResourceUsage,
  public val rawUsage: UInt,
  public val storagePolicy: ResourceStoragePolicy,
  public val rawStoragePolicy: UInt,
  public val range: ByteRange?,
  public val priorModifiedUnixMs: Long?,
  public val priorExpiresUnixMs: Long?,
  public val priorEtag: String?,
  priorData: ByteArray,
) {
  public val priorData: ByteArray = priorData.copyOf()

  public data class ByteRange(public val start: ULong, public val end: ULong)
}
