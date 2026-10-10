package org.maplibre.nativeffi

import kotlin.time.Duration
import kotlin.time.Duration.Companion.seconds
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Deferred
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.TimeoutCancellationException
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.withContext
import kotlinx.coroutines.withTimeout
import org.maplibre.nativeffi.generated.GeneratedApi
import org.maplibre.nativeffi.generated.MapHandle
import org.maplibre.nativeffi.generated.MapMode
import org.maplibre.nativeffi.generated.MapOptions
import org.maplibre.nativeffi.generated.ResourceErrorReason
import org.maplibre.nativeffi.generated.ResourceProvider
import org.maplibre.nativeffi.generated.ResourceProviderDecision
import org.maplibre.nativeffi.generated.ResourceRequest
import org.maplibre.nativeffi.generated.ResourceRequestHandle
import org.maplibre.nativeffi.generated.ResourceResponse
import org.maplibre.nativeffi.generated.ResourceResponseStatus
import org.maplibre.nativeffi.generated.RuntimeEvent
import org.maplibre.nativeffi.generated.RuntimeEventSourceType
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.RuntimeHandle
import org.maplibre.nativeffi.generated.RuntimeOptions
import org.maplibre.nativeffi.generated.Wake
import org.maplibre.nativeffi.runtime.AsyncReleasable

/** How long one test may run before it fails instead of hanging the suite. */
internal val TEST_TIMEOUT: Duration = 60.seconds

/** How long any single wait in a test lasts before it fails. */
internal val WAIT_TIMEOUT: Duration = 10.seconds

/** Smallest style a map can load: valid, empty, and free of resource requests. */
internal const val EMPTY_STYLE_JSON: String = """{"version":8,"sources":{},"layers":[]}"""

/** Runs a suspending test body on the calling thread, failing it after [TEST_TIMEOUT]. */
internal fun <T> runSuspendTest(block: suspend CoroutineScope.() -> T): T = runBlocking {
  withTimeout(TEST_TIMEOUT) { block() }
}

/** Awaits this deferred, failing the test when it does not complete within [WAIT_TIMEOUT]. */
internal suspend fun <T> Deferred<T>.awaitWithin(what: String): T =
  try {
    withTimeout(WAIT_TIMEOUT) { await() }
  } catch (timeout: TimeoutCancellationException) {
    throw AssertionError("timed out waiting for $what", timeout)
  }

/**
 * Answers every request that [serve] leaves to it with a NOT_FOUND error, so no test reaches the
 * network or the file system. [serve] returns a decision for the requests it handles itself, and
 * null for the rest.
 */
internal fun denyingProvider(
  serve: (ResourceRequest, ResourceRequestHandle) -> ResourceProviderDecision? = { _, _ -> null }
): ResourceProvider = ResourceProvider { request, handle ->
  serve(request, handle) ?: handle.deny()
}

/** Completes this request with the fixture's NOT_FOUND error and releases it. */
internal fun ResourceRequestHandle.deny(): ResourceProviderDecision {
  complete(
    ResourceResponse(
      status = ResourceResponseStatus.ERROR,
      errorReason = ResourceErrorReason.NOT_FOUND,
      errorMessage = "the test fixture serves no resources",
    )
  )
  close()
  return ResourceProviderDecision.HANDLE
}

/** Completes this request with [body] and releases it. */
internal fun ResourceRequestHandle.serve(body: String): ResourceProviderDecision {
  complete(ResourceResponse(status = ResourceResponseStatus.OK, bytes = body.encodeToByteArray()))
  close()
  return ResourceProviderDecision.HANDLE
}

/** Options for the fixture's small map, [width] x [height] logical pixels. */
internal fun smallMapOptions(
  mapMode: MapMode = MapMode.CONTINUOUS,
  width: Int = 64,
  height: Int = 64,
): MapOptions =
  GeneratedApi.mapOptionsDefault().let { defaults ->
    defaults.copy(
      initialExtent = defaults.initialExtent.copy(width = width.toUInt(), height = height.toUInt()),
      mapMode = mapMode,
    )
  }

/**
 * A runtime whose event queue wakes the test, with a resource provider installed before anything
 * can request a resource.
 */
internal class RuntimeFixture
private constructor(val runtime: RuntimeHandle, private val wakes: Channel<Unit>) {
  /**
   * Drains runtime events until one matches [predicate], blocking on the event wake between drains.
   * Events drained before the match are dropped.
   */
  suspend fun awaitEvent(what: String, predicate: (RuntimeEvent) -> Boolean): RuntimeEvent =
    try {
      withTimeout(WAIT_TIMEOUT) {
        var match: RuntimeEvent? = null
        while (match == null) {
          match = runtime.drainEvents().use { it.get().events }.firstOrNull(predicate)
          // The wake fires when the queue becomes nonempty, so an event queued after this drain
          // leaves a wake behind for the receive.
          if (match == null) wakes.receive()
        }
        match
      }
    } catch (timeout: TimeoutCancellationException) {
      throw AssertionError("timed out waiting for $what", timeout)
    }

  /** Waits for the next map event of [type]. */
  suspend fun awaitMapEvent(type: RuntimeEventType): RuntimeEvent =
    awaitEvent("map event $type") { it.type == type && it.sourceType == RuntimeEventSourceType.MAP }

  companion object {
    /**
     * Runs [block] against a fresh runtime created from [options] with [provider] installed, and
     * releases the runtime afterwards even when [block] fails. The fixture owns the event wake.
     */
    suspend fun <T> use(
      provider: ResourceProvider = denyingProvider(),
      options: RuntimeOptions = GeneratedApi.runtimeOptionsDefault(),
      block: suspend RuntimeFixture.() -> T,
    ): T {
      val wakes = Channel<Unit>(Channel.CONFLATED)
      val runtime =
        GeneratedApi.runtimeCreate(options.copy(eventWake = Wake { wakes.trySend(Unit) }))
      val fixture = RuntimeFixture(runtime, wakes)
      return runThenRelease(listOf(runtime)) {
        runtime.setResourceProvider(provider).awaitWithin("the resource provider")
        fixture.block()
      }
    }
  }
}

/** A small map on a [RuntimeFixture]. */
internal class MapFixture(val runtimeFixture: RuntimeFixture, val map: MapHandle) {
  val runtime: RuntimeHandle
    get() = runtimeFixture.runtime

  suspend fun awaitMapEvent(type: RuntimeEventType): RuntimeEvent =
    runtimeFixture.awaitMapEvent(type)

  /** Loads [json] and waits for the map to report the loaded style. */
  suspend fun loadStyle(json: String = EMPTY_STYLE_JSON) {
    map.setStyleJson(json.encodeToByteArray()).awaitWithin("the style command")
    awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
  }
}

/**
 * Runs [block] against a fresh runtime created from [runtimeOptions] and a small map, with
 * [provider] answering every resource request, and releases both afterwards.
 */
internal suspend fun <T> withMap(
  mapMode: MapMode = MapMode.CONTINUOUS,
  provider: ResourceProvider = denyingProvider(),
  options: MapOptions = smallMapOptions(mapMode),
  runtimeOptions: RuntimeOptions = GeneratedApi.runtimeOptionsDefault(),
  block: suspend MapFixture.() -> T,
): T =
  RuntimeFixture.use(provider, runtimeOptions) {
    val map = runtime.mapCreate(options).awaitWithin("the map")
    runThenRelease(listOf(map)) { MapFixture(this, map).block() }
  }

/**
 * Runs [block], then releases [owners] in order. Every release runs, and the first failure wins, so
 * a teardown failure never hides the failure that caused it.
 */
internal suspend fun <T> runThenRelease(owners: List<AsyncReleasable>, block: suspend () -> T): T {
  var failure: Throwable? = null
  val result =
    try {
      block()
    } catch (error: Throwable) {
      failure = error
      null
    }
  withContext(NonCancellable) {
    for (owner in owners) {
      try {
        owner.release().awaitWithin("the release of $owner")
      } catch (error: Throwable) {
        if (failure == null) failure = error
      }
    }
  }
  failure?.let { throw it }
  @Suppress("UNCHECKED_CAST")
  return result as T
}
