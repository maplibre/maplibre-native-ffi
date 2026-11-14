// Snippets for the docs site's Events and logs page. The test runs
// them, with every request answered by the test instead of the
// network.
package org.maplibre.nativeffi.docsnippets;

import java.util.List;
import java.util.concurrent.CompletableFuture;
import org.junit.Test;
import org.maplibre.nativeffi.LogHandler;
import org.maplibre.nativeffi.LogSeverity;
import org.maplibre.nativeffi.LogicalExtent;
import org.maplibre.nativeffi.MapHandle;
import org.maplibre.nativeffi.MapOptions;
import org.maplibre.nativeffi.MaplibreNative;
import org.maplibre.nativeffi.RuntimeEvent;
import org.maplibre.nativeffi.RuntimeEventFlag;
import org.maplibre.nativeffi.RuntimeEventHandler;
import org.maplibre.nativeffi.RuntimeEventType;
import org.maplibre.nativeffi.RuntimeHandle;

public class EventsAndLogsTest {
  private volatile MapHandle map;
  private final CompletableFuture<Void> styleLoaded =
      new CompletableFuture<>();

  private void onStyleLoaded() {
    styleLoaded.complete(null);
  }

  private void onLoadingFailed(String message) {
    styleLoaded.completeExceptionally(new AssertionError(message));
  }

  private void writeLog(LogSeverity severity, String message) {}

  // #region handler
  private void handleEvents(List<RuntimeEvent> events) {
    for (RuntimeEvent event : events) {
      if (map == null || event.getSource() != map.getId()) continue;

      switch (event.getType().getRawValue()) {
        case RuntimeEventType.MAP_STYLE_LOADED_VALUE ->
            onStyleLoaded();
        case RuntimeEventType.MAP_LOADING_FAILED_VALUE ->
            onLoadingFailed(event.getMessage());
        default -> {}
      }
    }
  }

  // #endregion handler

  @Test(timeout = SnippetFixture.TEST_TIMEOUT_MS)
  public void eventsAndLogs() throws Exception {
    RuntimeHandle runtime = RuntimeHandle.create(null);
    try {
      // #region handler
      // Runs on a library thread.
      runtime
          .getDiscarding()
          .setEventHandler(
              new RuntimeEventHandler(this::handleEvents));
      // #endregion handler

      // #region mask
      MapHandle created =
          runtime.createMap(
              MapOptions.of(LogicalExtent.of(512, 512, 1.0))
                  .withEventMask(
                      RuntimeEventFlag.MAP_STYLE_LOADED.or(
                          RuntimeEventFlag.MAP_LOADING_FAILED)));
      // #endregion mask
      map = created;
      try {
        created.getReady().join();
        created.setStyleJson(SnippetFixture.EMPTY_STYLE_JSON).join();
        SnippetFixture.within(styleLoaded);
      } finally {
        created.release().join();
      }

      try {
        // #region logs
        // Runs on a library thread, and can't call the library.
        MaplibreNative.setLogCallback(
            LogHandler.of(
                (severity, event, code, message) ->
                    writeLog(severity, message)));
        // #endregion logs
      } finally {
        MaplibreNative.clearLogCallback();
      }
    } finally {
      runtime.release().join();
    }
  }
}
