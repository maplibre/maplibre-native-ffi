package org.maplibre.nativeffi.internal.callback

import org.maplibre.nativeffi.internal.lifecycle.NativeResourceRequest
import org.maplibre.nativeffi.internal.status.Status
import org.maplibre.nativeffi.internal.wasm.Heap
import org.maplibre.nativeffi.internal.wasm.HeapArena
import org.maplibre.nativeffi.internal.wasm.HeapPointer
import org.maplibre.nativeffi.internal.wasm.InjectedFaults
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterQueuedResourceProvider
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterQueuedResourceProviderRoute
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterQueuedResourceRequest
import org.maplibre.nativeffi.internal.wasm.generated.MlnAdapterResourceRouteFlags
import org.maplibre.nativeffi.internal.wasm.generated.MlnResourceProvider
import org.maplibre.nativeffi.internal.wasm.generated.mln_adapter_queued_resource_provider_retire
import org.maplibre.nativeffi.internal.wasm.generated.mln_adapter_resource_provider_request_destroy
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_queued_provider_callback
import org.maplibre.nativeffi.internal.wasm.generated.mln_kotlin_resource_request_listener
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_clear_resource_provider
import org.maplibre.nativeffi.internal.wasm.generated.mln_runtime_set_resource_provider
import org.maplibre.nativeffi.resource.QueuedResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceErrorReason
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceLoadingMethod
import org.maplibre.nativeffi.resource.ResourcePriority
import org.maplibre.nativeffi.resource.ResourceProviderRoute
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceRequestHandle
import org.maplibre.nativeffi.resource.ResourceResponse
import org.maplibre.nativeffi.resource.ResourceResponseStatus
import org.maplibre.nativeffi.resource.ResourceStoragePolicy
import org.maplibre.nativeffi.resource.ResourceUsage

/** The rule kind that matches every resource kind, `MLN_ADAPTER_RESOURCE_KIND_ANY`. */
internal const val RESOURCE_KIND_ANY: Int = -1

/**
 * Owns the queued resource provider registrations that the ring delivers to.
 *
 * MapLibre needs a pass-through decision on the thread that raised the request, and this binding
 * cannot answer there, so routes declared at registration claim requests and host code answers them
 * later. The retirement marker is what says the routes are read no more, and so when their heap can
 * go.
 *
 * The registrations are global rather than per runtime, because a queued record names no provider:
 * one ring cannot say which of two a request was claimed for. A second concurrent registration
 * reports invalid argument instead.
 */
internal object QueuedResourceProviders {
  private const val SUBJECT = "resource provider callbacks"
  private const val SET = "mln_runtime_set_resource_provider"
  private const val CLEAR = "mln_runtime_clear_resource_provider"

  private var current: Registration? = null

  /** The registrations awaiting their marker, oldest first; delivery goes to the oldest. */
  private val retiring = ArrayDeque<Registration>()

  /**
   * The registrations still holding a block of the module's heap, for the tests.
   *
   * A registration that native refused holds none, and one a marker has released holds none, so
   * this is what says a refusal left nothing behind.
   */
  val liveRegistrations: Int
    get() = (if (current == null) 0 else 1) + retiring.size

  /**
   * Registers or replaces [runtime]'s queued provider, keeping the previous one if native refuses.
   */
  fun set(
    runtime: Long,
    routes: List<ResourceProviderRoute>,
    callback: QueuedResourceProviderCallback,
  ) {
    val previous = current
    previous?.checkCanClose()
    Status.requireArgument(previous == null || previous.runtime == runtime) {
      "One queued resource provider can be registered at a time, because a queued request names " +
        "the routes that claimed it rather than the provider that declared them. Clear the other " +
        "runtime's provider first."
    }
    val replacement = Registration(runtime, routes, callback)
    try {
      InjectedFaults.beginCall(SET)
      Status.check(mln_runtime_set_resource_provider(runtime, replacement.descriptor.address))
    } catch (error: Throwable) {
      replacement.release()
      throw error
    }
    current = replacement
    previous?.retire()
  }

  /** Clears [runtime]'s queued provider, which native accepts whether or not one was set. */
  fun clear(runtime: Long) {
    current?.checkCanClose()
    InjectedFaults.beginCall(CLEAR)
    Status.check(mln_runtime_clear_resource_provider(runtime))
    val previous = current
    current = null
    previous?.retire()
  }

  /** Retires [runtime]'s provider after a runtime close, which dropped the registration itself. */
  fun retireFor(runtime: Long) {
    val previous = current ?: return
    if (previous.runtime != runtime) return
    current = null
    previous.retire()
  }

  /** Delivers one `mln_adapter_queued_resource_request` and releases it. */
  fun deliver(record: HeapPointer) {
    val handle =
      ResourceRequestHandle.forQueuedRequest(
        NativeResourceRequest(MlnAdapterQueuedResourceRequest.handle(record))
      )
    try {
      val target = retiring.firstOrNull() ?: current
      val request = readRequest(record)
      if (target == null || !target.deliver(request, handle)) {
        // Native is waiting for this request, and no host code will answer it, so it is failed
        // rather than left outstanding for the life of the page.
        fail(handle, "the resource provider that claimed this request has been retired")
      }
    } catch (_: Throwable) {
      fail(handle, "the resource request could not be copied for the provider callback")
    } finally {
      mln_adapter_resource_provider_request_destroy(record.address)
    }
  }

  /** Retires the oldest registration, which every request ahead of the marker was claimed for. */
  fun retired() {
    retiring.removeFirstOrNull()?.release()
  }

  /** Completes a request no host callback took, and closes the handle if that failed too. */
  private fun fail(handle: ResourceRequestHandle, diagnostic: String) {
    val response =
      ResourceResponse(ResourceResponseStatus.ERROR).apply {
        errorReason = ResourceErrorReason.OTHER
        errorMessage = diagnostic
      }
    try {
      handle.complete(response)
    } catch (_: Throwable) {
      handle.close()
    }
  }

  private fun readRequest(record: HeapPointer): ResourceRequest =
    ResourceRequest(
      requestedUrl = Heap.loadUtf8(MlnAdapterQueuedResourceRequest.requestedUrl(record)),
      resolvedUrl = Heap.loadUtf8(MlnAdapterQueuedResourceRequest.resolvedUrl(record)),
      kind = ResourceKind.fromNative(MlnAdapterQueuedResourceRequest.kind(record)),
      loadingMethod =
        ResourceLoadingMethod.fromNative(MlnAdapterQueuedResourceRequest.loadingMethod(record)),
      priority = ResourcePriority.fromNative(MlnAdapterQueuedResourceRequest.priority(record)),
      usage = ResourceUsage.fromNative(MlnAdapterQueuedResourceRequest.usage(record)),
      storagePolicy =
        ResourceStoragePolicy.fromNative(MlnAdapterQueuedResourceRequest.storagePolicy(record)),
      range =
        if (MlnAdapterQueuedResourceRequest.hasRange(record)) {
          ResourceRequest.ByteRange(
            MlnAdapterQueuedResourceRequest.rangeStart(record),
            MlnAdapterQueuedResourceRequest.rangeEnd(record),
          )
        } else {
          null
        },
      priorModifiedUnixMs =
        if (MlnAdapterQueuedResourceRequest.hasPriorModified(record)) {
          MlnAdapterQueuedResourceRequest.priorModifiedUnixMs(record)
        } else {
          null
        },
      priorExpiresUnixMs =
        if (MlnAdapterQueuedResourceRequest.hasPriorExpires(record)) {
          MlnAdapterQueuedResourceRequest.priorExpiresUnixMs(record)
        } else {
          null
        },
      // Null and empty mean different things here: no prior ETag at all, against one that is the
      // empty string. Reading the string would collapse them.
      priorEtag =
        MlnAdapterQueuedResourceRequest.priorEtag(record).let {
          if (it.address == 0) null else Heap.loadUtf8(it)
        },
      priorData =
        MlnAdapterQueuedResourceRequest.priorData(record).let {
          if (it.address == 0) {
            ByteArray(0)
          } else {
            Heap.loadBytes(it, MlnAdapterQueuedResourceRequest.priorDataSize(record))
          }
        },
    )

  /**
   * One host callback's registration, and the native descriptor it is registered through.
   *
   * The descriptor, its route table, and the route URLs share one heap block that native borrows
   * for the registration's whole life, and that the retirement marker releases.
   */
  private class Registration(
    val runtime: Long,
    routes: List<ResourceProviderRoute>,
    private val callback: QueuedResourceProviderCallback,
  ) {
    private val gate = CallbackGate(SUBJECT)
    private var retirementAsked = false

    /** The `mln_resource_provider` to register, in the block this registration owns. */
    val descriptor: HeapPointer

    private val block: HeapPointer
    private val provider: HeapPointer

    init {
      routes.forEach { Heap.requireCString(it.url, "route url") }
      var total = HeapArena.aligned(MlnResourceProvider.SIZEOF.toLong(), POINTER_ALIGN)
      total += HeapArena.aligned(MlnAdapterQueuedResourceProvider.SIZEOF.toLong(), POINTER_ALIGN)
      total +=
        HeapArena.aligned(
          Heap.sizeOf(MlnAdapterQueuedResourceProviderRoute.SIZEOF, routes.size).toLong(),
          POINTER_ALIGN,
        )
      routes.forEach { total += Heap.utf8Size(it.url).toLong() }
      Status.requireArgument(total <= Int.MAX_VALUE) { "the route table is too large to place" }

      block = Heap.acquire(total.toInt())
      try {
        val arena = HeapArena(block, total.toInt())
        descriptor = arena.allocate(MlnResourceProvider.SIZEOF, POINTER_ALIGN)
        provider = arena.allocate(MlnAdapterQueuedResourceProvider.SIZEOF, POINTER_ALIGN)
        val table =
          arena.allocate(
            Heap.sizeOf(MlnAdapterQueuedResourceProviderRoute.SIZEOF, routes.size),
            POINTER_ALIGN,
          )
        routes.forEachIndexed { index, route ->
          val entry = table + index * MlnAdapterQueuedResourceProviderRoute.SIZEOF
          val url = arena.allocate(Heap.utf8Size(route.url), BYTE_ALIGN)
          Heap.storeUtf8(url, route.url)
          MlnAdapterQueuedResourceProviderRoute.setKind(
            entry,
            route.kind?.nativeValue ?: RESOURCE_KIND_ANY,
          )
          MlnAdapterQueuedResourceProviderRoute.setFlags(entry, flagsOf(route))
          MlnAdapterQueuedResourceProviderRoute.setUrl(entry, url)
        }
        MlnAdapterQueuedResourceProvider.setRoutes(provider, table)
        MlnAdapterQueuedResourceProvider.setRouteCount(provider, routes.size)
        // The layout generator leaves a function-pointer field to its caller, so the table index
        // the
        // shim reports is written at the offset the generator declares for it.
        Heap.storeInt(
          provider + MlnAdapterQueuedResourceProvider.OFFSET_LISTENER,
          mln_kotlin_resource_request_listener(),
        )
        MlnResourceProvider.setSize(descriptor, MlnResourceProvider.SIZEOF)
        Heap.storeInt(
          descriptor + MlnResourceProvider.OFFSET_CALLBACK,
          mln_kotlin_queued_provider_callback(),
        )
        MlnResourceProvider.setUserData(descriptor, provider)
      } catch (error: Throwable) {
        Heap.release(block)
        throw error
      }
    }

    /** Runs [callback], or reports that this registration has stopped admitting requests. */
    fun deliver(request: ResourceRequest, handle: ResourceRequestHandle): Boolean {
      val lease = gate.enter() ?: return false
      try {
        callback.handle(request, handle)
      } catch (_: Throwable) {
        fail(handle, "the resource provider callback failed")
      } finally {
        lease.close()
      }
      return true
    }

    fun checkCanClose() = gate.checkCanClose()

    /** Asks native for the marker that says the routes are read no more. */
    fun retire() {
      if (retirementAsked) return
      retirementAsked = true
      retiring.addLast(this)
      mln_adapter_queued_resource_provider_retire(provider.address)
    }

    /** Stops delivering and releases the block the routes live in. */
    fun release() {
      try {
        gate.close()
      } finally {
        Heap.release(block)
      }
    }

    private companion object {
      const val POINTER_ALIGN = 4
      const val BYTE_ALIGN = 1
    }
  }

  private fun flagsOf(route: ResourceProviderRoute): Int {
    var flags = MlnAdapterResourceRouteFlags.MLN_ADAPTER_RESOURCE_ROUTE_FLAGS_NONE
    if (route.matchGlob)
      flags = flags or MlnAdapterResourceRouteFlags.MLN_ADAPTER_RESOURCE_ROUTE_MATCH_GLOB
    if (route.useRequestedUrl) {
      flags = flags or MlnAdapterResourceRouteFlags.MLN_ADAPTER_RESOURCE_ROUTE_USE_REQUESTED_URL
    }
    return flags
  }
}
