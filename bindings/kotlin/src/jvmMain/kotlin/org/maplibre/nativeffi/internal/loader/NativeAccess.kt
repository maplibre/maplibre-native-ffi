package org.maplibre.nativeffi.internal.loader

import java.lang.foreign.MemorySegment
import java.nio.file.Path
import java.util.NoSuchElementException
import org.maplibre.nativeffi.error.AbiVersionMismatchException
import org.maplibre.nativeffi.internal.c.MapLibreNativeC
import org.maplibre.nativeffi.internal.c.mln_completion_result

/** Ensures the native library is loaded before JVM FFM downcalls run. */
internal object NativeAccess {
  fun pluginRegisterFunctionV1(): Long =
    MapLibreNativeC.mln_plugin_get_register_function_v1().address()

  const val EXPECTED_C_ABI_VERSION: Long = 0L
  private val lock = Any()

  @Volatile private var initialized = false

  fun ensureLoaded() {
    if (initialized) {
      return
    }

    synchronized(lock) {
      if (initialized) {
        return
      }

      NativeLibrary.load()
      checkNativeAccessAndAbi()
      initialized = true
    }
  }

  fun load(libraryPath: Path) {
    synchronized(lock) {
      NativeLibrary.load(libraryPath)
      checkNativeAccessAndAbi()
      initialized = true
    }
  }

  internal fun checkAbiVersion(version: Long) {
    if (version != EXPECTED_C_ABI_VERSION) {
      throw AbiVersionMismatchException(version, EXPECTED_C_ABI_VERSION)
    }
  }

  internal fun checkNativeAccessAndAbi(cVersion: () -> Long) {
    val version =
      try {
        cVersion()
      } catch (error: ExceptionInInitializerError) {
        val cause = deepestCause(error)
        if (cause is IllegalCallerException) {
          throw nativeAccessFailure(cause)
        }
        if (cause is NoSuchElementException || cause is UnsatisfiedLinkError) {
          throw missingSymbols(error)
        }
        throw error
      } catch (error: IllegalCallerException) {
        throw nativeAccessFailure(error)
      } catch (error: NoSuchElementException) {
        throw missingSymbols(error)
      } catch (error: UnsatisfiedLinkError) {
        throw missingSymbols(error)
      }

    checkAbiVersion(version)
  }

  private fun checkNativeAccessAndAbi() {
    checkNativeAccessAndAbi(::cVersion)
  }

  internal fun cVersion(): Long = Integer.toUnsignedLong(MapLibreNativeC.mln_c_version())

  /** Borrows the completion payload during its callback. */
  internal fun completionValue(result: MemorySegment, byteSize: Long): MemorySegment {
    val value = mln_completion_result.value(result)
    check(value != MemorySegment.NULL) { "native completion omitted its result value" }
    return value.reinterpret(byteSize)
  }

  private fun nativeAccessFailure(cause: Throwable): IllegalStateException =
    IllegalStateException(
      "Java FFM native access is not enabled. Run the JVM with " +
        "--enable-native-access=ALL-UNNAMED for this classpath build.",
      cause,
    )

  private fun missingSymbols(cause: Throwable): UnsatisfiedLinkError {
    val missing =
      UnsatisfiedLinkError("Loaded native library does not expose the Maplibre C ABI symbols.")
    missing.addSuppressed(cause)
    return missing
  }

  private fun deepestCause(error: Throwable): Throwable {
    var current = error
    while (current.cause != null) {
      current = current.cause!!
    }
    return current
  }
}
