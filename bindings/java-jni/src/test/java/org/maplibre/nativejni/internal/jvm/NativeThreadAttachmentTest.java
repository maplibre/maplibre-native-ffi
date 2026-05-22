package org.maplibre.nativejni.internal.jvm;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.atomic.AtomicReference;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.bridge.JniTestNative;
import org.maplibre.nativejni.test.NativeTestSupport;

final class NativeThreadAttachmentTest {
  @BeforeAll
  static void loadNativeLibrary() {
    NativeTestSupport.loadNativeLibraryOrSkip();
  }

  @Test
  void nativeCreatedThreadAttachesBeforeInvokingJava() {
    var testThread = Thread.currentThread();
    var callbackThread = new AtomicReference<Thread>();

    assertTrue(
        JniTestNative.invokeOnAttachedNativeThread(
            () -> callbackThread.set(Thread.currentThread())));
    assertTrue(callbackThread.get() != null);
    assertEquals("Thread", callbackThread.get().getClass().getSimpleName());
    assertTrue(callbackThread.get() != testThread);
  }
}
