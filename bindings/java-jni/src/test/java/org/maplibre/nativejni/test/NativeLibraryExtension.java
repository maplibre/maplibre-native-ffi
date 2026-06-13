package org.maplibre.nativejni.test;

import org.junit.jupiter.api.extension.BeforeAllCallback;
import org.junit.jupiter.api.extension.ExtensionContext;

public final class NativeLibraryExtension implements BeforeAllCallback {
  @Override
  public void beforeAll(ExtensionContext context) {
    NativeTestSupport.loadNativeLibrary();
  }
}
