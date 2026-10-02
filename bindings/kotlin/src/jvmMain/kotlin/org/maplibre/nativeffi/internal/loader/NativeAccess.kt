package org.maplibre.nativeffi.internal.loader

import java.nio.file.Path
import java.util.NoSuchElementException
import org.maplibre.nativeffi.internal.c.C

internal actual fun ensureNativeLibrary() = NativeAccess.ensureLoaded()

/** Ensures the native library is loaded before JVM FFM downcalls run. */
internal object NativeAccess {
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

  internal fun cVersion(): Long = Integer.toUnsignedLong(C.mln_c_version())

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
