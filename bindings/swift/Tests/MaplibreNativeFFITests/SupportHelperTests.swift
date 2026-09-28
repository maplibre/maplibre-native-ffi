import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

@Test func nativeArenaPreservesCountedUtf8AndRejectsCStringNul() throws {
  let arena = NativeInputArena()
  #expect(throws: NativeStringError.self) { try arena.cString("a\0b") }
  let view = arena.view("é\0")
  #expect(try NativeString.copyUTF8(data: view.data, size: view.size) == "é\0")
}

@Test func nativeStringCopyUTF8RejectsInvalidBytes() throws {
  var invalid = [UInt8](arrayLiteral: 0xFF)

  do {
    _ = try invalid.withUnsafeMutableBufferPointer { buffer in
      try NativeString.copyUTF8(
        data: UnsafeRawPointer(buffer.baseAddress!)
          .assumingMemoryBound(to: CChar.self),
        size: buffer.count
      )
    }
    Issue.record("invalid UTF-8 should throw")
  } catch let error as NativeStringError {
    #expect(error.message.contains("invalid bytes"))
  } catch {
    Issue.record("unexpected error: \(error)")
  }
}

@Test func nativeHandleStateCloseIsIdempotentAfterSuccess() throws {
  let closes = LockedBox(0)
  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x2)
  )

  try state.closeOnce { _ in
    closes.update { $0 += 1 }
  }
  try state.closeOnce { _ in
    closes.update { $0 += 1 }
  }

  #expect(state.isClosed)
  #expect(closes.read { $0 } == 1)
}

@Test func nativeHandleReadReservesOwnerUntilCopyCompletes() throws {
  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x2)
  )
  let access = try state.borrow()
  #expect(throws: NativeStatusFailure.self) {
    try state.closeOnce { _ in Issue.record("reserved owner was released") }
  }
  #expect(!state.isClosed)
  access.end()
  try state.closeOnce { _ in }
  #expect(state.isClosed)
}

@Test func nativeHandleStateConcurrentCloseDestroysOnce() throws {
  let closes = LockedBox(0)
  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x5)
  )

  DispatchQueue.concurrentPerform(iterations: 16) { _ in
    try? state.closeOnce { _ in
      closes.update { $0 += 1 }
    }
  }

  #expect(state.isClosed)
  #expect(closes.read { $0 } == 1)
}

@Test func nativeHandleStateAllowsRetryAfterFailedClose() throws {
  struct CloseFailure: Error {}

  let closes = LockedBox(0)
  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x4)
  )

  do {
    try state.closeOnce { _ in
      closes.update { $0 += 1 }
      throw CloseFailure()
    }
    Issue.record("failed close should throw")
  } catch is CloseFailure {}

  #expect(!state.isClosed)

  try state.closeOnce { _ in
    closes.update { $0 += 1 }
  }

  #expect(state.isClosed)
  #expect(closes.read { $0 } == 2)
}

@Test func nativeHandleStateRejectsUseWhileCloseIsInFlightAndRetriesAfterFailure(
) throws {
  struct CloseFailure: Error {}

  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x6)
  )
  let closeStarted = DispatchSemaphore(value: 0)
  let allowCloseToFail = DispatchSemaphore(value: 0)
  let closeFinished = DispatchSemaphore(value: 0)

  Thread {
    do {
      try state.closeOnce { _ in
        closeStarted.signal()
        _ = allowCloseToFail.wait(timeout: .now() + .seconds(5))
        throw CloseFailure()
      }
      Issue.record("failed close should throw")
    } catch is CloseFailure {
    } catch {
      Issue.record("unexpected error: \(error)")
    }
    closeFinished.signal()
  }.start()

  #expect(closeStarted.wait(timeout: .now() + .seconds(5)) == .success)
  #expect(!state.isClosed)
  do {
    _ = try state.requireLive()
    Issue.record("closing handle should throw")
  } catch let failure as NativeStatusFailure {
    #expect(failure.diagnostic == "test_handle is closing")
  } catch {
    Issue.record("unexpected error: \(error)")
  }

  allowCloseToFail.signal()
  _ = closeFinished.wait(timeout: .now() + .seconds(5))
  #expect(!state.isClosed)

  try state.closeOnce { _ in }
  #expect(state.isClosed)
}

@Test func nativeHandleStateReportsLeaksWithoutDestroying() throws {
  let leaks = LockedBox([NativeHandleLeak]())
  struct UnretiredHandle: NativeHandle {
    let raw: UInt64; func disposeAbandoned() -> Bool {
      false
    }
  }
  let leaked = UnretiredHandle(raw: 3)

  try NativeHandleLeakTestSupport.withHandler({ leak in
    leaks.update { $0.append(leak) }
  }) {
    do {
      _ = try NativeHandleState(
        typeName: "leaky_handle",
        handle: leaked
      )
    }

    #expect(leaks.read { $0 } == [NativeHandleLeak(
      typeName: "leaky_handle",
      handle: leaked.raw
    )])
  }
}

@Test func repeatedInlineCloseClaimsDecisionAndReleasesOnce() throws {
  let state = try NativeHandleState(
    typeName: "request",
    handle: SyntheticHandles.resourceRequest(99),
    pendingDecision: true
  )
  let releases = LockedBox(0)
  try state.closeOnce { _ in releases.update { $0 += 1 } }
  try state.closeOnce { _ in releases.update { $0 += 1 } }
  #expect(state.finishDecision(accepted: false))
  #expect(state.isClosed)
  #expect(releases.value == 1)
}
