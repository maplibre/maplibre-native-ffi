package org.maplibre.nativejni.internal.javacpp;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotEquals;

import java.util.concurrent.atomic.AtomicInteger;
import org.bytedeco.javacpp.Pointer;
import org.junit.jupiter.api.Test;

final class JavaCppBoundaryTest {
  @Test
  void nativeThreadCallbackAttachesBeforeCallingJava() {
    var javaThreadId = Thread.currentThread().threadId();
    var callbackThreadId = new AtomicInteger(0);
    var callback =
        new JavaCppBoundaryTestC.mln_javacpp_test_callback() {
          @Override
          public int call(Pointer userData) {
            callbackThreadId.set((int) Thread.currentThread().threadId());
            return 7;
          }
        };

    assertEquals(7, JavaCppBoundaryTestC.mln_javacpp_test_invoke_on_native_thread(callback, null));
    assertNotEquals(javaThreadId, callbackThreadId.get());
  }

  @Test
  void repeatedNativeCallbacksKeepReferencesScoped() {
    var callback =
        new JavaCppBoundaryTestC.mln_javacpp_test_callback() {
          @Override
          public int call(Pointer userData) {
            return String.valueOf(System.nanoTime()).isEmpty() ? 0 : 1;
          }
        };

    assertEquals(4096, JavaCppBoundaryTestC.mln_javacpp_test_repeat_callback(callback, null, 4096));
  }
}
