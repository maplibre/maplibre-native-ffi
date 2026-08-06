package org.maplibre.nativeffi

import kotlin.wasm.WasmExport
import org.maplibre.nativeffi.internal.wasm.BrowserModule

// The adapter kotlin-test runs the suite through, installed on the namespace kotlin-test reads it
// from. It is written in JavaScript because that is what the namespace holds: kotlin-test wraps the
// object below and hands its two methods the suite and test bodies as ordinary JS functions.
//
// Reporting each test as a line is the whole of what a failing run says. There is no test framework
// on this thread to report to, and `console.log` in this worker's realm is what the runner
// collects.
@JsFun(
  """
  () => {
    const state = { failures: 0, path: [] }
    globalThis.__mlnTestState = state
    globalThis.kotlinTest = {
      adapter: {
        suite: (name, ignored, suiteFn) => {
          if (ignored) return
          state.path.push(name)
          try { suiteFn() } finally { state.path.pop() }
        },
        test: (name, ignored, testFn) => {
          const qualified = state.path.concat(name).join(".")
          if (ignored) { console.log("IGNORED " + qualified); return }
          try {
            testFn()
            console.log("PASSED " + qualified)
          } catch (error) {
            state.failures++
            console.log("FAILED " + qualified + ": " + (error && error.stack || error))
          }
        },
      },
    }
  }
"""
)
private external fun installTestAdapter()

@JsFun("() => globalThis.__mlnTestState.failures") private external fun testFailures(): Int

/**
 * Prepares the suite to run inside the module, on the thread the module imported it into.
 *
 * `bindings/kotlin/browser-test/maplibre-native-kotlin.mjs` stands in for the application a host
 * would serve: it calls this, then the compiler-emitted `startUnitTests`, then
 * [mlnKotlinTestFailures]. A test binary cannot be its own entry point the way an application is,
 * because the compiler emits the suite as an export only JavaScript can call.
 *
 * Naming the module first is what makes every later call work: each generated entry point reads
 * `globalThis.__maplibreNativeC`, and nothing else on this thread sets it.
 */
@WasmExport("mlnKotlinTestBegin")
internal fun mlnKotlinTestBegin() {
  BrowserModule.attach()
  installTestAdapter()
}

/** The number of tests that failed, which is what the run's exit status is built from. */
@WasmExport("mlnKotlinTestFailures") internal fun mlnKotlinTestFailures(): Int = testFailures()
