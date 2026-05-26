package org.maplibre.nativejni.internal.javacpp;

import org.bytedeco.javacpp.FunctionPointer;
import org.bytedeco.javacpp.Loader;
import org.bytedeco.javacpp.Pointer;

public class JavaCppBoundaryTestC extends JavaCppBoundaryTestCConfig {
  static {
    Loader.load();
  }

  public abstract static class mln_javacpp_test_callback extends FunctionPointer {
    static {
      Loader.load();
    }

    protected mln_javacpp_test_callback() {
      allocate();
    }

    private native void allocate();

    public abstract int call(Pointer userData);
  }

  public static native int mln_javacpp_test_invoke_on_native_thread(
      mln_javacpp_test_callback callback, Pointer userData);

  public static native int mln_javacpp_test_repeat_callback(
      mln_javacpp_test_callback callback, Pointer userData, int count);
}
