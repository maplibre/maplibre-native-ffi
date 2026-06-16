package org.maplibre.nativejni.runtime;

import static org.junit.jupiter.api.Assertions.assertArrayEquals;
import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.io.BufferedReader;
import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.nio.charset.StandardCharsets;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.error.InvalidArgumentException;
import org.maplibre.nativejni.error.InvalidStateException;
import org.maplibre.nativejni.error.MaplibreStatus;
import org.maplibre.nativejni.error.WrongThreadException;
import org.maplibre.nativejni.geo.Geometry;
import org.maplibre.nativejni.geo.LatLng;
import org.maplibre.nativejni.geo.LatLngBounds;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.offline.OfflineRegionDefinition;
import org.maplibre.nativejni.offline.OfflineRegionDownloadState;
import org.maplibre.nativejni.resource.ResourceErrorReason;
import org.maplibre.nativejni.resource.ResourceKind;
import org.maplibre.nativejni.resource.ResourceProviderDecision;
import org.maplibre.nativejni.resource.ResourceRequestHandle;
import org.maplibre.nativejni.resource.ResourceResponse;

class RuntimeHandleTest {
  private static final String STYLE_JSON = "{\"version\":8,\"sources\":{},\"layers\":[]}";

  @AfterEach
  void resetTestInjections() {
    RuntimeHandle.resetInstallFailuresForTesting();
  }

  @Test
  void bnd023AndBnd040AndBnd080CreateRunOnceAndCloseRuntime() {
    var runtime = RuntimeHandle.create();
    assertFalse(runtime.isClosed());
    runtime.runOnce();
    runtime.close();
    assertTrue(runtime.isClosed());
    runtime.close();
    assertTrue(runtime.isClosed());
    assertThrows(InvalidStateException.class, runtime::runOnce);
  }

  @Test
  void bnd060CreateRuntimeUsesSuppliedOptions() {
    try (var runtime = RuntimeHandle.create(new RuntimeOptions().maximumCacheSize(42))) {
      runtime.runOnce();
    }
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().assetPath("asset\0path")));
    assertThrows(
        InvalidArgumentException.class,
        () -> RuntimeHandle.create(new RuntimeOptions().maximumCacheSize(-1)));
  }

  @Test
  void bnd084StartsAndDiscardsAmbientCacheOperation() {
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);
      assertFalse(operation.isClosed());
      assertTrue(operation.id() != 0);
      assertTrue(operation.kind() == OfflineOperationKind.AMBIENT_CACHE);
      assertTrue(operation.resultKind() == OfflineOperationResultKind.NONE);
      operation.close();
      assertTrue(operation.isClosed());
      operation.close();
    }
  }

  @Test
  void bnd044OfflineOperationLeakReportDoesNotDiscardNativeOperation() {
    try (var runtime = RuntimeHandle.create()) {
      var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);

      var leaked = captureStderr(operation::reportLeakForTesting);
      assertTrue(leaked.contains("Leaked OfflineOperationHandle"));
      assertTrue(leaked.contains("kind=AMBIENT_CACHE"));
      assertFalse(operation.isClosed());

      operation.close();
      var closed = captureStderr(operation::reportLeakForTesting);
      assertTrue(closed.isEmpty());
    }
  }

  @Test
  void bnd042RuntimeCloseFailsWhileOfflineOperationIsLive() {
    var runtime = RuntimeHandle.create();
    var operation = runtime.startAmbientCacheOperation(AmbientCacheOperation.CLEAR);
    assertThrows(InvalidStateException.class, runtime::close);
    operation.close();
    runtime.close();
    operation.close();
  }

  @Test
  void bnd042RuntimeCloseFailsWhileMapIsLive() {
    var runtime = RuntimeHandle.create();
    var map = MapHandle.create(runtime, new MapOptions().size(64, 64));

    var error = assertThrows(InvalidStateException.class, runtime::close);
    assertTrue(error.diagnostic().contains("live child handle"));
    assertFalse(runtime.isClosed());

    map.close();
    runtime.close();
    assertTrue(runtime.isClosed());
  }

  @Test
  void bnd084StartsAndDiscardsCreateOfflineRegionOperation() {
    try (var runtime = RuntimeHandle.create()) {
      var operation =
          runtime.startCreateOfflineRegion(
              new OfflineRegionDefinition.TilePyramid(
                  "https://example.com/style.json",
                  new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                  0.0,
                  1.0,
                  1.0f,
                  true),
              new byte[] {1, 2, 3});
      assertTrue(operation.kind() == OfflineOperationKind.REGION_CREATE);
      assertTrue(operation.resultKind() == OfflineOperationResultKind.REGION);
      try {
        runtime.takeCreateOfflineRegionResult(operation);
      } catch (InvalidStateException expectedIfStillRunning) {
        operation.close();
      }
      var geometryOperation =
          runtime.startCreateOfflineRegion(
              new OfflineRegionDefinition.GeometryRegion(
                  "https://example.com/style.json",
                  Geometry.point(new LatLng(0.5, 0.5)),
                  0.0,
                  1.0,
                  1.0f,
                  true),
              new byte[] {1, 2, 3});
      assertTrue(geometryOperation.kind() == OfflineOperationKind.REGION_CREATE);
      assertTrue(geometryOperation.resultKind() == OfflineOperationResultKind.REGION);
      geometryOperation.close();
    }
  }

  @Test
  void bnd024CreateOfflineRegionRejectsEmbeddedNulStyleUrlBeforeNativeCall() {
    try (var runtime = RuntimeHandle.create()) {
      var error =
          assertThrows(
              InvalidArgumentException.class,
              () ->
                  runtime.startCreateOfflineRegion(
                      new OfflineRegionDefinition.TilePyramid(
                          "https://example.com/style\0json",
                          new LatLngBounds(new LatLng(0, 0), new LatLng(1, 1)),
                          0.0,
                          1.0,
                          1.0f,
                          true),
                      new byte[] {1, 2, 3}));

      assertTrue(error.diagnostic().contains("offline region style URL"));
    }
  }

  @Test
  void bnd084StartsAndDiscardsOfflineRegionControlOperations() {
    try (var runtime = RuntimeHandle.create()) {
      var get = runtime.startOfflineRegion(123);
      assertTrue(get.kind() == OfflineOperationKind.REGION_GET);
      assertTrue(get.resultKind() == OfflineOperationResultKind.OPTIONAL_REGION);
      get.close();
      var list = runtime.startOfflineRegions();
      assertTrue(list.kind() == OfflineOperationKind.REGIONS_LIST);
      assertTrue(list.resultKind() == OfflineOperationResultKind.REGION_LIST);
      list.close();
      var update = runtime.startUpdateOfflineRegionMetadata(123, new byte[] {4, 5, 6});
      assertTrue(update.kind() == OfflineOperationKind.REGION_UPDATE_METADATA);
      assertTrue(update.resultKind() == OfflineOperationResultKind.REGION);
      assertThrows(
          InvalidStateException.class, () -> runtime.takeUpdateOfflineRegionMetadataResult(update));
      update.close();
      var status = runtime.startOfflineRegionStatus(123);
      assertTrue(status.kind() == OfflineOperationKind.REGION_GET_STATUS);
      assertTrue(status.resultKind() == OfflineOperationResultKind.REGION_STATUS);
      assertThrows(
          InvalidStateException.class, () -> runtime.takeOfflineRegionStatusResult(status));
      status.close();
      runtime.startSetOfflineRegionObserved(123, false).close();
      runtime.startSetOfflineRegionDownloadState(123, OfflineRegionDownloadState.INACTIVE).close();
      runtime.startInvalidateOfflineRegion(123).close();
      runtime.startDeleteOfflineRegion(123).close();
    }
  }

  @Test
  void bnd068UnknownOfflineRegionDownloadStateIsRejectedAsInput() {
    try (var runtime = RuntimeHandle.create()) {
      assertThrows(
          IllegalArgumentException.class,
          () ->
              runtime.startSetOfflineRegionDownloadState(
                  123, OfflineRegionDownloadState.fromNative(999)));
    }
  }

  @Test
  void bnd142ResourceProviderLifecycleCrossesNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
    }
  }

  @Test
  void bnd081AndBnd143ResourceProviderInlineCompletionLoadsStyle() throws Exception {
    var originalNetworkStatus = Maplibre.networkStatus();
    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    try {
      var providerCalls = new AtomicInteger();
      try (var runtime = RuntimeHandle.create()) {
        runtime.setResourceProvider(
            (request, handle) -> {
              providerCalls.incrementAndGet();
              assertEquals(ResourceKind.STYLE, request.kind());
              handle.complete(ResourceResponse.ok(STYLE_JSON.getBytes(StandardCharsets.UTF_8)));
              return ResourceProviderDecision.PASS_THROUGH;
            });
        try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
          map.setStyleUrl("http://provider.test/style.json");
          waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED);
        }
      }

      assertEquals(1, providerCalls.get());
    } finally {
      restoreNetworkStatus(originalNetworkStatus);
    }
  }

  @Test
  void bnd081AndBnd144AndBnd145ResourceProviderLateCompletionFromAnotherThreadLoadsStyle()
      throws Exception {
    var originalNetworkStatus = Maplibre.networkStatus();
    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    try {
      var requestReady = new CountDownLatch(1);
      var requestHandle = new AtomicReference<ResourceRequestHandle>();
      var completionFailure = new AtomicReference<Throwable>();
      try (var runtime = RuntimeHandle.create()) {
        runtime.setResourceProvider(
            (request, handle) -> {
              assertEquals(ResourceKind.STYLE, request.kind());
              requestHandle.set(handle);
              requestReady.countDown();
              return ResourceProviderDecision.HANDLE;
            });
        try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
          map.setStyleUrl("http://provider.test/style.json");
          assertTrue(requestReady.await(5, TimeUnit.SECONDS));

          var completionThread =
              new Thread(
                  () -> {
                    try {
                      requestHandle
                          .get()
                          .complete(
                              ResourceResponse.ok(STYLE_JSON.getBytes(StandardCharsets.UTF_8)));
                    } catch (Throwable throwable) {
                      completionFailure.set(throwable);
                    }
                  },
                  "resource-provider-completion");
          completionThread.start();
          completionThread.join(5_000);

          assertFalse(completionThread.isAlive());
          if (completionFailure.get() != null) {
            throw new AssertionError("resource completion failed", completionFailure.get());
          }
          waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED);
        }
      }
    } finally {
      restoreNetworkStatus(originalNetworkStatus);
    }
  }

  @Test
  void bnd148AndBnd151ResourceProviderCancellationClosesStaleRequestHandle() throws Exception {
    var originalNetworkStatus = Maplibre.networkStatus();
    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    try {
      var requestReady = new CountDownLatch(1);
      var requestHandle = new AtomicReference<ResourceRequestHandle>();
      try (var runtime = RuntimeHandle.create()) {
        runtime.setResourceProvider(
            (request, handle) -> {
              assertEquals(ResourceKind.STYLE, request.kind());
              requestHandle.set(handle);
              requestReady.countDown();
              return ResourceProviderDecision.HANDLE;
            });
        var map = MapHandle.create(runtime, new MapOptions().size(64, 64));
        try {
          map.setStyleUrl("http://provider.test/cancelled-style.json");
          assertTrue(requestReady.await(5, TimeUnit.SECONDS));

          map.close();
          assertTrue(awaitCancelled(requestHandle.get()));

          var error =
              assertThrows(
                  InvalidStateException.class,
                  () ->
                      requestHandle
                          .get()
                          .complete(
                              ResourceResponse.ok(STYLE_JSON.getBytes(StandardCharsets.UTF_8))));
          assertEquals(MaplibreStatus.INVALID_STATE, error.status());
          assertThrows(
              InvalidStateException.class,
              () ->
                  requestHandle
                      .get()
                      .complete(ResourceResponse.ok(STYLE_JSON.getBytes(StandardCharsets.UTF_8))));
          assertThrows(InvalidStateException.class, () -> requestHandle.get().isCancelled());
          requestHandle.get().close();
        } finally {
          map.close();
        }
      }
    } finally {
      restoreNetworkStatus(originalNetworkStatus);
    }
  }

  @Test
  void bnd149ResourceProviderErrorResponseBecomesLoadingFailureEvent() throws Exception {
    var originalNetworkStatus = Maplibre.networkStatus();
    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    try {
      try (var runtime = RuntimeHandle.create()) {
        runtime.setResourceProvider(
            (request, handle) -> {
              handle.complete(
                  ResourceResponse.error(ResourceErrorReason.NOT_FOUND, "provider missing style"));
              return ResourceProviderDecision.HANDLE;
            });
        try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
          map.setStyleUrl("http://provider.test/missing-style.json");
          var event = awaitMapEvent(runtime, map, RuntimeEventType.MAP_LOADING_FAILED);

          assertEquals(RuntimeEventType.MAP_LOADING_FAILED, event.type());
          assertTrue(event.mapSource().filter(source -> source == map).isPresent());
          assertFalse(event.message().isBlank());
        }
      }
    } finally {
      restoreNetworkStatus(originalNetworkStatus);
    }
  }

  @Test
  void bnd140ResourceTransformLifecycleCrossesNativeBoundary() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceTransform(request -> Optional.empty());
      runtime.setResourceTransform(request -> Optional.of(request.url() + "?rewritten=true"));
      runtime.clearResourceTransform();
      runtime.clearResourceTransform();
    }
  }

  @Test
  void bnd122ResourceTransformReplacementFailurePreservesCurrentCallbackAndClosesReplacement() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceTransform(request -> Optional.empty());
      var original = runtime.resourceTransformForTesting();
      var failure = new IllegalStateException("injected resource transform install failure");

      RuntimeHandle.failNextResourceTransformInstallForTesting(failure);
      assertSame(
          failure,
          assertThrows(
              IllegalStateException.class,
              () -> runtime.setResourceTransform(request -> Optional.of(request.url()))));

      assertSame(original, runtime.resourceTransformForTesting());
      assertFalse(original.isClosed());
      assertTrue(runtime.failedResourceTransformForTesting().isClosed());
    }
  }

  @Test
  void bnd122ResourceProviderReplacementFailurePreservesCurrentCallbackAndClosesReplacement() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.setResourceProvider((request, handle) -> ResourceProviderDecision.PASS_THROUGH);
      var original = runtime.resourceProviderForTesting();
      var failure = new IllegalStateException("injected resource provider install failure");

      RuntimeHandle.failNextResourceProviderInstallForTesting(failure);
      assertSame(
          failure,
          assertThrows(
              IllegalStateException.class,
              () ->
                  runtime.setResourceProvider(
                      (request, handle) -> ResourceProviderDecision.PASS_THROUGH)));

      assertSame(original, runtime.resourceProviderForTesting());
      assertFalse(original.isClosed());
      assertTrue(runtime.failedResourceProviderForTesting().isClosed());
    }
  }

  @Test
  void bnd140AndBnd141ResourceTransformRewritesStyleUrlThroughPublicLoad() throws Exception {
    var originalNetworkStatus = Maplibre.networkStatus();
    Maplibre.setNetworkStatus(NetworkStatus.ONLINE);
    try (var server = new ServerSocket(0, 1, InetAddress.getByName("127.0.0.1"))) {
      var served = new AtomicInteger();
      var serverFailure = new AtomicReference<Throwable>();
      var serverThread =
          new Thread(() -> serveOneStyle(server, served, serverFailure), "style-server");
      serverThread.start();

      var replacementUrl = "http://127.0.0.1:" + server.getLocalPort() + "/rewritten-style.json";
      var transformCalls = new AtomicInteger();
      try (var runtime = RuntimeHandle.create()) {
        runtime.setResourceTransform(
            request -> {
              transformCalls.incrementAndGet();
              return Optional.of(replacementUrl);
            });
        try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
          map.setStyleUrl("http://original.invalid/style.json");
          waitForMapEvent(runtime, map, RuntimeEventType.MAP_STYLE_LOADED);
        } finally {
          runtime.clearResourceTransform();
        }
      }

      serverThread.join(5_000);
      assertFalse(serverThread.isAlive());
      assertEquals(1, served.get());
      assertTrue(transformCalls.get() > 0);
      if (serverFailure.get() != null) {
        throw new AssertionError("style server failed", serverFailure.get());
      }
    } finally {
      restoreNetworkStatus(originalNetworkStatus);
    }
  }

  @Test
  void bnd080PollEventReturnsEmptyWhenNativeQueueIsEmpty() {
    try (var runtime = RuntimeHandle.create()) {
      runtime.runOnce();
      assertTrue(runtime.pollEvent().isEmpty());
    }
  }

  @Test
  void bnd083UnknownPayloadBytesAreCopiedValueData() {
    var originalBytes = new byte[] {1, 2, 3};
    var payload = new RuntimeEventPayload.Unknown(1234, originalBytes.length, originalBytes);
    originalBytes[0] = 9;

    var sameValue = new RuntimeEventPayload.Unknown(1234, 3, new byte[] {1, 2, 3});
    assertEquals(sameValue, payload);
    assertEquals(sameValue.hashCode(), payload.hashCode());

    var returnedBytes = payload.payloadBytes();
    returnedBytes[0] = 9;
    assertArrayEquals(new byte[] {1, 2, 3}, payload.payloadBytes());
  }

  @Test
  void bnd190AndBnd191RuntimeWrongThreadReportsCopiedDiagnostic() throws Exception {
    try (var runtime = RuntimeHandle.create()) {
      var error = assertWrongThread(runOnOtherThread(runtime::runOnce));

      assertEquals(MaplibreStatus.WRONG_THREAD.nativeCode(), error.nativeStatusCode());
      assertTrue(error.diagnostic().contains("runtime call"));
    }
  }

  private static void waitForMapEvent(RuntimeHandle runtime, MapHandle map, RuntimeEventType type)
      throws InterruptedException {
    awaitMapEvent(runtime, map, type);
  }

  private static void restoreNetworkStatus(NetworkStatus networkStatus) {
    try {
      Maplibre.setNetworkStatus(networkStatus);
    } catch (IllegalArgumentException ignored) {
      // Preserve the previous behavior: native statuses unknown to this binding
      // cannot be passed back as inputs.
    }
  }

  private static boolean awaitCancelled(ResourceRequestHandle handle) throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      if (handle.isCancelled()) {
        return true;
      }
      Thread.sleep(1);
    }
    return false;
  }

  private static RuntimeEvent awaitMapEvent(
      RuntimeHandle runtime, MapHandle map, RuntimeEventType type) throws InterruptedException {
    for (var attempt = 0; attempt < 1000; attempt++) {
      runtime.runOnce();
      while (true) {
        var event = runtime.pollEvent();
        if (event.isEmpty()) {
          break;
        }
        var value = event.get();
        if (value.type() == type && value.mapSource().filter(source -> source == map).isPresent()) {
          return value;
        }
      }
      Thread.sleep(1);
    }
    throw new AssertionError("Timed out waiting for " + type);
  }

  private static void serveOneStyle(
      ServerSocket server, AtomicInteger served, AtomicReference<Throwable> failure) {
    try (var socket = server.accept();
        var reader =
            new BufferedReader(
                new InputStreamReader(socket.getInputStream(), StandardCharsets.US_ASCII));
        var output = socket.getOutputStream()) {
      while (true) {
        var line = reader.readLine();
        if (line == null || line.isEmpty()) {
          break;
        }
      }
      var bytes = STYLE_JSON.getBytes(StandardCharsets.UTF_8);
      output.write(
          ("HTTP/1.1 200 OK\r\n"
                  + "Content-Type: application/json\r\n"
                  + "Content-Length: "
                  + bytes.length
                  + "\r\n"
                  + "Connection: close\r\n"
                  + "\r\n")
              .getBytes(StandardCharsets.US_ASCII));
      output.write(bytes);
      output.flush();
      served.incrementAndGet();
    } catch (IOException exception) {
      failure.set(exception);
    }
  }

  private static WrongThreadException assertWrongThread(Throwable thrown) {
    assertTrue(thrown instanceof WrongThreadException, () -> String.valueOf(thrown));
    var error = (WrongThreadException) thrown;
    assertEquals(MaplibreStatus.WRONG_THREAD, error.status());
    assertFalse(error.diagnostic().isBlank());
    return error;
  }

  private static String captureStderr(Runnable runnable) {
    var previous = System.err;
    var bytes = new ByteArrayOutputStream();
    try {
      System.setErr(new PrintStream(bytes, true, StandardCharsets.UTF_8));
      runnable.run();
    } finally {
      System.setErr(previous);
    }
    return bytes.toString(StandardCharsets.UTF_8);
  }

  private static Throwable runOnOtherThread(ThrowingRunnable action) throws InterruptedException {
    var thrown = new AtomicReference<Throwable>();
    var thread =
        new Thread(
            () -> {
              try {
                action.run();
              } catch (Throwable error) {
                thrown.set(error);
              }
            });
    thread.start();
    thread.join(5_000);
    if (thread.isAlive()) {
      thread.interrupt();
      return new AssertionError("Timed out waiting for cross-thread action to finish");
    }
    return thrown.get();
  }

  @FunctionalInterface
  private interface ThrowingRunnable {
    void run() throws Exception;
  }
}
