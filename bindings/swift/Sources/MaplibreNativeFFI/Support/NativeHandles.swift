/// A handle the C API issued.
///
/// The C API spells every handle as one integer type, so each kind gets its own
/// wrapper here to keep the kinds distinct at compile time. `raw` names one
/// object for the life of the process, carries no ownership, and is safe to
/// copy, compare, and hash. Zero is the null handle.
///
/// These are distinct from ``NativePointer``, which borrows a backend-native
/// address such as a Metal texture or a Vulkan device.
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

struct NativeJSONSnapshotHandle: NativeHandle {
  let raw: UInt64
}

struct NativeStyleIdListHandle: NativeHandle {
  let raw: UInt64
}

struct NativeStyleStringListHandle: NativeHandle {
  let raw: UInt64
}

struct NativeFeatureQueryResultHandle: NativeHandle {
  let raw: UInt64
}

struct NativeFeatureExtensionResultHandle: NativeHandle {
  let raw: UInt64
}

struct NativeWakeSourceHandle: NativeHandle {
  let raw: UInt64
}

struct NativeResourceRequestHandle: NativeHandle {
  let raw: UInt64
}
