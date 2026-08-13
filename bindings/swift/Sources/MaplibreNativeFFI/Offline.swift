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
  func runAmbientCacheOperationStart(_ operation: AmbientCacheOperation) throws
    -> OperationHandle
  {
    try makeOperation(
      NativeOffline.runAmbientCacheOperationStart(
        requireLiveHandle(),
        operation: operation.rawValue
      ),
      resultKind: .none
    )
  }

  /// Starts a change to this runtime's maximum ambient cache size. MapLibre
  /// evicts ambient resources to fit the new budget, so lowering it discards
  /// cached resources. Offline regions are unaffected.
  func setMaximumAmbientCacheSizeStart(_ size: UInt64) throws
    -> OperationHandle
  {
    try makeOperation(
      NativeOffline.setMaximumAmbientCacheSizeStart(
        requireLiveHandle(),
        size: size
      ),
      resultKind: .none
    )
  }

  func offlineRegionCreateStart(
    definition: OfflineRegionDefinition,
    metadata: Data = Data()
  ) throws -> OperationHandle {
    try definition.nativeDefinition.withNativeDefinition { definition in
      try makeOperation(
        NativeOffline.regionCreateStart(
          requireLiveHandle(),
          definition: definition,
          metadata: metadata
        ),
        resultKind: .createdRegion
      )
    }
  }

  func offlineRegionGetStart(regionId: Int64) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionGetStart(requireLiveHandle(), regionId: regionId),
      resultKind: .optionalRegion
    )
  }

  func offlineRegionsListStart() throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionsListStart(requireLiveHandle()),
      resultKind: .regionList
    )
  }

  func offlineRegionsMergeDatabaseStart(sideDatabasePath: String) throws
    -> OperationHandle
  {
    try makeOperation(
      NativeOffline.regionsMergeDatabaseStart(
        requireLiveHandle(),
        sideDatabasePath: sideDatabasePath
      ),
      resultKind: .mergedRegionList
    )
  }

  func offlineRegionUpdateMetadataStart(
    regionId: Int64,
    metadata: Data
  ) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionUpdateMetadataStart(
        requireLiveHandle(),
        regionId: regionId,
        metadata: metadata
      ),
      resultKind: .updatedRegionMetadata
    )
  }

  func offlineRegionGetStatusStart(regionId: Int64) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionGetStatusStart(
        requireLiveHandle(),
        regionId: regionId
      ),
      resultKind: .regionStatus
    )
  }

  func offlineRegionSetObservedStart(
    regionId: Int64,
    observed: Bool
  ) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionSetObservedStart(
        requireLiveHandle(),
        regionId: regionId,
        observed: observed
      ),
      resultKind: .none
    )
  }

  func offlineRegionSetDownloadStateStart(
    regionId: Int64,
    state: OfflineRegionDownloadState
  ) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionSetDownloadStateStart(
        requireLiveHandle(),
        regionId: regionId,
        state: state.rawValue
      ),
      resultKind: .none
    )
  }

  func offlineRegionInvalidateStart(regionId: Int64) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionInvalidateStart(
        requireLiveHandle(),
        regionId: regionId
      ),
      resultKind: .none
    )
  }

  func offlineRegionDeleteStart(regionId: Int64) throws -> OperationHandle {
    try makeOperation(
      NativeOffline.regionDeleteStart(
        requireLiveHandle(),
        regionId: regionId
      ),
      resultKind: .none
    )
  }

  func offlineRegionCreateTakeResult(
    operation: OperationHandle
  ) throws -> OfflineRegionInfo {
    try operation.take(from: self, resultKind: .createdRegion) {
      operation, transferred in
      try OfflineRegionInfo(
        native: NativeOffline.regionCreateTakeResult(
          operation: operation,
          didTransfer: transferred
        )
      )
    }
  }

  func offlineRegionGetTakeResult(
    operation: OperationHandle
  ) throws -> OfflineRegionInfo? {
    try operation.take(from: self, resultKind: .optionalRegion) {
      operation, transferred in
      try NativeOffline.regionGetTakeResult(
        operation: operation,
        didTransfer: transferred
      ).map(OfflineRegionInfo.init(native:))
    }
  }

  func offlineRegionsListTakeResult(
    operation: OperationHandle
  ) throws -> [OfflineRegionInfo] {
    try operation.take(from: self, resultKind: .regionList) {
      operation, transferred in
      try NativeOffline.regionsListTakeResult(
        operation: operation,
        didTransfer: transferred
      ).map(OfflineRegionInfo.init(native:))
    }
  }

  func offlineRegionsMergeDatabaseTakeResult(
    operation: OperationHandle
  ) throws -> [OfflineRegionInfo] {
    try operation.take(from: self, resultKind: .mergedRegionList) {
      operation, transferred in
      try NativeOffline.regionsMergeDatabaseTakeResult(
        operation: operation,
        didTransfer: transferred
      ).map(OfflineRegionInfo.init(native:))
    }
  }

  func offlineRegionUpdateMetadataTakeResult(
    operation: OperationHandle
  ) throws -> OfflineRegionInfo {
    try operation.take(from: self, resultKind: .updatedRegionMetadata) {
      operation, transferred in
      try OfflineRegionInfo(
        native: NativeOffline.regionUpdateMetadataTakeResult(
          operation: operation,
          didTransfer: transferred
        )
      )
    }
  }

  func offlineRegionGetStatusTakeResult(
    operation: OperationHandle
  ) throws -> OfflineRegionStatus {
    try operation.take(from: self, resultKind: .regionStatus) {
      operation, transferred in
      try OfflineRegionStatus(
        native: NativeOffline.regionGetStatusTakeResult(
          operation: operation,
          didTransfer: transferred
        )
      )
    }
  }

  private func makeOperation(
    _ native: @autoclosure () throws -> NativeOperationHandle,
    resultKind: OperationResultKind
  ) throws -> OperationHandle {
    try mapNativeFailure {
      try OperationHandle(
        runtime: self,
        handle: native(),
        resultKind: resultKind
      )
    }
  }
}
