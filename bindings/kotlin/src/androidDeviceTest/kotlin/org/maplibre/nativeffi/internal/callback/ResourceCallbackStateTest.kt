package org.maplibre.nativeffi.internal.callback

import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import kotlin.concurrent.thread
import kotlin.test.Test
import kotlin.test.assertEquals
import kotlin.test.assertFalse
import kotlin.test.assertTrue
import org.maplibre.nativeffi.error.MaplibreStatus
import org.maplibre.nativeffi.internal.javacpp.JavaCppSupport
import org.maplibre.nativeffi.internal.javacpp.MaplibreNativeC
import org.maplibre.nativeffi.resource.HttpHeaderTransformCallback
import org.maplibre.nativeffi.resource.ResourceKind
import org.maplibre.nativeffi.resource.ResourceProviderCallback
import org.maplibre.nativeffi.resource.ResourceProviderDecision
import org.maplibre.nativeffi.resource.ResourceRequest
import org.maplibre.nativeffi.resource.ResourceTransformCallback

class ResourceCallbackStateTest {
  @Test
  fun providerCopiesResourceRequestAndUnknownRawEnums(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var copied: ResourceRequest? = null
      ResourceProviderState(
          ResourceProviderCallback { request, _ ->
            copied = request
            ResourceProviderDecision.PASS_THROUGH
          }
        )
        .use { state ->
          withRequest { request ->
            assertEquals(
              ResourceProviderDecision.PASS_THROUGH.nativeValue,
              state.invoke(request, 1),
            )
          }
        }
      assertEquals("maplibre://tiles/2/1/1.pbf", copied?.requestedUrl)
      assertEquals("https://example.com/tile.pbf", copied?.resolvedUrl)
      assertEquals(900, copied?.kind?.nativeValue)
      assertEquals(901, copied?.loadingMethod?.nativeValue)
    }

  @Test
  fun providerClosureDuringAndConcurrentWithCallbacksRejectsLaterEntry(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      lateinit var reentrant: ResourceProviderState
      reentrant =
        ResourceProviderState(
          ResourceProviderCallback { _, _ ->
            reentrant.close()
            ResourceProviderDecision.PASS_THROUGH
          }
        )
      withRequest { request ->
        assertEquals(
          ResourceProviderDecision.PASS_THROUGH.nativeValue,
          reentrant.invoke(request, 1),
        )
        assertTrue(reentrant.isClosedForTesting())
        assertEquals(-1, reentrant.invoke(request, 1))
      }

      val entered = CountDownLatch(1)
      val release = CountDownLatch(1)
      val state =
        ResourceProviderState(
          ResourceProviderCallback { _, _ ->
            entered.countDown()
            release.await(5, TimeUnit.SECONDS)
            ResourceProviderDecision.PASS_THROUGH
          }
        )
      withRequest { request ->
        val invocation = thread { state.invoke(request, 2) }
        assertTrue(entered.await(5, TimeUnit.SECONDS))
        val closed = CountDownLatch(1)
        val closer = thread {
          state.close()
          closed.countDown()
        }
        assertFalse(closed.await(50, TimeUnit.MILLISECONDS))
        release.countDown()
        invocation.join()
        closer.join()
        assertTrue(state.isClosedForTesting())
      }
    }

  @Test
  fun resourceTransformCopiesUnknownKindsContainsFailuresAndClosesDuringCallback(): Unit =
    org.maplibre.nativeffi.runtime.runSuspendTest {
      var copiedKind: ResourceKind? = null
      lateinit var state: ResourceTransformState
      state =
        ResourceTransformState(
          ResourceTransformCallback { request ->
            copiedKind = request.kind
            state.close()
            throw IllegalStateException("contained")
          }
        )
      JavaCppSupport.cString("https://example.com/style.json").use { url ->
        MaplibreNativeC.mln_resource_transform_response().use { response ->
          assertEquals(MaplibreStatus.NATIVE_ERROR.nativeCode, state.invoke(991, url, response))
          assertEquals(991, copiedKind?.nativeValue)
          assertTrue(state.isClosedForTesting())
          assertEquals(MaplibreStatus.INVALID_ARGUMENT.nativeCode, state.invoke(991, url, response))
        }
      }
    }

  @Test
  fun elevenLiveCallbackStatesKeepNonNullThunksAndDispatchByUserData() {
    val providerHits = IntArray(11)
    val transformHits = IntArray(11)
    val headerHits = IntArray(11)
    val providers =
      List(11) { index ->
        ResourceProviderState(
          ResourceProviderCallback { _, _ ->
            providerHits[index] += 1
            ResourceProviderDecision.PASS_THROUGH
          }
        )
      }
    val transforms =
      List(11) { index ->
        ResourceTransformState(
          ResourceTransformCallback { _ ->
            transformHits[index] += 1
            null
          }
        )
      }
    val headers =
      List(11) { index ->
        HttpHeaderTransformState(
          HttpHeaderTransformCallback { _ ->
            headerHits[index] += 1
            emptyList()
          }
        )
      }
    try {
      providers.forEach { assertFalse(it.descriptor().callback().isNull) }
      transforms.forEach { assertFalse(it.descriptor().callback().isNull) }
      headers.forEach { assertFalse(it.descriptor().callback().isNull) }

      withRequest { request ->
        val first = providers.first()
        val last = providers.last()
        assertEquals(
          ResourceProviderDecision.PASS_THROUGH.nativeValue,
          first.descriptor().callback().call(first.descriptor().user_data(), request, 1),
        )
        assertEquals(
          ResourceProviderDecision.PASS_THROUGH.nativeValue,
          last.descriptor().callback().call(last.descriptor().user_data(), request, 2),
        )
      }
      JavaCppSupport.cString("https://example.com/style.json").use { url ->
        MaplibreNativeC.mln_resource_transform_response().use { response ->
          val first = transforms.first()
          val last = transforms.last()
          assertEquals(
            MaplibreStatus.OK.nativeCode,
            first.descriptor().callback().call(first.descriptor().user_data(), 1, url, response),
          )
          assertEquals(
            MaplibreStatus.OK.nativeCode,
            last.descriptor().callback().call(last.descriptor().user_data(), 1, url, response),
          )
        }
        MaplibreNativeC.mln_http_header_transform_response().use { response ->
          val first = headers.first()
          val last = headers.last()
          assertEquals(
            MaplibreStatus.OK.nativeCode,
            first.descriptor().callback().call(first.descriptor().user_data(), 1, url, response),
          )
          assertEquals(
            MaplibreStatus.OK.nativeCode,
            last.descriptor().callback().call(last.descriptor().user_data(), 1, url, response),
          )
        }
      }
      assertEquals(1, providerHits.first())
      assertEquals(1, providerHits.last())
      assertEquals(0, providerHits.slice(1..9).sum())
      assertEquals(1, transformHits.first())
      assertEquals(1, transformHits.last())
      assertEquals(0, transformHits.slice(1..9).sum())
      assertEquals(1, headerHits.first())
      assertEquals(1, headerHits.last())
      assertEquals(0, headerHits.slice(1..9).sum())
    } finally {
      providers.forEach { it.close() }
      transforms.forEach { it.close() }
      headers.forEach { it.close() }
    }
  }

  private inline fun withRequest(block: (MaplibreNativeC.mln_resource_request) -> Unit) {
    JavaCppSupport.cString("maplibre://tiles/2/1/1.pbf").use { requested ->
      JavaCppSupport.cString("https://example.com/tile.pbf").use { resolved ->
        MaplibreNativeC.mln_resource_request().use { request ->
          request.size(request.sizeof())
          request.requested_url(requested)
          request.resolved_url(resolved)
          request.kind(900)
          request.loading_method(901)
          block(request)
        }
      }
    }
  }
}
