import Dispatch
import Foundation
@testable import MaplibreNative
import Testing

private let parkTimeout: TimeInterval = 10

/// Well below parkTimeout, and far above the scheduling noise a loaded CI
/// machine
/// adds to a condition-variable wake.
private let promptReturn: TimeInterval = 5

/// Pumps until the runtime is idle, so a park that follows is released by the
/// signal the test raises.
private func quiesce(_ runtime: RuntimeHandle) throws {
  for _ in 0 ..< 100 {
    try runtime.pump()
    var drained = false
    while try runtime.pollEvent() != nil {
      drained = true
    }
    if !drained {
      return
    }
  }
  Issue.record("the runtime kept producing events while idle")
}

@Test func parkedOwnerThreadWakesForNativeWorkAndForAWakeSource() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 512, height: 512)
  )
  try quiesce(runtime)

  // The style is malformed, so native reports the failure from its own threads
  // and
  // the failure reaches the parked owner thread.
  try map.setStyleURL("unsupported://style.json")
  var loadingFailed = false
  let loadStarted = Date()
  for _ in 0 ..< 20 where !loadingFailed {
    try runtime.pump(timeout: parkTimeout)
    #expect(
      Date().timeIntervalSince(loadStarted) < promptReturn,
      "parks sat out their timeouts while loading was pending"
    )
    while let event = try runtime.pollEvent() {
      if event.type == .mapLoadingFailed {
        loadingFailed = true
      }
    }
  }
  #expect(loadingFailed)

  // A source signalled from another thread matches a host's submission path,
  // and
  // the park it releases has no other work to end it.
  let source = try runtime.wakeSource()
  try quiesce(runtime)
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

@Test func pumpConsumesOneLatchedSignalAtATime() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  let source = try runtime.wakeSource()
  try quiesce(runtime)

  try source.signal()
  let signalledStarted = Date()
  try runtime.pump(timeout: parkTimeout)
  #expect(
    Date().timeIntervalSince(signalledStarted) < promptReturn,
    "a pump blocked despite a latched signal"
  )

  // With the latch spent, an idle runtime sits out its timeout.
  let idleStarted = Date()
  try runtime.pump(timeout: 0.2)
  #expect(
    Date().timeIntervalSince(idleStarted) >= 0.1,
    "a second pump consumed a latch the first should have spent"
  )

  try source.close()
  try runtime.close()
}
