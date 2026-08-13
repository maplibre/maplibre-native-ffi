internal import CMaplibreNativeC
import Foundation

enum NativeOffline {
  static func runAmbientCacheOperationStart(
    _ runtime: NativeRuntimeHandle,
    operation: UInt32
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { outOperation in
      try checkStatus(mln_runtime_run_ambient_cache_operation_start(
        runtime.raw,
        operation,
        outOperation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func setMaximumAmbientCacheSizeStart(
    _ runtime: NativeRuntimeHandle,
    size: UInt64
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_set_maximum_ambient_cache_size_start(
        runtime.raw,
        size,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionCreateStart(
    _ runtime: NativeRuntimeHandle,
    definition: UnsafePointer<mln_offline_region_definition>,
    metadata: Data
  ) throws -> NativeOperationHandle {
    let raw = try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operation in
        try checkStatus(mln_runtime_offline_region_create_start(
          runtime.raw,
          definition,
          bytes.bindMemory(to: UInt8.self).baseAddress,
          bytes.count,
          operation
        ))
      }.value
    }
    return NativeOperationHandle(raw: raw)
  }

  static func regionGetStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_region_get_start(
        runtime.raw,
        regionId,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionsListStart(
    _ runtime: NativeRuntimeHandle
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_regions_list_start(
        runtime.raw,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionsMergeDatabaseStart(
    _ runtime: NativeRuntimeHandle,
    sideDatabasePath: String
  ) throws -> NativeOperationHandle {
    let raw = try NativeString.withCString(sideDatabasePath) { path in
      try NativeMemory.withTemporary(UInt64(0)) { operation in
        try checkStatus(mln_runtime_offline_regions_merge_database_start(
          runtime.raw,
          path,
          operation
        ))
      }.value
    }
    return NativeOperationHandle(raw: raw)
  }

  static func regionUpdateMetadataStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64,
    metadata: Data
  ) throws -> NativeOperationHandle {
    let raw = try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operation in
        try checkStatus(mln_runtime_offline_region_update_metadata_start(
          runtime.raw,
          regionId,
          bytes.bindMemory(to: UInt8.self).baseAddress,
          bytes.count,
          operation
        ))
      }.value
    }
    return NativeOperationHandle(raw: raw)
  }

  static func regionGetStatusStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_region_get_status_start(
        runtime.raw,
        regionId,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionSetObservedStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64,
    observed: Bool
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_region_set_observed_start(
        runtime.raw,
        regionId,
        observed,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionSetDownloadStateStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64,
    state: UInt32
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_region_set_download_state_start(
        runtime.raw,
        regionId,
        state,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionInvalidateStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_region_invalidate_start(
        runtime.raw,
        regionId,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionDeleteStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64
  ) throws -> NativeOperationHandle {
    let raw = try NativeMemory.withTemporary(UInt64(0)) { operation in
      try checkStatus(mln_runtime_offline_region_delete_start(
        runtime.raw,
        regionId,
        operation
      ))
    }.value
    return NativeOperationHandle(raw: raw)
  }

  static func regionCreateTakeResult(
    operation: NativeOperationHandle,
    didTransfer: () -> Void
  ) throws -> NativeOfflineRegionInfo {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_runtime_offline_region_create_take_result(
          operation.raw,
          outHandle
        ))
      }.value
    didTransfer()
    let snapshot = NativeOfflineRegionSnapshotHandle(raw: snapshotValue)
    guard !snapshot.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline region create result was null"
    ) }
    defer { mln_offline_region_snapshot_destroy(snapshot.raw) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionGetTakeResult(
    operation: NativeOperationHandle,
    didTransfer: () -> Void
  ) throws -> NativeOfflineRegionInfo? {
    var found = false
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_runtime_offline_region_get_take_result(
            operation.raw,
            outHandle,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    didTransfer()
    let snapshot = NativeOfflineRegionSnapshotHandle(raw: snapshotValue)
    guard found, !snapshot.isNull else { return nil }
    defer { mln_offline_region_snapshot_destroy(snapshot.raw) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionsListTakeResult(
    operation: NativeOperationHandle,
    didTransfer: () -> Void
  ) throws -> [NativeOfflineRegionInfo] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try checkStatus(mln_runtime_offline_regions_list_take_result(
        operation.raw,
        outHandle
      ))
    }.value
    didTransfer()
    let list = NativeOfflineRegionListHandle(raw: listValue)
    guard !list.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline region list result was null"
    ) }
    defer { mln_offline_region_list_destroy(list.raw) }
    return try offlineRegionListCopy(list)
  }

  static func regionsMergeDatabaseTakeResult(
    operation: NativeOperationHandle,
    didTransfer: () -> Void
  ) throws -> [NativeOfflineRegionInfo] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try checkStatus(mln_runtime_offline_regions_merge_database_take_result(
        operation.raw,
        outHandle
      ))
    }.value
    didTransfer()
    let list = NativeOfflineRegionListHandle(raw: listValue)
    guard !list.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline merge result list was null"
    ) }
    defer { mln_offline_region_list_destroy(list.raw) }
    return try offlineRegionListCopy(list)
  }

  static func regionUpdateMetadataTakeResult(
    operation: NativeOperationHandle,
    didTransfer: () -> Void
  ) throws -> NativeOfflineRegionInfo {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_runtime_offline_region_update_metadata_take_result(
          operation.raw,
          outHandle
        ))
      }.value
    didTransfer()
    let snapshot = NativeOfflineRegionSnapshotHandle(raw: snapshotValue)
    guard !snapshot.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline update metadata result was null"
    ) }
    defer { mln_offline_region_snapshot_destroy(snapshot.raw) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionGetStatusTakeResult(
    operation: NativeOperationHandle,
    didTransfer: () -> Void
  ) throws -> NativeOfflineRegionStatus {
    var status = mln_offline_region_status()
    status.size = UInt32(MemoryLayout<mln_offline_region_status>.size)
    try checkStatus(mln_runtime_offline_region_get_status_take_result(
      operation.raw,
      &status
    ))
    didTransfer()
    return NativeOfflineRegionStatus(status)
  }

  private static func offlineRegionSnapshotCopy(
    _ snapshot: NativeOfflineRegionSnapshotHandle
  ) throws
    -> NativeOfflineRegionInfo
  {
    var info = mln_offline_region_info()
    info.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
    try checkStatus(mln_offline_region_snapshot_get(snapshot.raw, &info))
    return try NativeOfflineRegionInfo(copying: info)
  }

  private static func offlineRegionListCopy(
    _ list: NativeOfflineRegionListHandle
  ) throws
    -> [NativeOfflineRegionInfo]
  {
    let count = try NativeMemory.withTemporary(0) { count in
      try checkStatus(mln_offline_region_list_count(list.raw, count))
    }.value
    return try (0 ..< count).map { index in
      var info = mln_offline_region_info()
      info.size = UInt32(MemoryLayout<mln_offline_region_info>.size)
      try checkStatus(mln_offline_region_list_get(list.raw, index, &info))
      return try NativeOfflineRegionInfo(copying: info)
    }
  }
}
