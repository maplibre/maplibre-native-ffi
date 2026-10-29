package org.maplibre.nativeffi;

import com.facebook.proguard.annotations.DoNotStrip;
import com.facebook.react.ReactApplication;
import com.facebook.react.ReactHost;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.turbomodule.core.interfaces.BindingsInstallerHolder;
import com.facebook.react.turbomodule.core.interfaces.TurboModuleWithJSIBindings;
import java.util.Objects;
import java.util.concurrent.CountDownLatch;
import kotlin.Unit;
import kotlin.jvm.functions.Function0;

public final class MaplibreNativeFfiModule extends NativeMaplibreNativeFfiSpec
    implements TurboModuleWithJSIBindings {
  private final ReactHost host;
  private final Function0<Unit> beforeDestroy;
  @DoNotStrip private long nativeModule;

  public MaplibreNativeFfiModule(ReactApplicationContext context) {
    super(context);
    host =
        Objects.requireNonNull(
            ((ReactApplication) context.getApplicationContext()).getReactHost(),
            "MapLibre Native FFI needs ReactApplication.reactHost for JS-thread teardown");
    beforeDestroy =
        () -> {
          CountDownLatch completed = new CountDownLatch(1);
          // ReactInstance stops its JS queue before TurboModule.invalidate().
          // This earlier host hook waits until all JSI roots have been released.
          if (!context.runOnJSQueueThread(
              () -> {
                try {
                  cleanup(nativeModule);
                } finally {
                  completed.countDown();
                }
              }))
            throw new IllegalStateException(
                "MapLibre Native FFI's JS queue stopped before cleanup");
          try {
            completed.await();
          } catch (InterruptedException error) {
            Thread.currentThread().interrupt();
            throw new IllegalStateException("MapLibre Native FFI cleanup was interrupted", error);
          }
          return Unit.INSTANCE;
        };
    host.addBeforeDestroyListener(beforeDestroy);
  }

  @Override
  public String getName() {
    return "MaplibreNativeFfi";
  }

  @Override
  public void invalidate() {
    host.removeBeforeDestroyListener(beforeDestroy);
    destroy(nativeModule);
    nativeModule = 0;
    super.invalidate();
  }

  @DoNotStrip
  @Override
  public native BindingsInstallerHolder getBindingsInstaller();

  private native void cleanup(long module);

  private native void destroy(long module);

  static {
    System.loadLibrary("maplibre_native_ffi_react");
  }
}
