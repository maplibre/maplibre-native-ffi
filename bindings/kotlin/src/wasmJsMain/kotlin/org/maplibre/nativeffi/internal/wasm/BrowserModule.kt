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
 * Checks the module already on this page against what this binding was generated for.
 *
 * The module is a page global, so it can be one this binding never loaded: another separately
 * bundled copy of the binding may have loaded it, and so may a host written against the module
 * directly. The C ABI version cannot settle whether such a module is the right one -- it stays 0
 * for the whole prerelease -- so the digest, the call protocol, and the entry points are what say
 * so, and they are the same three the load path checks on a module it built itself.
 *
 * Nothing here fetches or instantiates anything: it reads properties off a module that already
 * exists and calls two of its entry points. That is what makes it safe to run before the preflight
 * that keeps a doomed module from starting a worker pool, because there is no pool left to start.
 *
 * Returns null when the page carries no module, an empty string when it carries an acceptable one,
 * and otherwise the reason it is refused. Reported rather than thrown, so the failure reaches a
 * host as this binding's own exception instead of as a JavaScript error the Kotlin end cannot
 * classify.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun(
  """
  (expectedDigest, expectedProtocol, requiredNames, runtimeNames) => {
    const module = globalThis.__maplibreNativeC
    if (!module) return null
    const here = 'The MapLibre Native browser module already loaded on this page '
    // Checked before anything is called out of the module, because the digest below is read through
    // two of the names this loop is what vouches for.
    for (const name of requiredNames.split(',')) {
      if (typeof module[name] !== 'function') {
        return here + 'is missing ' + name + ', so it was not built as a browser module this ' +
          'binding can drive.'
      }
    }
    for (const name of runtimeNames.split(',')) {
      if (module[name] === undefined) {
        return here + 'is missing the ' + name + ' runtime helper, so it was not built as a ' +
          'browser module this binding can drive.'
      }
    }
    const digest = module.UTF8ToString(module._mln_browser_headers_digest())
    if (digest !== expectedDigest) {
      return here + 'was built from different headers than this binding was generated from ' +
        '(module ' + digest + ', binding ' + expectedDigest + '). One module serves the whole ' +
        'page, so a binding generated against another set of headers would write descriptors at ' +
        'offsets that are not this module\'s.'
    }
    const protocol = module._mln_browser_dispatch_protocol()
    if (protocol !== expectedProtocol) {
      return here + 'packs calls for protocol ' + protocol + ', but this binding packs for ' +
        expectedProtocol + '.'
    }
    return ''
  }
"""
)
private external fun verifyIncumbent(
  expectedDigest: String,
  expectedProtocol: Int,
  requiredNames: String,
  runtimeNames: String,
): String?

/**
 * Takes the next binding-instance number from the page.
 *
 * The counter is a page global and every instance reads it through this, so two separately bundled
 * copies of the binding cannot mint the same number however they interleave. A number rather than
 * an object, because it survives the boundary as a value and needs no identity to be preserved.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun(
  "() => (globalThis.__maplibreNativeCInstances = (globalThis.__maplibreNativeCInstances ?? 0) + 1)"
)
private external fun nextInstanceId(): Int

/**
 * Claims the page for [id], and reports which instance holds it.
 *
 * Returns zero when [id] has just taken an unclaimed page, and otherwise the id of whichever
 * instance holds it -- which is [id] itself for the instance that claimed it earlier. Zero is never
 * a minted id, because [nextInstanceId] counts from one.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun(
  """
  (id) => {
    const owner = globalThis.__maplibreNativeCOwner
    if (owner === undefined) {
      globalThis.__maplibreNativeCOwner = id
      return 0
    }
    return owner
  }
"""
)
private external fun claimPage(id: Int): Int

/**
 * Terminates the module's worker pool and drops the page's last reference to it, as one step.
 *
 * One step because neither half is safe alone. Terminating kills each worker wherever it happens to
 * be, and a worker killed inside the module's allocator leaves its lock held in shared memory,
 * where the next allocation on the page would block forever on a thread that may not block.
 * Dropping the reference is what guarantees there is no next allocation. Waiting for the owner
 * thread first -- see [awaitOwnerThreadRelease] -- keeps the one worker whose teardown the page
 * knows about out of that window, and cannot cover the rest: a thread's own exit path allocates
 * too, and every MapLibre worker is somewhere the page cannot see.
 *
 * The reference goes first, and the memo with it, so a terminate that throws still leaves nothing
 * reachable. The flag is what every entry point reads afterwards: it separates a page that has
 * released its module from one that never loaded, which are the same absence and need opposite
 * answers.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun(
  """
  () => {
    const module = globalThis.__maplibreNativeC
    globalThis.__maplibreNativeC = null
    globalThis.__maplibreNativeCLoading = null
    globalThis.__maplibreNativeCReleased = true
    // Optional throughout, because this also runs on a page that never finished loading a module,
    // and a module being refused is exactly one that may not carry the helper.
    module?.PThread?.terminateAllThreads?.()
  }
"""
)
private external fun releaseInstance()

@OptIn(ExperimentalWasmJsInterop::class)
@JsFun("() => globalThis.__maplibreNativeCReleased === true")
private external fun isReleased(): Boolean

/**
 * Reports how many stopped owner threads have not finished releasing themselves.
 *
 * Read off the page global rather than through [BrowserModule.require], because the one caller is a
 * shutdown: a refusal there has nobody left to report to, and a page with no module has nothing
 * left to wait for either, which is what the zero says.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun("() => globalThis.__maplibreNativeC?._mln_browser_dispatcher_pending_stops() ?? 0")
private external fun pendingStops(): Int

/**
 * Resolves on the next page task, which is the cadence the wait below polls on.
 *
 * A task rather than a microtask, because what it is waiting for happens on another thread and
 * reaches this one as a message. Draining microtasks would spin without ever letting one arrive.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun("() => new Promise((resolve) => { setTimeout(() => resolve(null), 0) })")
private external fun nextPageTask(): Promise<JsAny?>

@OptIn(ExperimentalWasmJsInterop::class)
@JsFun("() => Date.now()")
private external fun nowMillis(): Double

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
 * happens before the import, so a mismatched module is never instantiated at all, and a rejection
 * that does reach an instance terminates that instance's worker pool before it rethrows.
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
    // Joined rather than repeated, and joining is not the same as accepting: the load in flight was
    // started by whoever arrived first, with that caller's digest, protocol, and URL. So a caller
    // that joins one has checked nothing yet, and BrowserModule.load checks the module this
    // resolves against its own expectations before it returns.
    if (globalThis.__maplibreNativeCLoading) return globalThis.__maplibreNativeCLoading
    // The manifest beside the module is a preflight, not the authority: rejecting a mismatch here
    // means a module that is about to be refused never starts a 16-worker pthread pool at all,
    // rather than starting one that the catch below has to terminate. The module's own digest is
    // still checked there, since a sidecar only vouches for whatever file happens to sit next to
    // it.
    const loading = fetch(new URL('maplibre_native_c-abi.json', url).href)
      .then((response) => {
        if (!response.ok) {
          // Refused rather than skipped. The manifest ships with the module, so its absence means
          // a broken deployment, and proceeding would start a sixteen-worker pthread pool for a
          // module that may then be rejected anyway.
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
        try {
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
        } catch (error) {
          // The factory spawns the worker pool before it resolves, so by the time any of the
          // checks above can run there are sixteen workers alive that only this instance refers
          // to. Clearing the memo below lets a host retry, and a retry that left the pool standing
          // would add sixteen more each time -- which is what a CDN serving a stale wasm behind a
          // fresh manifest produces. Emscripten's own pool teardown is what `exit` runs, and the
          // link exports it for this; a module that reaches here without it is one of the modules
          // this binding is in the middle of refusing, so its absence is tolerated rather than
          // reported over the failure that matters.
          module.PThread?.terminateAllThreads?.()
          throw error
        }
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

/**
 * Reports whether this page may use the shared memory the module's threads are built on.
 *
 * The module owns a pthread, and Emscripten builds one out of a worker and a `SharedArrayBuffer`
 * heap. A browser exposes that buffer only to a cross-origin isolated document, so a deployment
 * that serves the page without `Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` is
 * one where the module can be fetched and instantiated and can then never start a thread.
 *
 * Both halves are asked for because they fail at different moments: without the buffer the factory
 * itself throws while it allocates the heap, and without isolation a worker that receives the heap
 * is refused it. The remedy is the same one, which is why they share a message.
 */
@OptIn(ExperimentalWasmJsInterop::class)
@JsFun("() => typeof SharedArrayBuffer === 'function' && globalThis.crossOriginIsolated === true")
private external fun supportsSharedMemory(): Boolean

internal object BrowserModule {
  /**
   * This binding instance's number on the page, taken once.
   *
   * Read on the first use of this object, which for any binding instance is far ahead of anything
   * that could matter, so which instance holds the page is settled by which one starts using the
   * binding rather than by which bundle the page evaluated first.
   */
  private val instanceId: Int = mintInstanceId()

  /**
   * Returns the loaded module, or reports that the host never loaded it.
   *
   * Every entry point that reaches native goes through this, so a host that forgot to await the
   * loader gets one clear failure rather than a null dereference inside interop. It is also where a
   * second binding instance is caught whatever route it took: an instance that never called the
   * loader still has to come through here before its first call reaches native, because it resolves
   * every entry point for the first time itself.
   *
   * Ownership is asked about before the module is, because a second instance is wrong whether or
   * not a module is loaded, and telling it to await a loader it must not call would send it after
   * the wrong problem.
   */
  fun require(): MaplibreNativeCModule {
    checkSoleBinding()
    checkNotReleased()
    return instance()
      ?: throw Status.invalidState(
        "The MapLibre Native browser module is not loaded. Await " +
          "Maplibre.loadNativeLibraryAsync() before calling this binding."
      )
  }

  /** Reports whether the module is on the page, which a release makes false again. */
  fun isLoaded(): Boolean = instance() != null

  /**
   * Terminates the module's worker pool and drops this page's reference to it.
   *
   * A shutdown stops the thread this binding's runtimes ran on; this is what releases the module
   * they ran inside. Sixteen workers and a heap that starts at half a gigabyte stay reachable
   * otherwise, for as long as the document lives, which on a single-page host is the whole session.
   *
   * **Final, and the last thing that may touch the module.** Terminating a worker inside the
   * module's allocator would leave its lock held, so this drops the page's reference in the same
   * step and every entry point refuses afterwards -- including the loader, which would otherwise
   * instantiate a second sixteen-worker module beside the one just released.
   *
   * **Nothing on a page task may still be reaching the module.** A stack parked on the owner thread
   * is already excluded, because a shutdown takes the gate a scope holds. A repeating page task is
   * not: the log drain reschedules itself for as long as a callback is installed, and it calls the
   * module directly, so it has to have stopped before this runs.
   */
  internal fun discardAfterShutdown() {
    releaseInstance()
  }

  /**
   * What the last [awaitOwnerThreadRelease] saw, and false before the first one.
   *
   * True says the owner thread finished releasing itself, or that there was nothing to release.
   * False says the wait ran out first, which leaves a thread that was still mid-teardown when the
   * pool was terminated.
   *
   * This is the only place the outcome is recorded. A shutdown reports nothing about it, because
   * there is nothing a host could do with it: the module goes either way, and the host's own state
   * is the same afterwards. The suite's final-shutdown test reads it, because it is what turns "the
   * stop was posted" into "the stop ran".
   */
  internal var ownerThreadReleased: Boolean = false
    private set

  /**
   * Waits for every stopped owner thread to finish releasing itself, for up to a second.
   *
   * A stop is fire-and-forget -- it posts a wake to the owner thread and returns -- so without this
   * the pool is terminated with no idea whether that wake ran. A worker killed before it does never
   * drains what was queued, never drops the keepalive that ends it, and never frees the dispatcher.
   * The module counts a stopped dispatcher until its own teardown has run, and that count reaching
   * zero is what this waits for.
   *
   * **Polled from page tasks rather than blocked on.** A page may not block, and what it is waiting
   * for is a worker that reports through shared memory, so this returns to the event loop between
   * reads. That also keeps the page able to answer a synchronous callback from a call that was
   * still queued when the stop arrived.
   *
   * **Bounded, and expiry is not a failure.** A worker can be inside a call queued before the stop,
   * or gone before it could acknowledge anything, and a shutdown that waited for either would hang
   * the page rather than release the module. The bound is generous next to the single message turn
   * a stopped thread needs, so reaching it means something is wrong rather than slow.
   */
  internal suspend fun awaitOwnerThreadRelease() {
    val deadline = nowMillis() + OWNER_THREAD_RELEASE_TIMEOUT_MILLIS
    while (pendingStops() != 0) {
      if (nowMillis() >= deadline) {
        ownerThreadReleased = false
        return
      }
      nextPageTask().awaitOrThrow()
    }
    ownerThreadReleased = true
  }

  /**
   * Refuses a page whose module has been released.
   *
   * Separated from "never loaded" because the remedy is opposite. A page that never loaded is told
   * to await the loader; a page that released is told that awaiting it again is the one thing that
   * would make this worse, since a page instantiates one module and a second is another pool and
   * another heap beside the one just given back.
   */
  private fun checkNotReleased() {
    if (!isReleased()) return
    throw Status.invalidState(
      "The MapLibre Native browser module has been released. Shutting down terminated its worker " +
        "pool and dropped this page's last reference to it, which cannot be undone: loading again " +
        "would instantiate a second module beside the one just released rather than recover it. A " +
        "host that wants a map again reloads the document."
    )
  }

  /**
   * Mints the number a binding instance identifies itself by.
   *
   * Called once by this instance, for [instanceId]. It is also the seam the test that refuses a
   * second instance uses: minting another number is exactly what a second instance does, and it is
   * the only part of being one that a suite running in a single WebAssembly instance can reproduce.
   * Minting alone claims nothing, so a caller that only mints leaves the page as it found it.
   */
  internal fun mintInstanceId(): Int = nextInstanceId()

  /**
   * Refuses a page that another binding instance already owns.
   *
   * The module and the map of calls parked on it are page globals, while everything that indexes
   * into them -- the dispatcher handle, the call tokens, the callback registrations -- belongs to
   * one WebAssembly instance. Two separately bundled instances therefore share half their state and
   * duplicate the other half: both start issuing call tokens at one, the second overwrites the
   * first's resolver in the shared map, and a completion then resumes the wrong caller with the
   * wrong output while the right one parks forever.
   *
   * Refused rather than isolated. A second instance on one page is a deployment mistake -- the same
   * binding bundled twice, or two versions of it -- and a host can fix it by loading the binding
   * once and sharing it. Isolating would mean giving each instance its own module, and a page
   * cannot have two: a canvas is transferred to a thread once, the log callback is process-global
   * to the module, and the synchronous callback hosts are module globals as well.
   *
   * [candidate] is this instance's number. The test that says a second instance is refused passes
   * one of its own, which is what a second instance would arrive with.
   */
  internal fun checkSoleBinding(candidate: Int = instanceId) {
    val owner = claimPage(candidate)
    if (owner == 0 || owner == candidate) return
    throw Status.invalidState(
      "Another instance of the MapLibre Native browser binding already owns this page. The module " +
        "and the calls parked on it are shared by the whole page, while the tokens and handles " +
        "that index into them belong to one WebAssembly instance, so a second instance would " +
        "resolve the first's calls with its own results. Load this binding once and share it " +
        "across the page."
    )
  }

  /**
   * Checks the module on this page against what this binding was generated for, if there is one.
   *
   * Returns whether a module was there, and throws when one was there and does not match. Every
   * path that hands a caller a module goes through this: the one that instantiates it, the one that
   * finds it already loaded, and the one that joins a load another caller started.
   *
   * The two expectations are parameters, here and in [load], so that the guard can be exercised
   * against values no loadable module reports, the way
   * [org.maplibre.nativeffi.Maplibre.checkCompatibleCAbi] is. This page serves one module, and it
   * is the matching one, so a mismatch has no other way to be reached from a test.
   */
  internal fun checkLoadedModule(
    expectedDigest: String = StructLayouts.HEADERS_DIGEST,
    expectedProtocol: Int = NativeCall.EXPECTED_PROTOCOL,
  ): Boolean {
    val problem =
      verifyIncumbent(
        expectedDigest,
        expectedProtocol,
        REQUIRED_EXPORTS.joinToString(","),
        REQUIRED_RUNTIME.joinToString(","),
      ) ?: return false
    if (problem.isNotEmpty()) throw Status.invalidState(problem)
    return true
  }

  /**
   * Instantiates the module from [url], which names the ES module beside its wasm and manifest.
   *
   * Loading twice is not an error: the module is process-global, and every caller that arrives
   * while a load is in flight joins that one rather than starting another. What a caller gets that
   * way is still checked against this binding's own digest and call protocol, because the module it
   * joins was loaded for somebody else's.
   *
   * [expectedDigest] and [expectedProtocol] are what this binding was generated for. They are
   * parameters for the reason [checkLoadedModule]'s are, and they are what makes the *placement* of
   * that check testable rather than only the check itself: a test that loads with a digest no
   * module carries reaches this with the suite's module already on the page, which is exactly the
   * path that used to return without looking.
   */
  suspend fun load(
    url: String,
    expectedDigest: String = StructLayouts.HEADERS_DIGEST,
    expectedProtocol: Int = NativeCall.EXPECTED_PROTOCOL,
  ) {
    // First, because it is the one refusal that has nothing to do with the browser, the page, or
    // the module: a second binding instance is a deployment mistake whatever else is true, and it
    // is this binding's own state that makes it one.
    checkSoleBinding()
    // Before the preflights, because a released page fails them or passes them irrelevantly: what a
    // load would do here is build a second module beside the one this page just gave back, and no
    // browser capability changes that.
    checkNotReleased()
    // Both preflights are a property read, and both come before the fetch that starts the load,
    // because the factory spawns a sixteen-worker pool before it resolves and neither of these
    // failures is one those workers could recover from. The browser is asked about first: a browser
    // too old for suspension cannot run this binding whatever the page's headers say, so telling
    // such a host to change its headers would send it after the wrong problem.
    if (!supportsSuspension()) {
      throw Status.invalidState(
        "This browser does not support the WebAssembly JavaScript Promise Integration this " +
          "binding requires. Chrome 137, Firefox 139, or a newer browser is needed."
      )
    }
    if (!supportsSharedMemory()) {
      throw Status.invalidState(
        "This page is not cross-origin isolated, so the MapLibre Native browser module cannot " +
          "start the threads it runs on. Serve the document with the headers " +
          "\"Cross-Origin-Opener-Policy: same-origin\" and " +
          "\"Cross-Origin-Embedder-Policy: require-corp\", and serve every resource it embeds " +
          "so that those headers allow it."
      )
    }
    // A module already here is checked rather than accepted on sight. It may be one this binding
    // never loaded -- another copy of the binding, or a host driving the module directly -- and the
    // C ABI version cannot tell them apart, because it stays 0 for the whole prerelease. This costs
    // nothing on the path that matters: an unloaded page reads one undefined property and goes on.
    if (checkLoadedModule(expectedDigest, expectedProtocol)) return
    verifyAndInstantiate(
        url,
        expectedDigest,
        expectedProtocol,
        REQUIRED_EXPORTS.joinToString(","),
        REQUIRED_RUNTIME.joinToString(","),
      )
      .awaitOrThrow()
    // Checked again, because the load above may have been somebody else's. A caller that arrives
    // while a load is in flight joins it, and that load compared the module against the digest and
    // protocol of whoever started it. This is that same comparison against this binding's own, and
    // the only one covering a module this call did not build.
    if (!checkLoadedModule(expectedDigest, expectedProtocol)) {
      throw Status.invalidState(
        "The MapLibre Native browser module finished loading but is no longer on this page. " +
          "Something on the page removed it between the load resolving and this check."
      )
    }
  }

  /**
   * How long a shutdown waits for a stopped owner thread, in milliseconds.
   *
   * A stopped thread needs one message turn to run the wake it was sent, so this is three orders of
   * magnitude more than the wait costs when everything works, and it is spent only by a page that
   * is being torn down anyway.
   */
  private const val OWNER_THREAD_RELEASE_TIMEOUT_MILLIS = 1000.0

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
      // What a stop reports through, since the stop itself returns before the thread has run it.
      "_mln_browser_dispatcher_pending_stops",
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
      // The asynchronous callbacks native invokes per tile, from the worker its custom geometry
      // source's tile loader runs on. Nothing waits for these, so they are posted to the page
      // rather than proxied synchronously; see src/browser/custom_geometry.c.
      "_mln_browser_custom_geometry_install",
      "_mln_browser_custom_geometry_fetch_thunk",
      "_mln_browser_custom_geometry_cancel_thunk",
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
      // The worker pool's teardown, which is the only way to release the sixteen workers a module
      // spawned before this binding could decide whether to keep it. See the catch in
      // verifyAndInstantiate.
      "PThread",
    )
}
