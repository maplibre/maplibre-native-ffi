package org.maplibre.nativeffi.docsnippets;

import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.TimeUnit;
import org.maplibre.nativeffi.LogicalExtent;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MapOptions;
import org.maplibre.nativeffi.ResourceErrorReason;
import org.maplibre.nativeffi.ResourceProvider;
import org.maplibre.nativeffi.ResourceResponse;
import org.maplibre.nativeffi.ResourceResponseStatus;
import org.maplibre.nativeffi.ResourceRoute;
import org.maplibre.nativeffi.RuntimeHandle;
import org.maplibre.nativeffi.UrlMatchFlag;

/**
 * What the Java snippets' tests share and the pages leave out: a
 * runtime that answers every HTTP and HTTPS request itself, and a map
 * on it.
 */
final class SnippetFixture implements AutoCloseable {
  /**
   * How long any single wait in a snippet test lasts before it fails.
   */
  static final long WAIT_SECONDS = 10;

  /**
   * How long one snippet test may run before it fails instead of
   * hanging the suite.
   */
  static final long TEST_TIMEOUT_MS = 60_000;

  /**
   * Smallest style a map can load: valid, empty, and free of resource
   * requests.
   */
  static final String EMPTY_STYLE_JSON =
      "{\"version\":8,\"sources\":{},\"layers\":[]}";

  final RuntimeHandle runtime;
  final MapHandle map;

  private SnippetFixture(RuntimeHandle runtime, MapHandle map) {
    this.runtime = runtime;
    this.map = map;
  }

  /**
   * A fresh runtime kept off the network, with a small map that is
   * ready.
   */
  static SnippetFixture withMap() {
    RuntimeHandle runtime = RuntimeHandle.create(null).join();
    keepOffTheNetwork(runtime);
    MapHandle map =
        runtime.createMap(
            MapOptions.of(LogicalExtent.of(64, 64, 1.0)));
    map.getReady().join();
    return new SnippetFixture(runtime, map);
  }

  /**
   * Answers every HTTP and HTTPS request of {@code runtime} with a
   * NOT_FOUND error, so a snippet that names a real host never
   * reaches the network.
   */
  static void keepOffTheNetwork(RuntimeHandle runtime) {
    List<ResourceRoute> routes =
        List.of("http://**", "https://**").stream()
            .map(
                url ->
                    ResourceRoute.of()
                        .withFlags(UrlMatchFlag.GLOB)
                        .withUrl(url))
            .toList();
    ResourceResponse notFound =
        ResourceResponse.of()
            .withStatus(ResourceResponseStatus.ERROR)
            .withErrorReason(ResourceErrorReason.NOT_FOUND)
            .withErrorMessage("the snippet tests serve no resources");
    runtime
        .setResourceProvider(
            ResourceProvider.of(
                    (request, handle) -> handle.complete(notFound))
                .withRoutes(routes))
        .join();
  }

  /**
   * The value of {@code future}, failing the test when it doesn't
   * complete within {@link #WAIT_SECONDS}.
   */
  static <T> T within(CompletableFuture<T> future) throws Exception {
    return future.get(WAIT_SECONDS, TimeUnit.SECONDS);
  }

  /** Releases the map, then the runtime, and waits for each. */
  @Override
  public void close() {
    try {
      map.release().join();
    } finally {
      runtime.release().join();
    }
  }
}
