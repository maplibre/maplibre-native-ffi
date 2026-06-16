package org.maplibre.nativejni;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

import java.lang.module.ModuleDescriptor;
import java.lang.module.ModuleFinder;
import java.lang.reflect.Modifier;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import org.junit.jupiter.api.Test;
import org.maplibre.nativejni.internal.access.InternalAccess;
import org.maplibre.nativejni.map.MapHandle;
import org.maplibre.nativejni.map.MapOptions;
import org.maplibre.nativejni.resource.ResourceRequestHandle;
import org.maplibre.nativejni.runtime.RuntimeHandle;

class PublicApiSurfaceTest {
  @Test
  void bnd002ModuleDoesNotExportRawOrInternalPackages() {
    var exports =
        descriptor().exports().stream()
            .map(ModuleDescriptor.Exports::source)
            .collect(Collectors.toUnmodifiableSet());

    assertTrue(exports.contains("org.maplibre.nativejni"));
    assertTrue(exports.contains("org.maplibre.nativejni.render"));
    assertFalse(exports.stream().anyMatch(packageName -> packageName.contains(".internal")));
    assertFalse(exports.contains("org.maplibre.nativejni.internal.javacpp"));
  }

  @Test
  void bnd002JavacppDependencyIsNotTransitiveApi() {
    var javacppRequires =
        descriptor().requires().stream()
            .filter(requires -> requires.name().equals("org.bytedeco.javacpp"))
            .findFirst()
            .orElseThrow();

    assertFalse(
        javacppRequires.modifiers().contains(ModuleDescriptor.Requires.Modifier.TRANSITIVE));
  }

  @Test
  void bnd002PublicMembersDoNotExposeRawFfiTypes() throws Exception {
    for (var type : publicClasses()) {
      for (var constructor : type.getConstructors()) {
        for (var parameterType : constructor.getParameterTypes()) {
          assertSupportedApiType(parameterType);
        }
      }
      for (var method : type.getMethods()) {
        if (method.getDeclaringClass() == Object.class) {
          continue;
        }
        assertSupportedApiType(method.getReturnType());
        for (var parameterType : method.getParameterTypes()) {
          assertSupportedApiType(parameterType);
        }
      }
      for (var field : type.getFields()) {
        assertSupportedApiType(field.getType());
      }
    }
  }

  @Test
  void bnd047InternalAccessTokenRejectsPublicCallers() {
    org.junit.jupiter.api.Assertions.assertThrows(
        SecurityException.class, () -> new ResourceRequestHandle(InternalAccess.INSTANCE, 0x1234));
    try (var runtime = RuntimeHandle.create()) {
      org.junit.jupiter.api.Assertions.assertThrows(
          SecurityException.class, () -> runtime.nativeAddress(InternalAccess.INSTANCE));
      org.junit.jupiter.api.Assertions.assertThrows(
          SecurityException.class, () -> runtime.retainChild(InternalAccess.INSTANCE, "test"));
      try (var map = MapHandle.create(runtime, new MapOptions().size(64, 64))) {
        org.junit.jupiter.api.Assertions.assertThrows(
            SecurityException.class, () -> runtime.registerMap(InternalAccess.INSTANCE, map));
        org.junit.jupiter.api.Assertions.assertThrows(
            SecurityException.class, () -> runtime.unregisterMap(InternalAccess.INSTANCE, map));
        org.junit.jupiter.api.Assertions.assertThrows(
            SecurityException.class, () -> map.nativeAddress(InternalAccess.INSTANCE));
        org.junit.jupiter.api.Assertions.assertThrows(
            SecurityException.class, () -> map.retainChild(InternalAccess.INSTANCE, "test"));
        org.junit.jupiter.api.Assertions.assertThrows(
            SecurityException.class,
            () -> map.releaseDetachedCustomGeometrySources(InternalAccess.INSTANCE));
      }
    }
  }

  private static ModuleDescriptor descriptor() {
    for (var candidate :
        new Path[] {
          Path.of("build/classes/java/main"), Path.of("bindings/java-jni/build/classes/java/main")
        }) {
      if (!Files.exists(candidate.resolve("module-info.class"))) {
        continue;
      }
      return ModuleFinder.of(candidate).find("org.maplibre.nativejni").orElseThrow().descriptor();
    }
    throw new IllegalStateException("compiled org.maplibre.nativejni module not found");
  }

  private static Class<?>[] publicClasses() throws Exception {
    var exportedPackages =
        descriptor().exports().stream()
            .map(ModuleDescriptor.Exports::source)
            .collect(Collectors.toUnmodifiableSet());
    try (var files = Files.walk(classesRoot())) {
      return files
          .filter(path -> path.toString().endsWith(".class"))
          .filter(path -> !path.getFileName().toString().equals("module-info.class"))
          .map(PublicApiSurfaceTest::className)
          .flatMap(PublicApiSurfaceTest::loadClass)
          .filter(type -> exportedPackages.contains(type.getPackageName()))
          .filter(type -> Modifier.isPublic(type.getModifiers()))
          .toArray(Class<?>[]::new);
    }
  }

  private static Path classesRoot() {
    for (var candidate :
        new Path[] {
          Path.of("build/classes/java/main"), Path.of("bindings/java-jni/build/classes/java/main")
        }) {
      if (Files.exists(candidate.resolve("module-info.class"))) {
        return candidate;
      }
    }
    throw new IllegalStateException("compiled org.maplibre.nativejni module not found");
  }

  private static String className(Path path) {
    var relative = classesRoot().relativize(path).toString();
    return relative.substring(0, relative.length() - ".class".length()).replace('/', '.');
  }

  private static Stream<Class<?>> loadClass(String className) {
    try {
      return Stream.of(Class.forName(className));
    } catch (ClassNotFoundException exception) {
      throw new IllegalStateException(
          "compiled public API class not found: " + className, exception);
    }
  }

  private static void assertSupportedApiType(Class<?> type) {
    while (type.isArray()) {
      type = type.getComponentType();
    }
    var name = type.getName();
    assertFalse(name.startsWith("org.bytedeco.javacpp."), () -> "public API exposes " + name);
    assertFalse(
        name.startsWith("org.maplibre.nativejni.internal.javacpp."),
        () -> "public API exposes " + name);
  }
}
