import CMaplibreNativeC
import Foundation
@testable import MaplibreNativeFFI
import Testing

/// A second close succeeds without reaching native, and a closed handle
/// rejects use in the binding, before any native call.
@Test func closingAHandleTwiceDoesNothingTheSecondTime() async throws {
  try await withMapFixture { fixture in
    try await fixture.map.setStyleJson(json: emptyStyle)
    let data = try Maplibre.geojsonSourceDataCreate(
      data: Data(#"{"type":"FeatureCollection","features":[]}"#.utf8)
    )
    try data.close()
    try data.close()
    #expect(data.isClosed)

    let error = await expectMaplibreError(.invalidState) {
      try await fixture.map.addGeojsonSourceData(sourceId: "late", data: data)
    }
    #expect(error?.rawStatus == nil)
  }
}

/// Native refuses to release a runtime that still has a map, and the refusal
/// leaves the runtime usable, so the host closes the map and then the runtime.
@Test func aRefusedCloseLeavesTheRuntimeUsableAndALaterCloseSucceeds(
) async throws {
  let fixture = try await MapFixture.make()
  let refusal = await expectMaplibreError(.invalidState) {
    try await fixture.runtime.close()
  }
  #expect(refusal?.rawStatus == MLN_STATUS_INVALID_STATE.rawValue)
  #expect(!fixture.runtime.isClosed)
  try await fixture.runtime.barrier()

  try await fixture.map.close()
  try await fixture.runtime.close()
  #expect(fixture.runtime.isClosed)
}

/// A borrow that another thread holds, as a native call in flight does, holds
/// off a close, which fails as in use rather than releasing the handle under
/// the call. Once the borrow ends, the close succeeds.
@Test func aBorrowOnAnotherThreadHoldsOffAClose() async throws {
  let fixture = try await MapFixture.make()
  let borrowed = DispatchSemaphore(value: 0)
  let finish = DispatchSemaphore(value: 0)
  let ended = DispatchSemaphore(value: 0)
  let map = fixture.map
  Thread {
    do {
      let access = try map.handle.borrow()
      borrowed.signal()
      _ = isSignalled(finish)
      access.end()
    } catch {
      Issue.record("borrowing the map failed: \(error)")
      borrowed.signal()
    }
    ended.signal()
  }.start()
  #expect(isSignalled(borrowed))

  let refusal = await expectMaplibreError(.invalidState) {
    try await map.close()
  }
  #expect(refusal?.diagnostic == "MapHandle is in use")
  #expect(!map.isClosed)

  finish.signal()
  #expect(isSignalled(ended))
  try await map.close()
  #expect(map.isClosed)
  await fixture.close()
}

/// A map holds its runtime, so a host may drop its own runtime reference
/// while the map is live. The runtime goes once the map does.
@Test func aMapKeepsItsRuntimeAlive() async throws {
  var runtime: RuntimeHandle? = try MapFixture.makeRuntime()
  weak var weakRuntime = runtime
  var map: MapHandle? = try await runtime?.mapCreate(options: MapOptions(
    initialExtent: LogicalExtent(width: 8, height: 8, scaleFactor: 1)
  ))
  runtime = nil
  #expect(weakRuntime != nil)
  _ = try await map?.setStyleJson(json: emptyStyle)

  try await map?.close()
  map = nil
  #expect(weakRuntime == nil)
}

/// Closes on sixteen threads at once destroy the native handle once.
@Test func concurrentClosesDestroyOnce() throws {
  let closes = LockedBox(0)
  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x5)
  )
  DispatchQueue.concurrentPerform(iterations: 16) { _ in
    try? state.closeOnce { _ in closes.update { $0 += 1 } }
  }
  #expect(state.isClosed)
  #expect(closes.value == 1)
}

/// A close in flight on another thread rejects use as closing, and a close
/// that fails leaves the handle live for a retry.
@Test func aCloseInFlightRejectsUseAndAFailedCloseCanBeRetried() throws {
  struct CloseFailure: Error {}
  let state = try NativeHandleState(
    typeName: "test_handle",
    handle: SyntheticHandles.resourceRequest(0x6)
  )
  let closeStarted = DispatchSemaphore(value: 0)
  let failClose = DispatchSemaphore(value: 0)
  let closeFinished = DispatchSemaphore(value: 0)
  Thread {
    #expect(throws: CloseFailure.self) {
      try state.closeOnce { _ in
        closeStarted.signal()
        _ = isSignalled(failClose)
        throw CloseFailure()
      }
    }
    closeFinished.signal()
  }.start()

  #expect(isSignalled(closeStarted))
  #expect(throws: NativeStatusFailure(
    rawStatus: 0,
    diagnostic: "test_handle is closing"
  )) { try state.requireLive() }

  failClose.signal()
  #expect(isSignalled(closeFinished))
  #expect(!state.isClosed)
  try state.closeOnce { _ in }
  #expect(state.isClosed)
}

/// A request handle whose provider decision is pending defers a close to the
/// decision, which releases it once.
@Test func aCloseDuringAPendingDecisionReleasesOnce() throws {
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
