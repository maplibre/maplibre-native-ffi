package org.maplibre.nativeffi

import kotlin.coroutines.Continuation
import kotlin.coroutines.EmptyCoroutineContext
import kotlin.coroutines.startCoroutine
import kotlin.js.JsAny
import kotlin.js.Promise

/**
 * Where the Karma harness serves the prelinked Emscripten module.
 *
 * Paired with the proxy in `karma.config.d/maplibre-browser-module.js`. The wasm and the ABI
 * manifest sit beside it, which is what the loader expects of any deployment.
 */
private const val MODULE_URL: String = "/maplibre/maplibre_native_c.mjs"

@JsFun("(message) => new Error(message)") private external fun jsError(message: String): JsAny

/**
 * Runs [block] as one test, with the browser module loaded.
 *
 * The browser binding's entry points are suspending, because loading a module and parking a stack
 * on native work are both promise-shaped. A test therefore returns the promise its body settles,
 * which is what the test framework waits on; a test that called the body and returned would report
 * success before the body had run.
 *
 * Loading is memoized by the loader itself, so every test names the module and the first one to
 * arrive is the one that fetches it.
 */
internal fun browserTest(block: suspend () -> Unit): Promise<JsAny?> = Promise { resolve, reject ->
  val body: suspend () -> Unit = {
    Maplibre.loadNativeLibraryAsync(MODULE_URL)
    block()
  }
  body.startCoroutine(
    Continuation(EmptyCoroutineContext) { result ->
      result.fold(
        onSuccess = { resolve(null) },
        // Carried across as a JavaScript error, which is the only failure shape a promise has.
        // The Kotlin type and stack are written into the message so the report still names the
        // line that failed.
        onFailure = { reject(jsError("$it\n${it.stackTraceToString()}")) },
      )
    }
  )
}

/**
 * Runs [task] on the page's next browser task.
 *
 * Used to observe the page while a scope is parked. A parked scope resumes from the event loop, so
 * a task that never ran would say the page had stopped servicing it.
 */
@JsFun("(task) => { globalThis.setTimeout(task, 0) }")
internal external fun runOnNextPageTask(task: () -> Unit)

/** Milliseconds on the page's clock, for measuring how long a parked call actually waited. */
@JsFun("() => Date.now()") internal external fun pageTimeMillis(): Double
