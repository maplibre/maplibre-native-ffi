package org.maplibre.nativeffi.internal.wasm

import kotlin.js.ExperimentalWasmJsInterop
import kotlin.js.JsAny
import kotlin.js.Promise
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.StructLayouts

/**
 * The prelinked Emscripten module this binding calls.
 *
 * Every other platform loads a shared library and resolves symbols from it. A browser has no such
 * step: the module is linked by the same emsdk that built the C API, ships as an ES module beside
 * its wasm, and is instantiated by a factory that returns a promise. That promise is why loading
 * cannot present the synchronous face the other platforms do -- the pthread pool spawns before the
 * factory resolves, and nothing on a page may wait for it.
 *
 * The module object is held here rather than passed around because it is process-global in exactly
 * the way the native library is on every other platform.
 */
@OptIn(ExperimentalWasmJsInterop::class) internal external interface MaplibreNativeCModule : JsAny

@OptIn(ExperimentalWasmJsInterop::class)
@JsFun("() => globalThis.__maplibreNativeC ?? null")
private external fun instance(): MaplibreNativeCModule?

/**
 * Verifies and instantiates the module as one memoized operation.
 *
 * Memoizing here rather than in Kotlin is what makes concurrent loads safe. The whole operation --
 * fetching the manifest, comparing digests, importing, and running the factory -- has to be what a
 * second caller joins; a marker set only around the factory leaves both callers free to run the
 * verification and then instantiate twice, and the second instance would replace the first while
 * handles created against it were still live. JavaScript runs this check-and-set without
 * interleaving, so the first caller to arrive publishes the promise every later one awaits.
 *
 * A rejected load clears the memo, so a transient network failure can be retried. The digest check
 * happens before the import, so a mismatched module is never instantiated at all.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun(
  """
  (url, expectedDigest, expectedProtocol, requiredNames, runtimeNames) => {
    const required = requiredNames.split(',')
    const runtime = runtimeNames.split(',')
    // Resolved against the document first: a relative module URL is the normal thing for a host to
    // pass, and deriving the manifest URL from a relative base throws before anything is fetched.
    // A blob-backed worker has a `blob:` location, which cannot resolve a relative reference, so
    // the fallback is chosen by trying rather than by whether a location exists at all.
    const bases = [globalThis.location ? globalThis.location.href : null, import.meta.url]
    let resolved = null
    for (const base of bases) {
      if (!base) continue
      try { resolved = new URL(url, base).href; break } catch (error) { /* try the next base */ }
    }
    if (resolved === null) {
      throw new Error('cannot resolve ' + url + ' against this context')
    }
    url = resolved
    if (globalThis.__maplibreNativeCLoading) return globalThis.__maplibreNativeCLoading
    // The manifest beside the module is a preflight, not the authority: rejecting a mismatch here
    // avoids starting a 16-worker pthread pool for a module that is about to be refused, and a
    // refused instance has no shutdown path. The module's own digest is still checked below, since
    // a sidecar only vouches for whatever file happens to sit next to it.
    const loading = fetch(new URL('maplibre_native_c-abi.json', url).href)
      .then((response) => {
        if (!response.ok) {
          // Refused rather than skipped. The manifest ships with the module, so its absence means
          // a broken deployment -- and proceeding would start a sixteen-worker pthread pool for a
          // module that may then be rejected with no way to shut it down.
          throw new Error(
            'no ABI manifest beside the module at ' + url + ', so the binding cannot check what ' +
            'it is about to instantiate')
        }
        return response.json()
      })
      .then((manifest) => {
        if (String(manifest.headersDigest) !== expectedDigest) {
          throw new Error(
            'the module at ' + url + ' was built from different headers than this binding was ' +
            'generated from (manifest ' + manifest.headersDigest + ', binding ' + expectedDigest +
            ')')
        }
        // Protocol and helpers can change without a header changing, so the preflight checks them
        // too rather than leaving them to be discovered after the workers have started.
        if (Number(manifest.dispatchProtocol) !== expectedProtocol) {
          throw new Error(
            'the module at ' + url + ' packs calls for protocol ' + manifest.dispatchProtocol +
            ', but this binding packs for ' + expectedProtocol)
        }
        for (const name of required) {
          if (name[0] === '_' && !(name.slice(1) in manifest.functions)) {
            throw new Error(
              'the module at ' + url + ' does not carry ' + name.slice(1) + ', so it was not ' +
              'built as a browser module this binding can drive')
          }
        }
        // webpackIgnore keeps a bundler's hands off this. The module is fetched from wherever the
        // host deployed it, at a URL only known at run time, so a bundler that treats this as a
        // build-time dependency rewrites it to its own resolver and the load fails with the URL
        // reported as a missing module. Kotlin's own wasmJs browser toolchain runs webpack, so
        // this affects every host, not just this repository's tests.
        return import(/* webpackIgnore: true */ url)
      })
      .then((factory) => factory.default({ locateFile: (path) => new URL(path, url).href }))
      .then((module) => {
        // Checked before the module becomes reachable, so a failure leaves no
        // half-usable instance behind for a caller that retries after catching
        // it. The digest settles the headers; these settle how a call is packed
        // and whether the browser support this binding needs is present at all.
        for (const name of required) {
          if (typeof module[name] !== 'function') {
            throw new Error(
              'the module at ' + url + ' is missing ' + name + ', so it was not built as a ' +
              'browser module this binding can drive')
          }
        }
        for (const name of runtime) {
          if (module[name] === undefined) {
            throw new Error(
              'the module at ' + url + ' is missing the ' + name + ' runtime helper, so it was ' +
              'not built as a browser module this binding can drive')
          }
        }
        // The authority. The preflight above only saw a file beside the module, which a cache or
        // a partial deploy can make a different generation entirely; this is the module itself.
        const digest = module.UTF8ToString(module._mln_browser_headers_digest())
        if (digest !== expectedDigest) {
          throw new Error(
            'the module at ' + url + ' was built from different headers than this binding was ' +
            'generated from (module ' + digest + ', binding ' + expectedDigest + ')')
        }
        const actual = module._mln_browser_dispatch_protocol()
        if (actual !== expectedProtocol) {
          throw new Error(
            'the module at ' + url + ' packs calls for protocol ' + actual + ', but this ' +
            'binding packs for ' + expectedProtocol)
        }
        globalThis.__maplibreNativeC = module
        return null
      })
      .catch((error) => { globalThis.__maplibreNativeCLoading = null; throw error })
    globalThis.__maplibreNativeCLoading = loading
    return loading
  }
"""
)
private external fun verifyAndInstantiate(
  url: String,
  expectedDigest: String,
  expectedProtocol: Int,
  requiredNames: String,
  runtimeNames: String,
): Promise<JsAny?>

/**
 * Reports whether the browser supports the WebAssembly suspension this binding needs.
 *
 * The binding presents the same synchronous API as every other platform by parking a Kotlin stack
 * on a promise, which is a virtual-machine feature rather than a library one. A browser without it
 * cannot run this binding at all, so it is detected once, at load, rather than surfacing later as a
 * trap inside an ordinary map call.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun(
  "() => typeof WebAssembly.Suspending === 'function' && typeof WebAssembly.promising === 'function'"
)
private external fun supportsSuspension(): Boolean

internal object BrowserModule {
  /**
   * Returns the loaded module, or reports that the host never loaded it.
   *
   * Every entry point that reaches native goes through this, so a host that forgot to await the
   * loader gets one clear failure rather than a null dereference inside interop.
   */
  fun require(): MaplibreNativeCModule =
    instance()
      ?: throw Status.invalidState(
        "The MapLibre Native browser module is not loaded. Await " +
          "Maplibre.loadNativeLibraryAsync() before calling this binding."
      )

  /** Reports whether [require] would succeed. */
  fun isLoaded(): Boolean = instance() != null

  /**
   * Instantiates the module from [url], which names the ES module beside its wasm and manifest.
   *
   * Loading twice is not an error: the module is process-global, and every caller that arrives
   * while a load is in flight joins that one rather than starting another.
   */
  suspend fun load(url: String) {
    if (isLoaded()) return
    if (!supportsSuspension()) {
      throw Status.invalidState(
        "This browser does not support the WebAssembly JavaScript Promise Integration this " +
          "binding requires. Chrome 137, Firefox 139, or a newer browser is needed."
      )
    }
    verifyAndInstantiate(
        url,
        StructLayouts.HEADERS_DIGEST,
        NativeCall.EXPECTED_PROTOCOL,
        REQUIRED_EXPORTS.joinToString(","),
        REQUIRED_RUNTIME.joinToString(","),
      )
      .awaitOrThrow()
  }

  /**
   * The module entry points this binding cannot work without.
   *
   * A module built from the same headers and the same call protocol can still have been linked
   * without the browser helpers. Checking for them at load turns that into one load failure naming
   * the missing entry point, rather than an undefined JavaScript property at the first call that
   * needs it.
   */
  private val REQUIRED_EXPORTS =
    listOf(
      // The generic call path.
      "_mln_browser_dispatch_protocol",
      "_mln_browser_headers_digest",
      "_mln_browser_entry_index",
      "_mln_browser_entry_slots",
      "_mln_browser_entry_total",
      "_mln_browser_invoke_here",
      // The owner thread every runtime-affine call runs on. It is created with the page canvases a
      // host will render onto, because a browser transfers a canvas to a thread only as that
      // thread is created.
      "_mln_browser_dispatcher_create_with_canvases",
      "_mln_browser_dispatcher_submit",
      "_mln_browser_dispatcher_take_completion",
      "_mln_browser_dispatcher_stop",
      // The WebGL contexts a render target draws through, and the GL work a host does with what one
      // rendered. All of it has to run on that same owner thread, because a WebGL context belongs
      // to the thread that made it and shares nothing with any other context.
      "_mln_browser_webgl_context_create",
      "_mln_browser_webgl_context_destroy",
      "_mln_browser_webgl_canvas_resize",
      "_mln_browser_webgl_texture_create",
      "_mln_browser_webgl_texture_destroy",
      "_mln_browser_webgl_present_texture",
      "_mln_browser_webgl_read_pixels",
      // The log queue.
      "_mln_browser_log_install",
      "_mln_browser_log_take_since",
      "_mln_browser_log_mark",
      "_mln_browser_log_take_dropped",
      // The synchronous callbacks native invokes from its own threads, which reach the page
      // through the module rather than through a trampoline a worker cannot call.
      "_mln_browser_sync_provider_install",
      "_mln_browser_sync_provider_thunk",
      "_mln_browser_sync_transform_install",
      "_mln_browser_sync_transform_thunk",
      // Entry points this binding calls directly rather than through the table,
      // because they touch no runtime state and so have no owner thread to reach.
      "_mln_thread_last_error_message",
      "_mln_adapter_log_record_destroy",
      "_mln_render_target_extent_physical_size",
      "_mln_wake_source_signal",
      "_mln_wake_source_destroy",
      // The allocator every descriptor and argument buffer comes from.
      "_malloc",
      "_free",
    )

  /**
   * Emscripten runtime helpers this binding uses.
   *
   * These are not C entry points, so a module linked without them fails the same way a missing
   * export would -- an undefined property at the first call that needs one -- and they are checked
   * at load for the same reason.
   */
  private val REQUIRED_RUNTIME =
    listOf(
      "HEAPU8",
      "HEAPU16",
      "HEAPU32",
      "HEAPF32",
      "HEAPF64",
      "UTF8ToString",
      "stringToUTF8",
      "lengthBytesUTF8",
      // Installing a host callback puts a trampoline into the module's function table and takes it
      // out again. Without these two, a module loads and works until the first host callback is
      // registered, which is exactly the late, misattributed failure this list exists to prevent.
      "addFunction",
      "removeFunction",
    )
}
