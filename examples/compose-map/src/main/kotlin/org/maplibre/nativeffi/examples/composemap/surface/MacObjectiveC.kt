package org.maplibre.nativeffi.examples.composemap.surface

import org.lwjgl.system.JNI
import org.lwjgl.system.MemoryUtil.NULL
import org.lwjgl.system.macosx.DynamicLinkLoader.RTLD_LOCAL
import org.lwjgl.system.macosx.DynamicLinkLoader.RTLD_NOW
import org.lwjgl.system.macosx.DynamicLinkLoader.dlerror
import org.lwjgl.system.macosx.DynamicLinkLoader.dlopen
import org.lwjgl.system.macosx.ObjCRuntime

internal object MacObjectiveC {
  private val selectors = mutableMapOf<String, Long>()
  private val classes = mutableMapOf<String, Long>()
  private val frameworks = mutableMapOf<String, Long>()

  fun allocInit(className: String): Long = sendPointer(sendPointer(cls(className), "alloc"), "init")

  fun release(objectAddress: Long) {
    if (objectAddress != NULL) {
      sendVoid(objectAddress, "release")
    }
  }

  fun autoreleasePool(): Pool = Pool(allocInit("NSAutoreleasePool"))

  fun sendPointer(receiver: Long, selectorName: String): Long {
    val selector = selector(selectorName)
    return JNI.invokePPP(receiver, selector, implementation(receiver, selector))
  }

  fun sendPointer(receiver: Long, selectorName: String, argument: Long): Long {
    val selector = selector(selectorName)
    return JNI.invokePPPP(receiver, selector, argument, implementation(receiver, selector))
  }

  fun sendLong(receiver: Long, selectorName: String): Long = sendPointer(receiver, selectorName)

  fun sendVoid(receiver: Long, selectorName: String) {
    val selector = selector(selectorName)
    JNI.invokePPV(receiver, selector, implementation(receiver, selector))
  }

  fun sendVoid(receiver: Long, selectorName: String, argument: Long) {
    val selector = selector(selectorName)
    JNI.invokePPPV(receiver, selector, argument, implementation(receiver, selector))
  }

  @Synchronized
  private fun cls(name: String): Long =
    classes.getOrPut(name) {
      loadFrameworkForClass(name)
      val value = ObjCRuntime.objc_getClass(name)
      check(value != NULL) { "Objective-C class not found: $name" }
      value
    }

  private fun loadFrameworkForClass(className: String) {
    when {
      className.startsWith("MTL") -> loadFramework("Metal")
      else -> loadFramework("Foundation")
    }
  }

  @Synchronized
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

  @Synchronized
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
