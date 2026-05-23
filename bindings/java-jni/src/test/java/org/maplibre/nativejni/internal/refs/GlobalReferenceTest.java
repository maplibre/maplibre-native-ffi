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
  void logCallbackGlobalReferencesReleaseAfterReplaceAndClear() {
    var first = installCallback();
    var second = installCallback();

    assertReleased(first);
    Maplibre.clearLogCallback();
    assertReleased(second);
  }

  private static WeakReference<LogCallback> installCallback() {
    LogCallback callback = new CapturingLogCallback(new Object());
    var reference = new WeakReference<>(callback);
    Maplibre.setLogCallback(callback);
    return reference;
  }

  private record CapturingLogCallback(Object marker) implements LogCallback {
    @Override
    public boolean log(org.maplibre.nativejni.log.LogRecord record) {
      return marker != null;
    }
  }

  private static void assertReleased(WeakReference<?> reference) {
    for (var i = 0; i < 20; i++) {
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
