import Dispatch
import Foundation
@testable import MaplibreNativeFFI
import Testing

private let parkTimeout: TimeInterval = 10

/// Well below parkTimeout, and far above the scheduling noise a loaded CI
/// machine adds to a condition-variable wake.
private let promptReturn: TimeInterval = 5

@Test func parkedOwnerThreadWakesForNativeWorkAndForAWakeSource() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 512, height: 512)
  )
  try pumpUntilQuiet(runtime)

  // The style is malformed, so native reports the failure from its own threads
  // and it reaches the parked owner thread.
  try map.setStyleURL("unsupported://style.json")
  var loadingFailed = false
  let loadStarted = Date()
  for _ in 0 ..< 20 where !loadingFailed {
    try runtime.pump(timeout: parkTimeout)
    #expect(
      Date().timeIntervalSince(loadStarted) < promptReturn,
      "parks sat out their timeouts while loading was pending"
    )
    for event in try runtime.drainEvents().events
      where event.type == .mapLoadingFailed
    {
      loadingFailed = true
    }
  }
  #expect(loadingFailed)

  // The park a cross-thread signal releases has no other work to end it.
  let source = try runtime.wakeSource()
  try pumpUntilQuiet(runtime)
  let signalled = DispatchSemaphore(value: 0)
  DispatchQueue.global().async {
    Thread.sleep(forTimeInterval: 0.02)
    try? source.signal()
    signalled.signal()
  }
  let parkStarted = Date()
  try runtime.pump(timeout: parkTimeout)
  #expect(
    Date().timeIntervalSince(parkStarted) < promptReturn,
    "the parked owner thread timed out instead of taking the signal"
  )
  signalled.wait()

  // A wake source stays usable after its runtime closes, so hosts tear the two
  // down in either order.
  try map.close()
  try runtime.close()
  try source.signal()
  try source.close()
  #expect(source.isClosed)
}

@Test func pumpClearsTheWakeFlagItReturnsOn() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  let source = try runtime.wakeSource()
  try pumpUntilQuiet(runtime)

  try source.signal()
  let signalledStarted = Date()
  try runtime.pump(timeout: parkTimeout)
  #expect(
    Date().timeIntervalSince(signalledStarted) < promptReturn,
    "a pump waited even though the wake flag was set"
  )

  // The pump above cleared the wake flag, so this one waits its full timeout.
  let idleStarted = Date()
  try runtime.pump(timeout: 0.2)
  #expect(
    Date().timeIntervalSince(idleStarted) >= 0.1,
    "the first pump left the wake flag set"
  )

  try source.close()
  try runtime.close()
}

/// BND-197. The wake source is the one handle a host may use and release from
/// different threads, so `NativeHandleState` has to order the two.
@Test func closeWaitsForAUseInFlightOnAnotherThread() throws {
  let state = try NativeHandleState(
    typeName: "TestHandle",
    handle: SyntheticHandles.wakeSource(1)
  )
  let entered = DispatchSemaphore(value: 0)
  let releaseUse = DispatchSemaphore(value: 0)
  let closeReturned = DispatchSemaphore(value: 0)
  let destroys = DestroyCounter()

  let useThread = Thread {
    try? state.withLive { _ in
      entered.signal()
      releaseUse.wait()
      #expect(
        destroys.value() == 0,
        "the use should never observe a destroyed handle"
      )
    }
  }
  useThread.start()
  #expect(entered.wait(timeout: .now() + 5) == .success)

  let closeThread = Thread {
    try? state.closeOnce { _ in destroys.increment() }
    closeReturned.signal()
  }
  closeThread.start()

  #expect(
    closeReturned.wait(timeout: .now() + 0.2) == .timedOut,
    "close should wait for the use"
  )
  #expect(destroys.value() == 0)

  releaseUse.signal()
  #expect(closeReturned.wait(timeout: .now() + 5) == .success)
  #expect(destroys.value() == 1)
  #expect(state.isClosed)
}

/// BND-197. A use that starts once close has begun is turned away by the
/// wrapper rather than reaching native with a retired id.
@Test func aUseStartingAfterCloseBeginsIsRefused() throws {
  let state = try NativeHandleState(
    typeName: "TestHandle",
    handle: SyntheticHandles.wakeSource(2)
  )
  try state.closeOnce { _ in }

  #expect(throws: NativeStatusFailure.self) {
    try state.withLive { _ in }
  }
}

private final class DestroyCounter: @unchecked Sendable {
  private let lock = NSLock()
  private var count = 0

  func increment() {
    lock.withLock { count += 1 }
  }

  func value() -> Int {
    lock.withLock { count }
  }
}
