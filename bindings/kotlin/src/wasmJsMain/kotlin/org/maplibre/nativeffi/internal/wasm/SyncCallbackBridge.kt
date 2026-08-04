package org.maplibre.nativeffi.internal.wasm

import kotlin.wasm.WasmExport
import org.maplibre.nativeffi.error.InvalidArgumentException
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.callback.CallbackGate
import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceTransformResponse
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceRequestHandle
import org.maplibre.nativeffi.resource.ResourceTransformCallback

/**
 * Places this module's provider trampoline in the browser module's function table.
 *
 * `wasmExports` names this module's raw WebAssembly exports, so what reaches `addFunction` is a
 * WebAssembly function rather than a JavaScript closure. Emscripten stores such a function in its
 * table directly, and the signature is the fallback it uses for a function it has to wrap: `i` for
 * each 32-bit value, and `j` for the 64-bit request handle.
 *
 * The entry belongs to the agent that added it, which here is the page. That is exactly the
 * property `src/browser/sync_callback.c` proxies for: a MapLibre worker reaches this entry by
 * asking the main runtime thread to make the call, rather than by calling it itself.
 */
@JsFun(
  "() => globalThis.__maplibreNativeC.addFunction(" +
    "wasmExports['mln_browser_resource_provider_host'], 'iiij')"
)
private external fun addProviderTrampoline(): Int

@JsFun(
  "() => globalThis.__maplibreNativeC.addFunction(" +
    "wasmExports['mln_browser_resource_transform_host'], 'iiiii')"
)
private external fun addTransformTrampoline(): Int

@JsFun("(index) => { globalThis.__maplibreNativeC.removeFunction(index) }")
private external fun removeTrampoline(index: Int)

@JsFun("(host) => globalThis.__maplibreNativeC._mln_browser_sync_provider_install(host)")
private external fun installProviderHost(host: Int): Boolean

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_sync_provider_thunk()")
private external fun providerThunk(): Int

@JsFun("(host) => globalThis.__maplibreNativeC._mln_browser_sync_transform_install(host)")
private external fun installTransformHost(host: Int): Boolean

@JsFun("() => globalThis.__maplibreNativeC._mln_browser_sync_transform_thunk()")
private external fun transformThunk(): Int

/**
 * The decision the C API turns into a provider error response.
 *
 * It is deliberately outside the decision enum: a request the host neither served nor passed
 * through has no decision to report, and the C API answers it by releasing the handle and failing
 * the request rather than by loading the resource itself.
 */
private const val UNKNOWN_DECISION = -1

/** Slots `mln_resource_transform_response_set_url` reads: the response, the URL, and its length. */
private const val SET_URL_SLOTS = 3

/**
 * The trampoline MapLibre's resource provider reaches, by way of the module's proxy.
 *
 * A raw WebAssembly export rather than a `@JsExport`, so that the browser module's function table
 * holds this module's function itself. The call then travels from the worker to the page and into
 * Kotlin without passing through JavaScript at all, and the 64-bit handle keeps its bits.
 *
 * Nothing may unwind out of a WebAssembly export: the C frame that called it belongs to a waiting
 * MapLibre worker, which has no handler, and the trap would take the module down with it.
 */
@WasmExport("mln_browser_resource_provider_host")
internal fun mlnBrowserResourceProviderHost(userData: Int, request: Int, handle: Long): Int =
  try {
    ResourceProviderBridge.dispatch(userData, HeapPointer(request), handle)
  } catch (_: Throwable) {
    UNKNOWN_DECISION
  }

/** The trampoline MapLibre's resource URL transform reaches, for the reasons above. */
@WasmExport("mln_browser_resource_transform_host")
internal fun mlnBrowserResourceTransformHost(
  userData: Int,
  kind: Int,
  url: Int,
  outResponse: Int,
): Int =
  try {
    ResourceTransformBridge.dispatch(userData, kind, HeapPointer(url), HeapPointer(outResponse))
  } catch (_: Throwable) {
    MaplibreStatus.NATIVE_ERROR.nativeCode
  }

/**
 * Names both trampolines from Kotlin, so the linker keeps their exports.
 *
 * Calling them rather than merely mentioning them, because a mention of a function that is never
 * invoked is itself removable. Token 0 is never issued — [SyncCallbackHost] counts from one — so
 * each looks for a registration, finds none, and returns without reading any of its pointer
 * arguments. That is what makes calling them here safe as well as sufficient.
 */
private fun retainTrampolines() {
  mlnBrowserResourceProviderHost(0, 0, 0L)
  mlnBrowserResourceTransformHost(0, 0, 0, 0)
}

/**
 * The module-global half of one family of synchronous callbacks.
 *
 * The callback a runtime registers is the module's own thunk, and the thunk forwards to one host
 * function pointer this installs. That pointer is module-global while a registration is a
 * runtime's, so this owns the trampoline and hands each registration a token to be reached by.
 * Native carries the token back as the `user_data` it was registered with, which is what lets one
 * trampoline serve a registration that is being replaced and the one replacing it at the same time.
 *
 * The trampoline is added when the first registration arrives and removed when the last one goes.
 * `sync_callback.c` requires a host to install before it registers and to clear after it has
 * cleared the registration, and both halves of that ordering are the caller's: it registers with
 * the runtime only after [add] has returned, and it calls [remove] only after the registration is
 * gone.
 *
 * A plain map and a plain counter are the whole implementation, because a Kotlin/Wasm module runs
 * on one thread and every call here is made from the page.
 */
private class SyncCallbackHost<T : Any>(
  private val subject: String,
  private val addTrampoline: () -> Int,
  private val installHost: (Int) -> Boolean,
) {
  private val registrations = mutableMapOf<Int, T>()
  private var trampoline = 0
  private var nextToken = 1

  /** Returns the registration [token] names, or null once that registration has gone. */
  fun find(token: Int): T? = registrations[token]

  /**
   * Installs the host trampoline if it is absent, and registers what [create] builds under a fresh
   * token.
   *
   * [create] runs before the installation, so a failure there leaves nothing registered and the
   * trampoline as it was.
   */
  fun add(create: (Int) -> T): T {
    val token = nextToken++
    val registration = create(token)
    install()
    registrations[token] = registration
    return registration
  }

  /**
   * Drops [token]'s registration, and clears the host pointer once the last one has gone.
   *
   * The host pointer is cleared before the table entry is released, because native reaches the
   * entry through that pointer.
   */
  fun remove(token: Int) {
    if (registrations.remove(token) == null) return
    if (registrations.isNotEmpty()) return
    installHost(0)
    removeTrampoline(trampoline)
    trampoline = 0
  }

  private fun install() {
    if (trampoline != 0) return
    BrowserModule.require()
    // Keeps the two trampolines in the linked binary. Their only other reference is the JavaScript
    // string that reads them out of `wasmExports`, and dead-code elimination cannot see inside a
    // string, so a binary that links this module as a klib -- every host consuming the published
    // artifact -- drops them. `addFunction` is then handed `undefined` and reports that a function
    // import requires a callable, which names neither the export nor the reason.
    retainTrampolines()
    val added = addTrampoline()
    if (added == 0) {
      throw Status.invalidState(
        "The MapLibre Native browser module could not place the $subject trampoline in its " +
          "function table."
      )
    }
    if (!installHost(added)) {
      removeTrampoline(added)
      throw Status.invalidState(
        "The MapLibre Native browser module could not install the $subject host callback."
      )
    }
    trampoline = added
  }
}

/**
 * One runtime's registration of a Kotlin resource provider.
 *
 * The callback runs on the page while the MapLibre thread that asked for the resource waits inside
 * the module's proxy, so it answers with a decision the way every other platform's provider does.
 * The body runs inside a [CallbackScope] so that it cannot wait for that thread in turn: a call
 * placed on the runtime's owner thread reports an error rather than parking this stack. A callback
 * that waited for a worker would close the wait graph and stop both threads for good.
 */
internal class ResourceProviderBridge
private constructor(private val callback: ResourceProviderCallback, private val token: Int) :
  AutoCloseable {
  private val gate = CallbackGate(SUBJECT)

  /** The `user_data` to register, which native carries back to [dispatch] with every request. */
  val userData: HeapPointer
    get() = HeapPointer(token)

  fun checkCanClose() {
    gate.checkCanClose()
  }

  /**
   * Stops delivering to this callback and releases the trampoline when it was the last one.
   *
   * Called after the registration it belongs to has been cleared with native, which is what makes
   * this safe: `mln_runtime_set_resource_provider` and `mln_runtime_clear_resource_provider` return
   * only once no in-flight request can still invoke the provider they replaced.
   */
  override fun close() {
    gate.close()
    host.remove(token)
  }

  private fun invoke(request: HeapPointer, rawHandle: Long): Int {
    if (request.address == 0 || rawHandle == 0L) return UNKNOWN_DECISION
    val lease = gate.enter() ?: return UNKNOWN_DECISION
    return try {
      CallbackScope.inside {
        val requestHandle = ResourceRequestHandle.fromNative(NativeResourceRequest(rawHandle))
        try {
          val decision = callback.handle(ResourceMarshal.readRequest(request), requestHandle)
          requestHandle.finishProviderDecision(decision)
        } catch (_: Throwable) {
          // A host failure leaves the request neither served nor passed through, so the handle is
          // given up and the C API fails the request. Reporting it here is the only place it can
          // be reported: there is no Kotlin frame above this one to unwind into.
          requestHandle.finishProviderException()
        }
      }
    } finally {
      lease.close()
    }
  }

  internal companion object {
    private const val SUBJECT = "resource provider callbacks"

    private val host =
      SyncCallbackHost<ResourceProviderBridge>(
        SUBJECT,
        ::addProviderTrampoline,
        ::installProviderHost,
      )

    /** Installs the host trampoline and registers [callback] with it. */
    fun install(callback: ResourceProviderCallback): ResourceProviderBridge = host.add { token ->
      ResourceProviderBridge(callback, token)
    }

    /** The callback to register in the descriptor, which is compiled into the browser module. */
    fun thunk(): Int = providerThunk()

    /**
     * Answers a request on behalf of the registration [token] names.
     *
     * A token with no registration is one the runtime has already released, so the request passes
     * through by way of the unknown decision rather than reaching a callback that has retired.
     */
    fun dispatch(token: Int, request: HeapPointer, rawHandle: Long): Int =
      host.find(token)?.invoke(request, rawHandle) ?: UNKNOWN_DECISION
  }
}

/**
 * One runtime's registration of a Kotlin resource URL transform.
 *
 * The transform runs on the page while a MapLibre thread waits, exactly as the provider does. The
 * replacement URL reaches that thread because `mln_resource_transform_response_set_url` copies it
 * into storage the response carries in its own `context` field, which every thread in the module
 * shares.
 */
internal class ResourceTransformBridge
private constructor(private val callback: ResourceTransformCallback, private val token: Int) :
  AutoCloseable {
  private val gate = CallbackGate(SUBJECT)

  val userData: HeapPointer
    get() = HeapPointer(token)

  fun checkCanClose() {
    gate.checkCanClose()
  }

  override fun close() {
    gate.close()
    host.remove(token)
  }

  private fun invoke(kind: Int, url: HeapPointer, outResponse: HeapPointer): Int {
    if (outResponse.address == 0) return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    val lease = gate.enter() ?: return MaplibreStatus.INVALID_ARGUMENT.nativeCode
    return try {
      CallbackScope.inside {
        // The leading size field carries the layout this binding was generated against, so native
        // knows which fields the answer below may be read from.
        MlnResourceTransformResponse.setSize(outResponse, MlnResourceTransformResponse.SIZEOF)
        MlnResourceTransformResponse.setUrl(outResponse, HeapPointer(0))
        val request = ResourceMarshal.readTransformRequest(kind, url)
        val replacement = callback.transform(request)
        if (replacement.isNullOrEmpty()) {
          MaplibreStatus.OK.nativeCode
        } else {
          setResponseUrl(outResponse, replacement)
        }
      }
    } catch (_: InvalidArgumentException) {
      MaplibreStatus.INVALID_ARGUMENT.nativeCode
    } catch (_: IllegalArgumentException) {
      MaplibreStatus.INVALID_ARGUMENT.nativeCode
    } catch (_: Throwable) {
      // MapLibre treats a non-OK transform as no rewrite, so a host failure keeps the URL the
      // request came with.
      MaplibreStatus.NATIVE_ERROR.nativeCode
    } finally {
      lease.close()
    }
  }

  private fun setResponseUrl(outResponse: HeapPointer, value: String): Int {
    Status.requireArgument('\u0000' !in value) { "replacement URL contains embedded NUL" }
    val bytes = Heap.utf8Size(value)
    return Heap.withScratch(bytes) { text ->
      Heap.storeUtf8(text, value)
      // Called directly rather than placed on the owner thread. The helper writes into storage the
      // response carries, has no owner thread of its own, and this stack may not park at all.
      NativeCall.call(
        "mln_resource_transform_response_set_url",
        SET_URL_SLOTS,
        { slots ->
          slots.setPointer(0, outResponse)
          slots.setPointer(1, text)
          // The C parameter counts the URL's bytes, and the scratch holds a terminator past them.
          slots.setInt(2, bytes - 1)
        },
        { Heap.loadInt(it) },
      )
    }
  }

  internal companion object {
    private const val SUBJECT = "resource transform callbacks"

    private val host =
      SyncCallbackHost<ResourceTransformBridge>(
        SUBJECT,
        ::addTransformTrampoline,
        ::installTransformHost,
      )

    fun install(callback: ResourceTransformCallback): ResourceTransformBridge = host.add { token ->
      ResourceTransformBridge(callback, token)
    }

    fun thunk(): Int = transformThunk()

    fun dispatch(token: Int, kind: Int, url: HeapPointer, outResponse: HeapPointer): Int =
      host.find(token)?.invoke(kind, url, outResponse) ?: MaplibreStatus.OK.nativeCode
  }
}
