package org.maplibre.nativejni.internal.refs;

import static org.junit.jupiter.api.Assertions.assertNull;

import java.lang.ref.WeakReference;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.log.LogCallback;
import org.maplibre.nativejni.test.NativeTestSupport;

final class GlobalReferenceTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @AfterEach
  void clearLogCallback() {
    Maplibre.clearLogCallback();
  }

  @Test
  void logCallbackGlobalReferencesReleaseOnReplaceAndClear() {
    var first = installCallback();
    var second = installCallback();

    awaitCleared(first);
    Maplibre.clearLogCallback();
    awaitCleared(second);
  }

  private static WeakReference<LogCallback> installCallback() {
    LogCallback callback =
        new LogCallback() {
          @Override
          public boolean log(org.maplibre.nativejni.log.LogRecord record) {
            return true;
          }
        };
    var reference = new WeakReference<>(callback);
    Maplibre.setLogCallback(callback);
    return reference;
  }

  private static void awaitCleared(WeakReference<?> reference) {
    for (var i = 0; i < 20 && reference.get() != null; i++) {
      System.gc();
      try {
        Thread.sleep(10);
      } catch (InterruptedException interrupted) {
        Thread.currentThread().interrupt();
        break;
      }
    }
    assertNull(reference.get());
  }
}
