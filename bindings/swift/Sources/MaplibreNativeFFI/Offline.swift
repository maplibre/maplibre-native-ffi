internal import CMaplibreNativeC
import Foundation

public enum AmbientCacheOperation: UInt32, Sendable, Hashable {
  case resetDatabase = 1
  case packDatabase = 2
  case invalidate = 3
  case clear = 4
}

public enum OfflineRegionDownloadState: UInt32, Sendable, Hashable {
  case inactive = 0
  case active = 1
}

public enum OfflineRegionDefinition: Equatable, Sendable {
  case tilePyramid(
    styleURL: String,
    bounds: LatLngBounds,
    minZoom: Double,
    maxZoom: Double,
    pixelRatio: Float,
    includeIdeographs: Bool
  )
  case geometry(
    styleURL: String,
    geometry: Data,
    minZoom: Double,
    maxZoom: Double,
    pixelRatio: Float,
    includeIdeographs: Bool
  )

  var nativeDefinition: NativeOfflineRegionDefinition {
    switch self {
    case let .tilePyramid(
      styleURL,
      bounds,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      .tilePyramid(
        styleURL: styleURL,
        bounds: bounds.nativeInput,
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    case let .geometry(
      styleURL,
      geometry,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      .geometry(
        styleURL: styleURL,
        geometry: geometry,
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    }
  }

  init(native: NativeOfflineRegionDefinition) {
    switch native {
    case let .tilePyramid(
      styleURL,
      bounds,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      self = .tilePyramid(
        styleURL: styleURL,
        bounds: LatLngBounds(native: bounds),
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    case let .geometry(
      styleURL,
      geometry,
      minZoom,
      maxZoom,
      pixelRatio,
      includeIdeographs
    ):
      self = .geometry(
        styleURL: styleURL,
        geometry: geometry,
        minZoom: minZoom,
        maxZoom: maxZoom,
        pixelRatio: pixelRatio,
        includeIdeographs: includeIdeographs
      )
    }
  }
}

public struct OfflineRegionInfo: Equatable, Sendable {
  public let id: Int64
  public let definition: OfflineRegionDefinition
  public let metadata: Data

  init(native: NativeOfflineRegionInfo) {
    id = native.id
    definition = OfflineRegionDefinition(native: native.definition)
    metadata = native.metadata
  }
}

public extension RuntimeHandle {
  /// Runs one maintenance operation over this runtime's ambient cache.
  ///
  /// The call validates its arguments on the calling thread and accepts
  /// without waiting for the runtime worker; a database failure arrives
  /// through the completion.
  func runAmbientCacheOperation(
    _ operation: AmbientCacheOperation
  ) async throws {
    try await offlineUnit {
      mln_runtime_run_ambient_cache_operation($0, operation.rawValue, $1)
    }
  }

  /// Changes this runtime's maximum ambient cache size. MapLibre evicts
  /// ambient resources to fit the new budget; offline regions are unaffected.
  func setMaximumAmbientCacheSize(_ size: UInt64) async throws {
    try await offlineUnit {
      mln_runtime_set_maximum_ambient_cache_size($0, size, $1)
    }
  }

  /// Creates one offline region and copies the record the completion borrows.
  func createOfflineRegion(
    definition: OfflineRegionDefinition,
    metadata: Data = Data()
  ) async throws -> OfflineRegionInfo {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try definition.nativeDefinition
        .withNativeDefinition { definition in
          try metadata.withUnsafeBytes { bytes in
            try NativeCompletion.start(
              { completion in
                mln_runtime_offline_region_create(
                  runtime.raw,
                  definition,
                  bytes.bindMemory(to: UInt8.self).baseAddress,
                  bytes.count,
                  completion
                )
              },
              convert: Self.copyOfflineRegion
            )
          }
        }
    }
  }

  /// Reads one offline region, or nil when no region carries `id`.
  func offlineRegion(id: Int64) async throws -> OfflineRegionInfo? {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try NativeCompletion.start(
        { mln_runtime_offline_region_get(runtime.raw, id, $0) },
        convert: Self.copyOptionalOfflineRegion
      )
    }
  }

  func offlineRegions() async throws -> [OfflineRegionInfo] {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try NativeCompletion.start(
        { mln_runtime_offline_regions_list(runtime.raw, $0) },
        convert: Self.copyOfflineRegions
      )
    }
  }

  /// Merges the regions of another offline database into this runtime's and
  /// returns the records it took on.
  func mergeOfflineRegionsDatabase(
    at sideDatabasePath: String
  ) async throws -> [OfflineRegionInfo] {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try NativeString.withCString(sideDatabasePath) { path in
        try NativeCompletion.start(
          { mln_runtime_offline_regions_merge_database(runtime.raw, path, $0) },
          convert: Self.copyOfflineRegions
        )
      }
    }
  }

  /// Replaces one region's metadata. It reports
  /// ``MaplibreErrorKind/notFound`` when no region carries `id`.
  func updateOfflineRegionMetadata(
    id: Int64,
    metadata: Data
  ) async throws -> OfflineRegionInfo {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try metadata.withUnsafeBytes { bytes in
        try NativeCompletion.start(
          { completion in
            mln_runtime_offline_region_update_metadata(
              runtime.raw,
              id,
              bytes.bindMemory(to: UInt8.self).baseAddress,
              bytes.count,
              completion
            )
          },
          convert: Self.copyOfflineRegion
        )
      }
    }
  }

  /// Reads one region's download progress. It reports
  /// ``MaplibreErrorKind/notFound`` when no region carries `id`.
  func offlineRegionStatus(id: Int64) async throws -> OfflineRegionStatus {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try NativeCompletion.start(
        { mln_runtime_offline_region_get_status(runtime.raw, id, $0) }
      ) { result in
        try OfflineRegionStatus(
          native: NativeOfflineRegionStatus(NativeCompletion.value(result))
        )
      }
    }
  }

  /// Selects whether this region reports progress through runtime events. It
  /// reports ``MaplibreErrorKind/notFound`` when no region carries `id`.
  func setOfflineRegionObserved(
    id: Int64,
    observed: Bool
  ) async throws {
    try await offlineUnit {
      mln_runtime_offline_region_set_observed($0, id, observed, $1)
    }
  }

  /// Starts or stops this region's download. It reports
  /// ``MaplibreErrorKind/notFound`` when no region carries `id`.
  func setOfflineRegionDownloadState(
    id: Int64,
    state: OfflineRegionDownloadState
  ) async throws {
    try await offlineUnit {
      mln_runtime_offline_region_set_download_state($0, id, state.rawValue, $1)
    }
  }

  /// Marks this region's resources stale so the next download refetches them.
  /// It reports ``MaplibreErrorKind/notFound`` when no region carries `id`.
  func invalidateOfflineRegion(id: Int64) async throws {
    try await offlineUnit {
      mln_runtime_offline_region_invalidate($0, id, $1)
    }
  }

  /// Deletes this region and the resources only it required. It reports
  /// ``MaplibreErrorKind/notFound`` when no region carries `id`.
  func deleteOfflineRegion(id: Int64) async throws {
    try await offlineUnit {
      mln_runtime_offline_region_delete($0, id, $1)
    }
  }

  private func offlineUnit(
    _ call: (mln_runtime, UnsafePointer<mln_completion>) -> mln_status
  ) async throws {
    try await awaitNative {
      let runtime = try requireLiveHandle()
      return try NativeCompletion.startUnit { call(runtime.raw, $0) }
    }
  }

  private static func copyOfflineRegion(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> OfflineRegionInfo {
    let raw: mln_offline_region_info = try NativeCompletion.value(result)
    return try OfflineRegionInfo(native: NativeOfflineRegionInfo(copying: raw))
  }

  private static func copyOptionalOfflineRegion(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> OfflineRegionInfo? {
    guard result.pointee.value_count > 0 else { return nil }
    return try copyOfflineRegion(result)
  }

  private static func copyOfflineRegions(
    _ result: UnsafePointer<mln_completion_result>
  ) throws -> [OfflineRegionInfo] {
    try NativeCompletion.values(result, as: mln_offline_region_info.self).map {
      try OfflineRegionInfo(native: NativeOfflineRegionInfo(copying: $0))
    }
  }
}
