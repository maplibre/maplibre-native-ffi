package org.maplibre.nativeffi.browser

import kotlin.test.Test
import kotlin.test.fail
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.RuntimeOptions
import org.maplibre.nativeffi.runSuspendTest

/**
 * Fails on purpose after a native round trip. Only `mise run //bindings/kotlin:test:wasm-canary`
 * runs it, and passes when it fails the Karma run, so a runner that passes a test whose failure
 * follows asynchronous work fails the canary instead.
 */
class HarnessCanaryTest {
  @Test
  fun aFailureAfterANativeRoundTripFailsTheRun() = runSuspendTest {
    val runtime = RuntimeHandle.create(RuntimeOptions())
    runtime.release().awaitWithin("the runtime release")
    fail("HarnessCanaryTest fails on purpose")
  }
}
