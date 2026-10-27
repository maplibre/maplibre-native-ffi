package org.maplibre.nativeffi

import java.lang.reflect.Modifier
import java.nio.file.Files
import java.nio.file.Path
import kotlin.io.path.extension
import kotlin.io.path.invariantSeparatorsPathString
import kotlin.system.exitProcess
import org.junit.Test
import org.junit.internal.TextListener
import org.junit.runner.JUnitCore

/**
 * Runs every test class under the class directory that its argument names, for a JVM that Gradle
 * does not start: one in Alpine or in the FreeBSD virtual machine.
 */
object DeviceSuite {
  @JvmStatic
  fun main(args: Array<String>) {
    val root = Path.of(args.single())
    val names =
      Files.walk(root).use { paths ->
        paths
          .filter { it.extension == "class" }
          .map { root.relativize(it).invariantSeparatorsPathString.removeSuffix(".class") }
          .toList()
      }
    val tests =
      names
        .map { Class.forName(it.replace('/', '.'), false, DeviceSuite::class.java.classLoader) }
        .filter { type ->
          !Modifier.isAbstract(type.modifiers) &&
            type.methods.any { it.isAnnotationPresent(Test::class.java) }
        }
    val junit = JUnitCore().apply { addListener(TextListener(System.out)) }
    exitProcess(if (junit.run(*tests.toTypedArray()).wasSuccessful()) 0 else 1)
  }
}
