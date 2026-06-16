package org.maplibre.nativejni.internal.callback;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertThrows;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
import org.bytedeco.javacpp.BytePointer;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.Maplibre;
import org.maplibre.nativejni.internal.javacpp.MaplibreNativeC;
import org.maplibre.nativejni.log.LogRecord;

// Support invariant for BND-120 through BND-123: this internal state holder is how
// Java JNI makes global log callback replacement, host failures, and in-flight close deterministic.
class LogCallbackStateTest {
  @AfterEach
  void clearLogCallback() {
    LogCallbackState.resetInstallFailureForTesting();
    Maplibre.clearLogCallback();
  }

  @Test
  void bnd120PublicInstallReplacementAndClearRouteCurrentCallback() {
    var firstCount = new AtomicInteger();
    Maplibre.setLogCallback(
        record -> {
          firstCount.incrementAndGet();
          return true;
        });
    var first = LogCallbackState.currentCallbackForTesting();
    try (var message = new BytePointer("first")) {
      assertEquals(
          1,
          first
              .nativeCallback()
              .call(
                  null,
                  MaplibreNativeC.MLN_LOG_SEVERITY_INFO,
                  MaplibreNativeC.MLN_LOG_EVENT_GENERAL,
                  7,
                  message));
    }
    assertEquals(1, firstCount.get());

    var secondRecord = new AtomicReference<LogRecord>();
    Maplibre.setLogCallback(
        record -> {
          secondRecord.set(record);
          return false;
        });
    var second = LogCallbackState.currentCallbackForTesting();
    assertTrue(first.isClosed());
    assertFalse(second.isClosed());
    try (var message = new BytePointer("second")) {
      assertEquals(0, second.nativeCallback().call(null, 0x7fff, 0x7ffe, 42, message));
    }

    assertEquals(1, firstCount.get());
    assertEquals(0x7fff, secondRecord.get().severity().rawValue());
    assertEquals(0x7ffe, secondRecord.get().event().rawValue());
    assertEquals(42, secondRecord.get().code());
    assertEquals("second", secondRecord.get().message());

    Maplibre.clearLogCallback();
    assertTrue(second.isClosed());
    assertNull(LogCallbackState.currentCallbackForTesting());
  }

  @Test
  void bnd122ReplacementFailurePreservesCurrentCallbackAndClosesReplacement() {
    LogCallbackState.set(record -> true);
    var original = LogCallbackState.currentCallbackForTesting();
    var failure = new IllegalStateException("injected log callback install failure");

    LogCallbackState.failNextInstallForTesting(failure);
    assertSame(
        failure,
        assertThrows(IllegalStateException.class, () -> LogCallbackState.set(record -> false)));

    assertSame(original, LogCallbackState.currentCallbackForTesting());
    assertFalse(original.isClosed());
    assertTrue(LogCallbackState.failedInstallForTesting().isClosed());
  }

  @Test
  void bnd123CloseWaitsForInflightCallbackBeforeClosingStub() throws Exception {
    var entered = new CountDownLatch(1);
    var release = new CountDownLatch(1);
    var result = new AtomicInteger();
    var closed = new AtomicBoolean();
    var registration =
        new LogCallbackState.CallbackRegistration(
            record -> {
              entered.countDown();
              assertTrue(await(release));
              return true;
            });

    try (var message = new BytePointer("hello")) {
      var callbackThread =
          new Thread(
              () ->
                  result.set(
                      registration
                          .nativeCallback()
                          .call(
                              null,
                              MaplibreNativeC.MLN_LOG_SEVERITY_INFO,
                              MaplibreNativeC.MLN_LOG_EVENT_GENERAL,
                              7,
                              message)));
      callbackThread.start();
      assertTrue(await(entered));

      var closeThread =
          new Thread(
              () -> {
                registration.close();
                closed.set(true);
              });
      closeThread.start();

      Thread.sleep(25);
      assertFalse(closed.get());
      release.countDown();
      callbackThread.join();
      closeThread.join();

      assertTrue(closed.get());
      assertEquals(1, result.get());
    }
  }

  @Test
  void bnd123ReplacementAllowsInflightCallbackToInstallReplacement() throws Exception {
    var entered = new CountDownLatch(1);
    var replacementStarted = new CountDownLatch(1);
    var callbackInstalledReplacement = new AtomicBoolean();
    var replacementInstalled = new AtomicBoolean();
    var callbackFailure = new AtomicReference<Throwable>();
    var originalReference = new AtomicReference<LogCallbackState.CallbackRegistration>();

    Maplibre.setLogCallback(
        record -> {
          entered.countDown();
          try {
            assertTrue(await(replacementStarted));
            assertTrue(awaitReplacement(originalReference.get()));
            Maplibre.setLogCallback(next -> false);
            callbackInstalledReplacement.set(true);
          } catch (Throwable failure) {
            callbackFailure.set(failure);
          }
          return true;
        });
    var original = LogCallbackState.currentCallbackForTesting();
    originalReference.set(original);

    try (var message = new BytePointer("hello")) {
      var callbackThread =
          new Thread(
              () ->
                  original
                      .nativeCallback()
                      .call(
                          null,
                          MaplibreNativeC.MLN_LOG_SEVERITY_INFO,
                          MaplibreNativeC.MLN_LOG_EVENT_GENERAL,
                          7,
                          message));
      callbackThread.start();
      assertTrue(await(entered));

      var replacementThread =
          new Thread(
              () -> {
                replacementStarted.countDown();
                Maplibre.setLogCallback(record -> true);
                replacementInstalled.set(true);
              });
      replacementThread.start();

      callbackThread.join(5000);
      replacementThread.join(5000);

      assertFalse(callbackThread.isAlive());
      assertFalse(replacementThread.isAlive());
      assertNull(callbackFailure.get());
      assertTrue(callbackInstalledReplacement.get());
      assertTrue(replacementInstalled.get());
    }
  }

  @Test
  void bnd123CloseFromCurrentCallbackFailsWithoutDeadlock() {
    var attempted = new AtomicBoolean();
    var registration = new AtomicReference<LogCallbackState.CallbackRegistration>();
    registration.set(
        new LogCallbackState.CallbackRegistration(
            record -> {
              attempted.set(true);
              registration.get().close();
              return true;
            }));

    try (var message = new BytePointer("hello")) {
      assertEquals(
          0,
          registration
              .get()
              .nativeCallback()
              .call(
                  null,
                  MaplibreNativeC.MLN_LOG_SEVERITY_INFO,
                  MaplibreNativeC.MLN_LOG_EVENT_GENERAL,
                  7,
                  message));

      assertTrue(attempted.get());
      assertFalse(registration.get().isClosed());
    } finally {
      registration.get().close();
    }
  }

  @Test
  void bnd121CallbackExceptionsReturnUnconsumed() {
    var registration =
        new LogCallbackState.CallbackRegistration(
            record -> {
              throw new IllegalStateException("boom");
            });
    try (var message = new BytePointer("hello")) {
      assertEquals(
          0,
          registration
              .nativeCallback()
              .call(
                  null,
                  MaplibreNativeC.MLN_LOG_SEVERITY_INFO,
                  MaplibreNativeC.MLN_LOG_EVENT_GENERAL,
                  7,
                  message));
    } finally {
      registration.close();
    }
  }

  private static boolean await(CountDownLatch latch) {
    try {
      return latch.await(5, TimeUnit.SECONDS);
    } catch (InterruptedException exception) {
      Thread.currentThread().interrupt();
      return false;
    }
  }

  private static boolean awaitReplacement(LogCallbackState.CallbackRegistration original) {
    var deadline = System.nanoTime() + TimeUnit.SECONDS.toNanos(5);
    while (System.nanoTime() < deadline) {
      if (LogCallbackState.currentCallbackForTesting() != original) {
        return true;
      }
      try {
        Thread.sleep(10);
      } catch (InterruptedException exception) {
        Thread.currentThread().interrupt();
        return false;
      }
    }
    return false;
  }
}
