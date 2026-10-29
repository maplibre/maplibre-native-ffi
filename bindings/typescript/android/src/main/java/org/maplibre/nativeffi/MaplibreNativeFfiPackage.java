package org.maplibre.nativeffi;

import com.facebook.react.BaseReactPackage;
import com.facebook.react.bridge.NativeModule;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.module.model.ReactModuleInfo;
import com.facebook.react.module.model.ReactModuleInfoProvider;
import java.util.Collections;

public final class MaplibreNativeFfiPackage extends BaseReactPackage {
  @Override
  public NativeModule getModule(String name, ReactApplicationContext context) {
    return "MaplibreNativeFfi".equals(name) ? new MaplibreNativeFfiModule(context) : null;
  }

  @Override
  public ReactModuleInfoProvider getReactModuleInfoProvider() {
    return () ->
        Collections.singletonMap(
            "MaplibreNativeFfi",
            new ReactModuleInfo(
                "MaplibreNativeFfi", "MaplibreNativeFfi", false, false, false, true));
  }
}
