package org.maplibre.nativeffi.examples.lwjglmap;

import static org.lwjgl.system.MemoryUtil.NULL;
import static org.lwjgl.system.MemoryUtil.memAddress;
import static org.lwjgl.system.MemoryUtil.memUTF8Safe;
import static org.lwjgl.system.macosx.DynamicLinkLoader.RTLD_LOCAL;
import static org.lwjgl.system.macosx.DynamicLinkLoader.RTLD_NOW;
import static org.lwjgl.system.macosx.DynamicLinkLoader.dlerror;
import static org.lwjgl.system.macosx.DynamicLinkLoader.dlopen;
import static org.lwjgl.system.macosx.DynamicLinkLoader.dlsym;

import java.util.HashMap;
import java.util.Map;
import org.lwjgl.system.JNI;
import org.lwjgl.system.MemoryStack;
import org.lwjgl.system.macosx.CoreFoundation;
import org.lwjgl.system.macosx.ObjCRuntime;

final class MacObjectiveC {
  private static final Map<String, Long> SELECTORS = new HashMap<>();
  private static final Map<String, Long> CLASSES = new HashMap<>();
  private static final Map<String, Long> FRAMEWORKS = new HashMap<>();

  private MacObjectiveC() {}

  static long allocInit(String className) {
    return sendPointer(sendPointer(cls(className), "alloc"), "init");
  }

  static long retain(long object) {
    return sendPointer(object, "retain");
  }

  static void release(long object) {
    if (object != NULL) {
      sendVoid(object, "release");
    }
  }

  static long metalSystemDefaultDevice() {
    loadFramework("Metal");
    var address = function("Metal", "MTLCreateSystemDefaultDevice");
    return JNI.invokeP(address);
  }

  static long cfString(String value) {
    try (var stack = MemoryStack.stackPush()) {
      return CoreFoundation.CFStringCreateWithCString(
          CoreFoundation.kCFAllocatorDefault,
          stack.UTF8(value),
          CoreFoundation.kCFStringEncodingUTF8);
    }
  }

  static String nsString(long string) {
    if (string == NULL) {
      return null;
    }
    return memUTF8Safe(sendPointer(string, "UTF8String"));
  }

  static long sendPointer(long receiver, String selectorName) {
    var selector = selector(selectorName);
    return JNI.invokePPP(receiver, selector, implementation(receiver, selector));
  }

  static long sendPointer(long receiver, String selectorName, int argument) {
    var selector = selector(selectorName);
    return JNI.invokePPP(receiver, selector, argument, implementation(receiver, selector));
  }

  static long sendPointer(long receiver, String selectorName, long argument) {
    var selector = selector(selectorName);
    return JNI.invokePPPP(receiver, selector, argument, implementation(receiver, selector));
  }

  static long sendPointer(long receiver, String selectorName, long first, long second) {
    var selector = selector(selectorName);
    return JNI.invokePPPPP(receiver, selector, first, second, implementation(receiver, selector));
  }

  static long sendPointer(long receiver, String selectorName, long first, long second, long third) {
    var selector = selector(selectorName);
    return JNI.invokePPPPPP(
        receiver, selector, first, second, third, implementation(receiver, selector));
  }

  static void sendVoid(long receiver, String selectorName) {
    var selector = selector(selectorName);
    JNI.invokePPV(receiver, selector, implementation(receiver, selector));
  }

  static void sendVoid(long receiver, String selectorName, boolean argument) {
    var selector = selector(selectorName);
    JNI.invokePPV(receiver, selector, argument, implementation(receiver, selector));
  }

  static void sendVoid(long receiver, String selectorName, int argument) {
    var selector = selector(selectorName);
    JNI.invokePPV(receiver, selector, argument, implementation(receiver, selector));
  }

  static void sendVoid(long receiver, String selectorName, long argument) {
    var selector = selector(selectorName);
    JNI.invokePPPV(receiver, selector, argument, implementation(receiver, selector));
  }

  static void sendVoid(long receiver, String selectorName, long first, int second) {
    var selector = selector(selectorName);
    JNI.invokePPPV(receiver, selector, first, second, implementation(receiver, selector));
  }

  static void sendVoid(long receiver, String selectorName, int first, int second, int third) {
    var selector = selector(selectorName);
    JNI.invokePPV(receiver, selector, first, second, third, implementation(receiver, selector));
  }

  static void sendSize(long receiver, String selectorName, double width, double height) {
    var targetSelector = selector(selectorName);
    try (var pool = autoreleasePool();
        var stack = MemoryStack.stackPush()) {
      var signature = sendPointer(receiver, "methodSignatureForSelector:", targetSelector);
      if (signature == NULL) {
        throw new IllegalStateException(
            "Objective-C selector has no method signature: " + selectorName);
      }
      var invocation =
          sendPointer(cls("NSInvocation"), "invocationWithMethodSignature:", signature);
      var size = stack.mallocDouble(2);
      size.put(0, width);
      size.put(1, height);
      sendVoid(invocation, "setTarget:", receiver);
      sendVoid(invocation, "setSelector:", targetSelector);
      sendVoid(invocation, "setArgument:atIndex:", memAddress(size), 2);
      sendVoid(invocation, "invoke");
    }
  }

  static Pool autoreleasePool() {
    return new Pool(allocInit("NSAutoreleasePool"));
  }

  static String errorDescription(long error) {
    if (error == NULL) {
      return "unknown Objective-C error";
    }
    return nsString(sendPointer(error, "localizedDescription"));
  }

  private static long cls(String name) {
    return CLASSES.computeIfAbsent(
        name,
        key -> {
          loadFrameworkForClass(key);
          var value = ObjCRuntime.objc_getClass(key);
          if (value == NULL) {
            throw new IllegalStateException("Objective-C class not found: " + key);
          }
          return value;
        });
  }

  private static void loadFrameworkForClass(String className) {
    if (className.startsWith("CA")) {
      loadFramework("QuartzCore");
    } else if (className.startsWith("MTL")) {
      loadFramework("Metal");
    } else {
      loadFramework("Foundation");
    }
  }

  private static long selector(String name) {
    return SELECTORS.computeIfAbsent(name, ObjCRuntime::sel_registerName);
  }

  private static long implementation(long receiver, long selector) {
    if (receiver == NULL) {
      throw new IllegalStateException("Objective-C receiver is null");
    }
    var objectClass = ObjCRuntime.object_getClass(receiver);
    var implementation = ObjCRuntime.class_getMethodImplementation(objectClass, selector);
    if (implementation == NULL) {
      throw new IllegalStateException(
          "Objective-C selector implementation not found: " + ObjCRuntime.sel_getName(selector));
    }
    return implementation;
  }

  private static long function(String framework, String symbol) {
    var address = dlsym(loadFramework(framework), symbol);
    if (address == NULL) {
      throw new IllegalStateException(framework + " symbol not found: " + symbol);
    }
    return address;
  }

  private static long loadFramework(String framework) {
    return FRAMEWORKS.computeIfAbsent(
        framework,
        name -> {
          var path = "/System/Library/Frameworks/" + name + ".framework/" + name;
          var handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
          if (handle == NULL) {
            throw new IllegalStateException("Failed to load framework " + path + ": " + dlerror());
          }
          return handle;
        });
  }

  static final class Pool implements AutoCloseable {
    private long pool;

    private Pool(long pool) {
      this.pool = pool;
    }

    @Override
    public void close() {
      if (pool != NULL) {
        sendVoid(pool, "drain");
        pool = NULL;
      }
    }
  }
}
