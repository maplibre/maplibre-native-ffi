import Dispatch
import Foundation
@testable import MaplibreNative
import Testing

private let parkTimeout: TimeInterval = 10

/// Leaves the runtime idle with no latched signal, so a following park can only
/// be released by the signal the test raises.
private func drainLatchedWakes(_ runtime: RuntimeHandle) throws {
  for _ in 0 ..< 100 {
    if try !runtime.wait(timeout: 0) {
      return
    }
    try runtime.runOnce()
    while try runtime.pollEvent() != nil {}
  }
  Issue.record("the runtime kept latching wakes while idle")
}

@Test func parkedOwnerThreadWakesForNativeWorkAndForAWakeSource() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  let map = try MapHandle(
    runtime: runtime,
    options: MapOptions(width: 512, height: 512)
  )
  try drainLatchedWakes(runtime)

  // The style is malformed, so native reports the failure from its own threads.
  // What matters here is that the failure reaches a parked owner thread at all.
  try map.setStyleURL("unsupported://style.json")
  var loadingFailed = false
  for _ in 0 ..< 20 where !loadingFailed {
    #expect(try runtime.wait(timeout: parkTimeout))
    try runtime.runOnce()
    while let event = try runtime.pollEvent() {
      if event.type == .mapLoadingFailed {
        loadingFailed = true
      }
    }
  }
  #expect(loadingFailed)

  // A source signalled from another thread is what a host's submission path
  // holds, and the park it releases has no other work to end it.
  let source = try runtime.wakeSource()
  try drainLatchedWakes(runtime)
  let signalled = DispatchSemaphore(value: 0)
  DispatchQueue.global().async {
    Thread.sleep(forTimeInterval: 0.02)
    try? source.signal()
    signalled.signal()
  }
  #expect(try runtime.wait(timeout: parkTimeout))
  signalled.wait()

  // A wake source stays usable once its runtime is gone, so host teardown
  // ordering is free.
  try map.close()
  try runtime.close()
  try source.signal()
  try source.close()
  #expect(source.isClosed)
}

@Test func waitConsumesOneLatchedSignalAtATime() throws {
  let runtime =
    try RuntimeHandle(options: RuntimeOptions(cachePath: ":memory:"))
  let source = try runtime.wakeSource()
  try drainLatchedWakes(runtime)

  try source.signal()
  #expect(try runtime.wait(timeout: 0))
  // The latch is consumed, so an idle runtime reports the timeout instead.
  #expect(try !runtime.wait(timeout: 0))

  try source.close()
  try runtime.close()
}
