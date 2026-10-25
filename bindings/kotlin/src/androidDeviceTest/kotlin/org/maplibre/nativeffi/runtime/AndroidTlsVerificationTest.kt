package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.IOException
import java.net.InetAddress
import java.security.KeyStore
import javax.net.ssl.KeyManagerFactory
import javax.net.ssl.SSLContext
import javax.net.ssl.SSLServerSocket
import javax.net.ssl.SSLSocket
import kotlin.concurrent.thread
import kotlin.test.Test
import kotlin.test.assertContains
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.generated.ResourceRoute
import org.maplibre.nativeffi.generated.RuntimeEventSourceType
import org.maplibre.nativeffi.generated.RuntimeEventType
import org.maplibre.nativeffi.generated.UrlMatchFlag
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.withMap

/**
 * Native HTTPS verifies through the platform trust manager, which the library reaches through the
 * Java helper that it embeds and loads itself. The test APK carries no copy of the helper, so only
 * the embedded one can report the trust manager's UnknownIssuer.
 */
class AndroidTlsVerificationTest {
  @Test
  fun aCertificateFromAnUntrustedIssuerFailsPlatformVerification(): Unit = runSuspendTest {
    UntrustedTlsServer().use { server ->
      // The provider claims plain HTTP alone, so the HTTPS request reaches the native client.
      val provider =
        denyingProvider(listOf(ResourceRoute(flags = UrlMatchFlag.GLOB, url = "http://**")))
      withMap(provider = provider) {
        map.setStyleUrl("https://$HOST:${server.port}/style.json").awaitWithin("the style command")
        val failure =
          runtimeFixture.awaitEvent("the style failure") {
            it.sourceType == RuntimeEventSourceType.MAP &&
              it.type == RuntimeEventType.MAP_LOADING_FAILED
          }
        assertContains(failure.message, "UnknownIssuer")
      }
    }
  }
}

/** A loopback TLS server whose self-signed certificate no trusted root issued. */
private class UntrustedTlsServer : AutoCloseable {
  private val socket: SSLServerSocket
  private val acceptor: Thread

  val port: Int
    get() = socket.localPort

  init {
    val keyStore = KeyStore.getInstance("PKCS12")
    InstrumentationRegistry.getInstrumentation().context.assets.open("untrusted-tls.p12").use {
      keyStore.load(it, PASSWORD)
    }
    val keyManagers =
      KeyManagerFactory.getInstance(KeyManagerFactory.getDefaultAlgorithm())
        .apply { init(keyStore, PASSWORD) }
        .keyManagers
    val context = SSLContext.getInstance("TLS").apply { init(keyManagers, null, null) }
    // Android's loopback address is ::1, and the request and certificate name 127.0.0.1.
    socket =
      context.serverSocketFactory.createServerSocket(0, 1, InetAddress.getByName(HOST))
        as SSLServerSocket
    // The client rejects the certificate, which ends each handshake with an error here.
    acceptor =
      thread(name = "untrusted-tls-server") {
        while (true) {
          val client = runCatching { socket.accept() as SSLSocket }.getOrNull() ?: break
          client.use {
            try {
              it.startHandshake()
            } catch (_: IOException) {}
          }
        }
      }
  }

  override fun close() {
    socket.close()
    acceptor.join()
  }
}

private const val HOST = "127.0.0.1"
private val PASSWORD = "fixture".toCharArray()
