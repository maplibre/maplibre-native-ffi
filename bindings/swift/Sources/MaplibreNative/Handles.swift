
class NativeHandleBox {
  private let state: NativeHandleState

  init(typeName: String, pointer: OpaquePointer?) throws {
    do {
      state = try NativeHandleState(typeName: typeName, pointer: pointer)
    } catch let failure as NativeStatusFailure {
      throw MaplibreError.invalidArgument(failure.diagnostic)
    }
  }

  var isClosed: Bool {
    state.isClosed
  }

  func requireLive() throws -> OpaquePointer {
    do {
      return try state.requireLive()
    } catch let failure as NativeStatusFailure {
      throw MaplibreError(
        kind: .invalidState,
        rawStatus: nil,
        diagnostic: failure.diagnostic
      )
    }
  }

  func closeOnce(_ destroy: (OpaquePointer) throws -> Void) throws {
    do {
      try state.closeOnce(destroy)
    } catch let failure as NativeStatusFailure {
      if failure.rawStatus == 0 {
        throw MaplibreError(
          kind: .invalidState,
          rawStatus: nil,
          diagnostic: failure.diagnostic
        )
      }
      throw MaplibreError.fromNativeFailure(failure)
    }
  }
}
