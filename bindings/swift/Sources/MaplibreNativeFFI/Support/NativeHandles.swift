/// A handle the C API issued. The C API spells every handle as one integer
/// type, so each kind gets its own wrapper here to stay distinct at compile
/// time. `raw` names one object for the life of the process, carries no
/// ownership, and is safe to copy. Zero is the null handle.
protocol NativeHandle: Hashable, Sendable {
  var raw: UInt64 { get }
  init(raw: UInt64)
}

extension NativeHandle {
  var isNull: Bool {
    raw == 0
  }
}

struct NativeRuntimeHandle: NativeHandle {
  let raw: UInt64
}

struct NativeMapHandle: NativeHandle {
  let raw: UInt64
}

struct NativeMapProjectionHandle: NativeHandle {
  let raw: UInt64
}

struct NativeRenderSessionHandle: NativeHandle {
  let raw: UInt64
}

struct NativeOfflineRegionSnapshotHandle: NativeHandle {
  let raw: UInt64
}

struct NativeOfflineRegionListHandle: NativeHandle {
  let raw: UInt64
}

struct NativeBufferHandle: NativeHandle {
  let raw: UInt64
}

struct NativeStyleIdListHandle: NativeHandle {
  let raw: UInt64
}

struct NativeStyleStringListHandle: NativeHandle {
  let raw: UInt64
}

struct NativeQueriedFeatureListHandle: NativeHandle {
  let raw: UInt64
}

struct NativeGeoJSONSourceDataHandle: NativeHandle {
  let raw: UInt64
}

struct NativeWakeSourceHandle: NativeHandle {
  let raw: UInt64
}

struct NativeResourceRequestHandle: NativeHandle {
  let raw: UInt64
}
