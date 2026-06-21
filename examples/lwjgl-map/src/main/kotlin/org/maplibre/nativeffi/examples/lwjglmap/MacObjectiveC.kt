package org.maplibre.nativeffi.examples.lwjglmap

import org.lwjgl.system.JNI
import org.lwjgl.system.MemoryStack
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.MemoryUtil.memUTF8Safe
import org.lwjgl.system.macosx.CoreFoundation
import org.lwjgl.system.macosx.DynamicLinkLoader.RTLD_LOCAL
import org.lwjgl.system.macosx.DynamicLinkLoader.RTLD_NOW
import org.lwjgl.system.macosx.DynamicLinkLoader.dlerror
import org.lwjgl.system.macosx.DynamicLinkLoader.dlopen
import org.lwjgl.system.macosx.DynamicLinkLoader.dlsym
import org.lwjgl.system.macosx.ObjCRuntime

internal object MacObjectiveC {
  private val selectors = mutableMapOf<String, Long>()
  private val classes = mutableMapOf<String, Long>()
  private val frameworks = mutableMapOf<String, Long>()

  fun allocInit(className: String): Long = sendPointer(sendPointer(cls(className), "alloc"), "init")

  fun retain(objectAddress: Long): Long = sendPointer(objectAddress, "retain")

  fun release(objectAddress: Long) {
    if (objectAddress != NULL) {
      sendVoid(objectAddress, "release")
    }
  }

  fun metalSystemDefaultDevice(): Long {
    loadFramework("Metal")
    val address = function("Metal", "MTLCreateSystemDefaultDevice")
    return JNI.invokeP(address)
  }

  fun cfString(value: String): Long =
    MemoryStack.stackPush().use { stack ->
      CoreFoundation.CFStringCreateWithCString(
        CoreFoundation.kCFAllocatorDefault,
        stack.UTF8(value),
        CoreFoundation.kCFStringEncodingUTF8,
      )
    }

  fun nsString(string: Long): String? =
    if (string == NULL) null else memUTF8Safe(sendPointer(string, "UTF8String"))

  fun sendPointer(receiver: Long, selectorName: String): Long {
    val selector = selector(selectorName)
    return JNI.invokePPP(receiver, selector, implementation(receiver, selector))
  }

  fun sendPointer(receiver: Long, selectorName: String, argument: Int): Long {
    val selector = selector(selectorName)
    return JNI.invokePPP(receiver, selector, argument, implementation(receiver, selector))
  }

  fun sendPointer(receiver: Long, selectorName: String, argument: Long): Long {
    val selector = selector(selectorName)
    return JNI.invokePPPP(receiver, selector, argument, implementation(receiver, selector))
  }

  fun sendPointer(receiver: Long, selectorName: String, first: Long, second: Long): Long {
    val selector = selector(selectorName)
    return JNI.invokePPPPP(receiver, selector, first, second, implementation(receiver, selector))
  }

  fun sendPointer(
    receiver: Long,
    selectorName: String,
    first: Long,
    second: Long,
    third: Long,
  ): Long {
    val selector = selector(selectorName)
    return JNI.invokePPPPPP(
      receiver,
      selector,
      first,
      second,
      third,
      implementation(receiver, selector),
    )
  }

  fun sendVoid(receiver: Long, selectorName: String) {
    val selector = selector(selectorName)
    JNI.invokePPV(receiver, selector, implementation(receiver, selector))
  }

  fun sendVoid(receiver: Long, selectorName: String, argument: Boolean) {
    val selector = selector(selectorName)
    JNI.invokePPV(receiver, selector, argument, implementation(receiver, selector))
  }

  fun sendVoid(receiver: Long, selectorName: String, argument: Int) {
    val selector = selector(selectorName)
    JNI.invokePPV(receiver, selector, argument, implementation(receiver, selector))
  }

  fun sendVoid(receiver: Long, selectorName: String, argument: Long) {
    val selector = selector(selectorName)
    JNI.invokePPPV(receiver, selector, argument, implementation(receiver, selector))
  }

  fun sendVoid(receiver: Long, selectorName: String, first: Long, second: Int) {
    val selector = selector(selectorName)
    JNI.invokePPPV(receiver, selector, first, second, implementation(receiver, selector))
  }

  fun sendVoid(receiver: Long, selectorName: String, first: Int, second: Int, third: Int) {
    val selector = selector(selectorName)
    JNI.invokePPV(receiver, selector, first, second, third, implementation(receiver, selector))
  }

  fun sendSize(receiver: Long, selectorName: String, width: Double, height: Double) {
    val targetSelector = selector(selectorName)
    autoreleasePool().use {
      MemoryStack.stackPush().use { stack ->
        val signature = sendPointer(receiver, "methodSignatureForSelector:", targetSelector)
        check(signature != NULL) { "Objective-C selector has no method signature: $selectorName" }
        val invocation =
          sendPointer(cls("NSInvocation"), "invocationWithMethodSignature:", signature)
        val size = stack.mallocDouble(2)
        size.put(0, width)
        size.put(1, height)
        sendVoid(invocation, "setTarget:", receiver)
        sendVoid(invocation, "setSelector:", targetSelector)
        sendVoid(
          invocation,
          "setArgument:atIndex:",
          org.lwjgl.system.MemoryUtil.memAddress(size),
          2,
        )
        sendVoid(invocation, "invoke")
      }
    }
  }

  fun autoreleasePool(): Pool = Pool(allocInit("NSAutoreleasePool"))

  fun errorDescription(error: Long): String =
    if (error == NULL) {
      "unknown Objective-C error"
    } else {
      nsString(sendPointer(error, "localizedDescription")) ?: "unknown Objective-C error"
    }

  private fun cls(name: String): Long =
    classes.getOrPut(name) {
      loadFrameworkForClass(name)
      val value = ObjCRuntime.objc_getClass(name)
      check(value != NULL) { "Objective-C class not found: $name" }
      value
    }

  private fun loadFrameworkForClass(className: String) {
    when {
      className.startsWith("CA") -> loadFramework("QuartzCore")
      className.startsWith("MTL") -> loadFramework("Metal")
      else -> loadFramework("Foundation")
    }
  }

  private fun selector(name: String): Long =
    selectors.getOrPut(name) { ObjCRuntime.sel_registerName(name) }

  private fun implementation(receiver: Long, selector: Long): Long {
    check(receiver != NULL) { "Objective-C receiver is null" }
    val objectClass = ObjCRuntime.object_getClass(receiver)
    val implementation = ObjCRuntime.class_getMethodImplementation(objectClass, selector)
    check(implementation != NULL) {
      "Objective-C selector implementation not found: ${ObjCRuntime.sel_getName(selector)}"
    }
    return implementation
  }

  private fun function(framework: String, symbol: String): Long {
    val address = dlsym(loadFramework(framework), symbol)
    check(address != NULL) { "$framework symbol not found: $symbol" }
    return address
  }

  private fun loadFramework(framework: String): Long =
    frameworks.getOrPut(framework) {
      val path = "/System/Library/Frameworks/$framework.framework/$framework"
      val handle = dlopen(path, RTLD_NOW or RTLD_LOCAL)
      check(handle != NULL) { "Failed to load framework $path: ${dlerror()}" }
      handle
    }

  internal class Pool(private var pool: Long) : AutoCloseable {
    override fun close() {
      if (pool != NULL) {
        sendVoid(pool, "drain")
        pool = NULL
      }
    }
  }
}
