// Stands in for the BuildConfig that the verifier's Gradle module generates. The
// helper reads TEST to enable mock roots, which only the upstream tests use.
package org.maplibre.nativeffi.internal.rustlsplatformverifier

internal object BuildConfig {
  const val TEST = false
}
