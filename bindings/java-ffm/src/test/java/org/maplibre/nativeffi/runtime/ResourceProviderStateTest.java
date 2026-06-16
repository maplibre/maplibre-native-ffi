package org.maplibre.nativeffi.runtime;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.lang.foreign.Arena;
import java.lang.foreign.MemorySegment;
import java.lang.foreign.ValueLayout;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativeffi.internal.c.MapLibreNativeC;
import org.maplibre.nativeffi.internal.c.mln_resource_provider;
import org.maplibre.nativeffi.internal.c.mln_resource_provider_callback;
import org.maplibre.nativeffi.internal.c.mln_resource_request;
import org.maplibre.nativeffi.internal.memory.MemoryUtil;
import org.maplibre.nativeffi.resource.ResourceKind;
import org.maplibre.nativeffi.resource.ResourceLoadingMethod;
import org.maplibre.nativeffi.resource.ResourcePriority;
import org.maplibre.nativeffi.resource.ResourceProviderDecision;
import org.maplibre.nativeffi.resource.ResourceRequest;
import org.maplibre.nativeffi.resource.ResourceStoragePolicy;
import org.maplibre.nativeffi.resource.ResourceUsage;
import org.maplibre.nativeffi.test.NativeTestSupport;

final class ResourceProviderStateTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibrary();
  }

  @Test
  void copiesProviderRequestBeforeInvokingCallback() {
    var seen = new AtomicReference<ResourceRequest>();
    int decision;
    try (var state =
            new ResourceProviderState(
                (request, handle) -> {
                  seen.set(request);
                  return ResourceProviderDecision.PASS_THROUGH;
                });
        var arena = Arena.ofConfined()) {
      decision = invoke(state, request(arena));
    }

    assertEquals(MapLibreNativeC.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH(), decision);
    assertEquals("https://example.test/tile.pbf", seen.get().url());
    assertEquals(ResourceKind.TILE, seen.get().kind());
    assertEquals(MapLibreNativeC.MLN_RESOURCE_KIND_TILE(), seen.get().rawKind());
    assertEquals(ResourceLoadingMethod.NETWORK_ONLY, seen.get().loadingMethod());
    assertEquals(
        MapLibreNativeC.MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY(), seen.get().rawLoadingMethod());
    assertEquals(ResourcePriority.LOW, seen.get().priority());
    assertEquals(MapLibreNativeC.MLN_RESOURCE_PRIORITY_LOW(), seen.get().rawPriority());
    assertEquals(ResourceUsage.OFFLINE, seen.get().usage());
    assertEquals(MapLibreNativeC.MLN_RESOURCE_USAGE_OFFLINE(), seen.get().rawUsage());
    assertEquals(ResourceStoragePolicy.VOLATILE, seen.get().storagePolicy());
    assertEquals(
        MapLibreNativeC.MLN_RESOURCE_STORAGE_POLICY_VOLATILE(), seen.get().rawStoragePolicy());
    assertEquals(new ResourceRequest.ByteRange(7, 11), seen.get().range().orElseThrow());
    assertEquals(123L, seen.get().priorModifiedUnixMs().orElseThrow());
    assertEquals(456L, seen.get().priorExpiresUnixMs().orElseThrow());
    assertEquals("etag", seen.get().priorEtag().orElseThrow());
    var priorData = seen.get().priorData();
    assertArrayEquals(new byte[] {1, 2, 3}, priorData);
    priorData[0] = 9;
    assertArrayEquals(new byte[] {1, 2, 3}, seen.get().priorData());
  }

  @Test
  void providerExceptionsProduceProviderErrorDecision() {
    try (var state =
            new ResourceProviderState(
                (request, handle) -> {
                  throw new AssertionError("boom");
                });
        var arena = Arena.ofConfined()) {
      assertEquals(ResourceProviderState.UNKNOWN_DECISION, invoke(state, request(arena)));
    }
  }

  @Test
  void nullProviderDecisionProducesProviderErrorDecision() {
    try (var state = new ResourceProviderState((request, handle) -> null);
        var arena = Arena.ofConfined()) {
      assertEquals(ResourceProviderState.UNKNOWN_DECISION, invoke(state, request(arena)));
    }
  }

  @Test
  void closedPassThroughHandleDefersReleaseToNativeDecision() {
    try (var state =
            new ResourceProviderState(
                (request, handle) -> {
                  handle.close();
                  return ResourceProviderDecision.PASS_THROUGH;
                });
        var arena = Arena.ofConfined()) {
      assertEquals(
          MapLibreNativeC.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH(),
          invoke(state, request(arena)));
    }
  }

  @Test
  void closeWaitsForActiveProviderUpcalls() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var state =
        new ResourceProviderState(
            (request, handle) -> {
              entered.countDown();
              await(release);
              return ResourceProviderDecision.PASS_THROUGH;
            });
    var executor = Executors.newFixedThreadPool(2);
    try (var arena = Arena.ofShared()) {
      var invoke = executor.submit(() -> invoke(state, request(arena)));
      assertTrue(entered.await(5, TimeUnit.SECONDS));

      var close = executor.submit(state::close);
      assertThrows(TimeoutException.class, () -> close.get(100, TimeUnit.MILLISECONDS));

      release.countDown();
      assertEquals(
          MapLibreNativeC.MLN_RESOURCE_PROVIDER_DECISION_PASS_THROUGH(),
          invoke.get(5, TimeUnit.SECONDS));
      close.get(5, TimeUnit.SECONDS);
    } finally {
      release.countDown();
      executor.shutdownNow();
    }
  }

  private static int invoke(ResourceProviderState state, MemorySegment request) {
    return mln_resource_provider_callback.invoke(
        mln_resource_provider.callback(state.descriptor()),
        MemorySegment.NULL,
        request,
        MemorySegment.ofAddress(0x1234));
  }

  private static MemorySegment request(Arena arena) {
    var request = mln_resource_request.allocate(arena);
    mln_resource_request.size(request, (int) mln_resource_request.sizeof());
    mln_resource_request.url(
        request, MemoryUtil.allocateCString(arena, "https://example.test/tile.pbf"));
    mln_resource_request.kind(request, MapLibreNativeC.MLN_RESOURCE_KIND_TILE());
    mln_resource_request.loading_method(
        request, MapLibreNativeC.MLN_RESOURCE_LOADING_METHOD_NETWORK_ONLY());
    mln_resource_request.priority(request, MapLibreNativeC.MLN_RESOURCE_PRIORITY_LOW());
    mln_resource_request.usage(request, MapLibreNativeC.MLN_RESOURCE_USAGE_OFFLINE());
    mln_resource_request.storage_policy(
        request, MapLibreNativeC.MLN_RESOURCE_STORAGE_POLICY_VOLATILE());
    mln_resource_request.has_range(request, true);
    mln_resource_request.range_start(request, 7);
    mln_resource_request.range_end(request, 11);
    mln_resource_request.has_prior_modified(request, true);
    mln_resource_request.prior_modified_unix_ms(request, 123);
    mln_resource_request.has_prior_expires(request, true);
    mln_resource_request.prior_expires_unix_ms(request, 456);
    mln_resource_request.prior_etag(request, MemoryUtil.allocateCString(arena, "etag"));
    var priorData = arena.allocate(3);
    priorData.setAtIndex(ValueLayout.JAVA_BYTE, 0, (byte) 1);
    priorData.setAtIndex(ValueLayout.JAVA_BYTE, 1, (byte) 2);
    priorData.setAtIndex(ValueLayout.JAVA_BYTE, 2, (byte) 3);
    mln_resource_request.prior_data(request, priorData);
    mln_resource_request.prior_data_size(request, 3);
    return request;
  }

  private static void await(CountDownLatch latch) {
    try {
      if (!latch.await(5, TimeUnit.SECONDS)) {
        throw new AssertionError("timed out waiting for latch");
      }
    } catch (InterruptedException error) {
      Thread.currentThread().interrupt();
      throw new AssertionError(error);
    }
  }
}
