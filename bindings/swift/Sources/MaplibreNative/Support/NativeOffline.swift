internal import CMaplibreNativeC
import Foundation

enum NativeOffline {
  static func runAmbientCacheOperationStart(
    _ runtime: NativeRuntimeHandle,
    operation: UInt32
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_run_ambient_cache_operation_start(
        runtime.raw,
        operation,
        operationId
      ))
    }.value
  }

  static func regionCreateStart(
    _ runtime: NativeRuntimeHandle,
    definition: UnsafePointer<mln_offline_region_definition>,
    metadata: Data
  ) throws -> UInt64 {
    try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_region_create_start(
          runtime.raw,
          definition,
          bytes.bindMemory(to: UInt8.self).baseAddress,
          bytes.count,
          operationId
        ))
      }.value
    }
  }

  static func regionGetStart(_ runtime: NativeRuntimeHandle,
                             regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_get_start(
        runtime.raw,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionsListStart(_ runtime: NativeRuntimeHandle) throws
    -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_regions_list_start(
        runtime.raw,
        operationId
      ))
    }.value
  }

  static func regionsMergeDatabaseStart(
    _ runtime: NativeRuntimeHandle,
    sideDatabasePath: String
  ) throws -> UInt64 {
    try NativeString.withCString(sideDatabasePath) { path in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_regions_merge_database_start(
          runtime.raw,
          path,
          operationId
        ))
      }.value
    }
  }

  static func regionUpdateMetadataStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64,
    metadata: Data
  ) throws -> UInt64 {
    try metadata.withUnsafeBytes { bytes in
      try NativeMemory.withTemporary(UInt64(0)) { operationId in
        try checkStatus(mln_runtime_offline_region_update_metadata_start(
          runtime.raw,
          regionId,
          bytes.bindMemory(to: UInt8.self).baseAddress,
          bytes.count,
          operationId
        ))
      }.value
    }
  }

  static func regionGetStatusStart(_ runtime: NativeRuntimeHandle,
                                   regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_get_status_start(
        runtime.raw,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionSetObservedStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64,
    observed: Bool
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_set_observed_start(
        runtime.raw,
        regionId,
        observed,
        operationId
      ))
    }.value
  }

  static func regionSetDownloadStateStart(
    _ runtime: NativeRuntimeHandle,
    regionId: Int64,
    state: UInt32
  ) throws -> UInt64 {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_set_download_state_start(
        runtime.raw,
        regionId,
        state,
        operationId
      ))
    }.value
  }

  static func regionInvalidateStart(_ runtime: NativeRuntimeHandle,
                                    regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_invalidate_start(
        runtime.raw,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionDeleteStart(_ runtime: NativeRuntimeHandle,
                                regionId: Int64) throws -> UInt64
  {
    try NativeMemory.withTemporary(UInt64(0)) { operationId in
      try checkStatus(mln_runtime_offline_region_delete_start(
        runtime.raw,
        regionId,
        operationId
      ))
    }.value
  }

  static func regionCreateTakeResult(
    _ runtime: NativeRuntimeHandle,
    operationId: UInt64
  ) throws -> NativeOfflineRegionInfo {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_runtime_offline_region_create_take_result(
          runtime.raw,
          operationId,
          outHandle
        ))
      }.value
    let snapshot = NativeOfflineRegionSnapshotHandle(raw: snapshotValue)
    guard !snapshot.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline region create result was null"
    ) }
    defer { mln_offline_region_snapshot_destroy(snapshot.raw) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionGetTakeResult(
    _ runtime: NativeRuntimeHandle,
    operationId: UInt64
  ) throws -> NativeOfflineRegionInfo? {
    var found = false
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try NativeMemory.withTemporary(false) { outFound in
          try checkStatus(mln_runtime_offline_region_get_take_result(
            runtime.raw,
            operationId,
            outHandle,
            outFound
          ))
          found = outFound.pointee
        }
      }.value
    let snapshot = NativeOfflineRegionSnapshotHandle(raw: snapshotValue)
    guard found, !snapshot.isNull else { return nil }
    defer { mln_offline_region_snapshot_destroy(snapshot.raw) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionsListTakeResult(
    _ runtime: NativeRuntimeHandle,
    operationId: UInt64
  ) throws -> [NativeOfflineRegionInfo] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try checkStatus(mln_runtime_offline_regions_list_take_result(
        runtime.raw,
        operationId,
        outHandle
      ))
    }.value
    let list = NativeOfflineRegionListHandle(raw: listValue)
    guard !list.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline region list result was null"
    ) }
    defer { mln_offline_region_list_destroy(list.raw) }
    return try offlineRegionListCopy(list)
  }

  static func regionsMergeDatabaseTakeResult(
    _ runtime: NativeRuntimeHandle,
    operationId: UInt64
  ) throws -> [NativeOfflineRegionInfo] {
    let listValue = try NativeMemory.withTemporary(UInt64(0)) { outHandle in
      try checkStatus(mln_runtime_offline_regions_merge_database_take_result(
        runtime.raw,
        operationId,
        outHandle
      ))
    }.value
    let list = NativeOfflineRegionListHandle(raw: listValue)
    guard !list.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline merge result list was null"
    ) }
    defer { mln_offline_region_list_destroy(list.raw) }
    return try offlineRegionListCopy(list)
  }

  static func regionUpdateMetadataTakeResult(
    _ runtime: NativeRuntimeHandle,
    operationId: UInt64
  ) throws -> NativeOfflineRegionInfo {
    let snapshotValue = try NativeMemory
      .withTemporary(UInt64(0)) { outHandle in
        try checkStatus(mln_runtime_offline_region_update_metadata_take_result(
          runtime.raw,
          operationId,
          outHandle
        ))
      }.value
    let snapshot = NativeOfflineRegionSnapshotHandle(raw: snapshotValue)
    guard !snapshot.isNull else { throw NativeStatusFailure(
      rawStatus: 0,
      diagnostic: "offline update metadata result was null"
    ) }
    defer { mln_offline_region_snapshot_destroy(snapshot.raw) }
    return try offlineRegionSnapshotCopy(snapshot)
  }

  static func regionGetStatusTakeResult(
    _ runtime: NativeRuntimeHandle,
    operationId: UInt64
  ) throws -> NativeOfflineRegionStatus {
    var status = mln_offline_region_status()
    status.size = UInt32(MemoryLayout<mln_offline_region_status>.size)
    try checkStatus(mln_runtime_offline_region_get_status_take_result(
      runtime.raw,
      operationId,
      &status
    ))
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
