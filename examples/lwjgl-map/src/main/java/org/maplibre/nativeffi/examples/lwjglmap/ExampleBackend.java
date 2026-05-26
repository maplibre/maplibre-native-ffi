package org.maplibre.nativeffi.examples.lwjglmap;

enum ExampleBackend {
  VULKAN("vulkan"),
  OPENGL("opengl");

  private final String cliName;

  ExampleBackend(String cliName) {
    this.cliName = cliName;
  }

  String cliName() {
    return cliName;
  }

  static ExampleBackend parse(String value) {
    for (var backend : values()) {
      if (backend.cliName.equals(value)) {
        return backend;
      }
    }
    throw new IllegalArgumentException("unknown backend '" + value + "'");
  }
}
