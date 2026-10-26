package org.maplibre.nativeffi.runtime

import androidx.test.platform.app.InstrumentationRegistry
import java.io.ByteArrayOutputStream
import java.io.IOException
import java.io.InputStream
import java.net.InetAddress
import java.net.ServerSocket
import java.net.Socket
import java.util.concurrent.CopyOnWriteArrayList
import java.util.concurrent.CountDownLatch
import java.util.concurrent.TimeUnit
import java.util.concurrent.locks.ReentrantLock
import java.util.zip.GZIPOutputStream
import kotlin.concurrent.thread
import kotlin.concurrent.withLock
import kotlin.test.Test
import kotlin.test.assertContains
import kotlin.test.assertEquals
import kotlin.test.assertNull
import kotlin.test.assertTrue
import org.maplibre.nativeffi.EMPTY_STYLE_JSON
import org.maplibre.nativeffi.HttpHeader
import org.maplibre.nativeffi.HttpHeaderRule
import org.maplibre.nativeffi.HttpHeaderTransform
import org.maplibre.nativeffi.MapFixture
import org.maplibre.nativeffi.ResourceProvider
import org.maplibre.nativeffi.ResourceRoute
import org.maplibre.nativeffi.RuntimeEventSourceType
import org.maplibre.nativeffi.RuntimeEventType
import org.maplibre.nativeffi.UrlMatchFlag
import org.maplibre.nativeffi.awaitWithin
import org.maplibre.nativeffi.denyingProvider
import org.maplibre.nativeffi.render.withOwnedTexture
import org.maplibre.nativeffi.runSuspendTest
import org.maplibre.nativeffi.runThenRelease
import org.maplibre.nativeffi.setHttpHeaderTransform
import org.maplibre.nativeffi.smallMapOptions
import org.maplibre.nativeffi.withMap

/**
 * Android's HTTP client, which calls Java and so runs here rather than in the C suite, whose test
 * executables have no Java VM. The cases mirror tests/native/abi/platform/transport.c.
 */
class AndroidHttpTransportTest {
  @Test
  fun eachStatusReachesTheMapAsTheStatusSays(): Unit = runSuspendTest {
    LoopbackServer(
        mapOf(
          "/ok.json" to Route(body = EMPTY_STYLE_JSON.encodeToByteArray()),
          "/server-error.json" to Route(status = 500),
          "/found.json" to Route(status = 302, headers = mapOf("Location" to "./ok.json")),
          "/to-missing.json" to Route(status = 302, headers = mapOf("Location" to "/missing.json")),
          "/gzipped.json" to
            Route(headers = mapOf("Content-Encoding" to "gzip"), body = gzip(EMPTY_STYLE_JSON)),
        ),
      )
      .use { server ->
        loadStyleUrl(server.url("/ok.json"))
        assertContains(loadingFailure(server.url("/missing.json")), "404")
        assertContains(loadingFailure(server.url("/server-error.json")), "500")

        val okRequests = server.requests("/ok.json").size
        loadStyleUrl(server.url("/found.json"))
        assertEquals(okRequests + 1, server.requests("/ok.json").size)

        val missingRequests = server.requests("/missing.json").size
        assertContains(loadingFailure(server.url("/to-missing.json")), "404")
        assertEquals(missingRequests + 1, server.requests("/missing.json").size)

        loadStyleUrl(server.url("/gzipped.json"))
      }
  }

  @Test
  fun aRefusedConnectionFailsTheLoad(): Unit = runSuspendTest {
    // A port that a server just gave up, so nothing else is listening there.
    val url = LoopbackServer(emptyMap()).use { it.url("/refused.json") }
    loadingFailure(url)
  }

  @Test
  fun aNotModifiedResponseRevalidatesTheCachedStyle(): Unit = runSuspendTest {
    val route =
      Route(
        headers = mapOf("Cache-Control" to "max-age=0, must-revalidate"),
        body = EMPTY_STYLE_JSON.encodeToByteArray(),
        etag = "\"e1\"",
      )
    LoopbackServer(mapOf("/revalidated.json" to route)).use { server ->
      val url = server.url("/revalidated.json")
      // The default cache is in memory and lives only while something holds it, so the first map
      // stays until the second has loaded.
      withMap(provider = nativeHttp()) {
        map.setStyleUrl(url).awaitWithin("the first style command")
        awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
        assertNull(server.requests("/revalidated.json")[0].headers["if-none-match"])

        val (second, ready) = runtime.createMap(smallMapOptions())
        runThenRelease(listOf(second)) {
          ready.awaitWithin("the second map")
          second.setStyleUrl(url).awaitWithin("the second style command")
          awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
        }
        assertEquals("\"e1\"", server.requests("/revalidated.json")[1].headers["if-none-match"])
      }
    }
  }

  @Test
  fun aRemotePmtilesArchiveIsReadInRanges(): Unit = runSuspendTest {
    val archive =
      InstrumentationRegistry.getInstrumentation().context.assets.open("range-check.pmtiles").use {
        it.readBytes()
      }
    LoopbackServer(mapOf("/archive.pmtiles" to Route(body = archive, servesRanges = true))).use {
      server ->
      // A static map renders its still image only once every source has loaded, and a PMTiles
      // source loads only when its ranged reads of the archive's header and metadata succeed.
      withOwnedTexture(width = 64, height = 64, provider = nativeHttp()) {
        setStyle(
          """{"version":8,"sources":{"tiles":{"type":"vector","url":"pmtiles://${server.url("/archive.pmtiles")}"}},"layers":[]}""",
        )
        renderStill()
      }
      val requests = server.requests("/archive.pmtiles")
      assertTrue(requests.size >= 2, "the archive was read ${requests.size} times")
      for (request in requests) {
        assertTrue(request.headers["range"].orEmpty().startsWith("bytes="), "$request")
        // A range of a compressed body is not a range of the archive.
        assertEquals("identity", request.headers["accept-encoding"], "$request")
      }
    }
  }

  @Test
  fun aRequestInFlightDoesNotHoldItsMapOpen(): Unit = runSuspendTest {
    LoopbackServer(
        mapOf("/held.json" to Route(body = EMPTY_STYLE_JSON.encodeToByteArray(), held = true)),
      )
      .use { server ->
        withMap(provider = nativeHttp()) {
          map.setStyleUrl(server.url("/held.json")).awaitWithin("the style command")
          server.awaitRequest("/held.json")
          map.release().awaitWithin("the release of the map")
        }
      }
  }

  @Test
  fun aStalledRequestDoesNotHoldBackAnother(): Unit = runSuspendTest {
    val geojson = """{"type":"FeatureCollection","features":[]}""".encodeToByteArray()
    LoopbackServer(
        mapOf(
          "/held.json" to Route(body = geojson, held = true),
          "/answered.json" to Route(body = geojson),
        ),
      )
      .use { server ->
        // The style's request leaves a thread idle, and the map then asks for both sources at
        // once, the stalled one first.
        val style =
          """{"version":8,"sources":{""" +
            """"held":{"type":"geojson","data":"${server.url("/held.json")}"},""" +
            """"answered":{"type":"geojson","data":"${server.url("/answered.json")}"}""" +
            """},"layers":[]}"""
        LoopbackServer(mapOf("/style.json" to Route(body = style.encodeToByteArray()))).use {
          styleServer ->
          withMap(provider = nativeHttp()) {
            map.setStyleUrl(styleServer.url("/style.json")).awaitWithin("the style command")
            server.awaitRequest("/held.json")
            server.awaitRequest("/answered.json")
          }
        }
      }
  }

  @Test
  fun aRedirectKeepsTheTransformHeadersOnlyWithinItsOrigin(): Unit = runSuspendTest {
    LoopbackServer(mapOf("/style.json" to Route(body = EMPTY_STYLE_JSON.encodeToByteArray())))
      .use { other ->
        LoopbackServer(
            mapOf(
              "/same.json" to Route(status = 302, headers = mapOf("Location" to "/style.json")),
              "/style.json" to Route(body = EMPTY_STYLE_JSON.encodeToByteArray()),
              // Another port is another origin.
              "/cross.json" to
                Route(status = 302, headers = mapOf("Location" to other.url("/style.json"))),
            ),
          )
          .use { origin ->
            val token = HttpHeader(TOKEN_HEADER, "secret")
            val rule =
              HttpHeaderRule(flags = UrlMatchFlag.GLOB, url = "http://**", headers = listOf(token))
            withMap(provider = nativeHttp()) {
              runtime
                .setHttpHeaderTransform(HttpHeaderTransform(rules = listOf(rule)))
                .awaitWithin("the header transform")
              loadStyleUrl(origin.url("/same.json"))
              loadStyleUrl(origin.url("/cross.json"))
            }
            assertEquals("secret", origin.requests("/same.json").single().headers[TOKEN_KEY])
            assertEquals("secret", origin.requests("/style.json").single().headers[TOKEN_KEY])
            assertEquals("secret", origin.requests("/cross.json").single().headers[TOKEN_KEY])
            assertNull(other.requests("/style.json").single().headers[TOKEN_KEY])
          }
      }
  }

  @Test
  fun plainHttpFollowsTheNetworkSecurityConfiguration(): Unit = runSuspendTest {
    LoopbackServer(mapOf("/ok.json" to Route(body = EMPTY_STYLE_JSON.encodeToByteArray()))).use {
      server ->
      // The configuration permits plain HTTP to 127.0.0.1 alone.
      loadStyleUrl(server.url("/ok.json"))
      val refused = server.url("/ok.json").replace(HOST, "localhost")
      // Android words the refusal differently across versions.
      assertContains(loadingFailure(refused), "cleartext", ignoreCase = true)
      assertEquals(1, server.requests("/ok.json").size)
    }
  }

  private suspend fun loadStyleUrl(url: String) {
    withMap(provider = nativeHttp()) { loadStyleUrl(url) }
  }

  private suspend fun MapFixture.loadStyleUrl(url: String) {
    map.setStyleUrl(url).awaitWithin("the style command for $url")
    awaitMapEvent(RuntimeEventType.MAP_STYLE_LOADED)
  }

  /** Loads [url], which must fail, and returns the failure's message. */
  private suspend fun loadingFailure(url: String): String =
    withMap(provider = nativeHttp()) {
      map.setStyleUrl(url).awaitWithin("the style command for $url")
      runtimeFixture
        .awaitEvent("the loading failure for $url") {
          it.sourceType == RuntimeEventSourceType.MAP &&
            it.type == RuntimeEventType.MAP_LOADING_FAILED
        }
        .message
    }
}

/** Claims only `asset:` requests, so every HTTP request reaches the native client. */
private fun nativeHttp(): ResourceProvider =
  denyingProvider(listOf(ResourceRoute(flags = UrlMatchFlag.GLOB, url = "asset://**")))

private fun gzip(text: String): ByteArray =
  ByteArrayOutputStream()
    .also { GZIPOutputStream(it).use { gz -> gz.write(text.encodeToByteArray()) } }
    .toByteArray()

/**
 * One path's answer. A [held] answer waits until its server closes. A route with an [etag] answers
 * 304 to a request that names it.
 */
private class Route(
  val status: Int = 200,
  val headers: Map<String, String> = emptyMap(),
  val body: ByteArray = ByteArray(0),
  val etag: String? = null,
  val servesRanges: Boolean = false,
  val held: Boolean = false,
)

/** A request's path, without its query, and its headers under lowercase names. */
private data class Request(val path: String, val headers: Map<String, String>)

/**
 * A plain HTTP/1.1 server on 127.0.0.1 that answers from [routes], with 404 for any other path,
 * closes each connection after one answer, and records every request.
 */
private class LoopbackServer(private val routes: Map<String, Route>) : AutoCloseable {
  private val socket = ServerSocket(0, 50, InetAddress.getByName(HOST))
  private val recorded = CopyOnWriteArrayList<Request>()
  private val closing = CountDownLatch(1)
  private val lock = ReentrantLock()
  private val arrived = lock.newCondition()
  private val acceptor =
    thread(name = "loopback-http-server") {
      while (true) {
        val client = runCatching { socket.accept() }.getOrNull() ?: break
        thread(name = "loopback-http-connection") { client.use(::answer) }
      }
    }

  val port: Int = socket.localPort

  fun url(path: String): String = "http://$HOST:$port$path"

  fun requests(path: String): List<Request> = recorded.filter { it.path == path }

  /** Waits for a request for [path]. */
  fun awaitRequest(path: String) {
    lock.withLock {
      var remaining = TimeUnit.SECONDS.toNanos(10)
      while (requests(path).isEmpty()) {
        check(remaining > 0) { "timed out waiting for a request for $path" }
        remaining = arrived.awaitNanos(remaining)
      }
    }
  }

  override fun close() {
    closing.countDown()
    socket.close()
    acceptor.join()
  }

  private fun answer(client: Socket) {
    try {
      val input = client.getInputStream()
      val target = readLine(input)?.split(' ')?.getOrNull(1) ?: return
      val headers = mutableMapOf<String, String>()
      while (true) {
        val line = readLine(input) ?: return
        if (line.isEmpty()) break
        val colon = line.indexOf(':')
        if (colon > 0) {
          headers[line.substring(0, colon).trim().lowercase()] = line.substring(colon + 1).trim()
        }
      }
      val request = Request(target.substringBefore('?'), headers)
      lock.withLock {
        recorded.add(request)
        arrived.signalAll()
      }

      val route = routes[request.path]
      if (route?.held == true) closing.await()
      var status = route?.status ?: 404
      var body = route?.body ?: ByteArray(0)
      val extra = route?.headers.orEmpty().toMutableMap()
      route?.etag?.let { etag ->
        extra["ETag"] = etag
        if (headers["if-none-match"] == etag) {
          status = 304
          body = ByteArray(0)
        }
      }
      val range = headers["range"]?.let(RANGE::matchEntire)
      if (route != null && route.servesRanges && range != null && status == 200) {
        val first = range.groupValues[1].toInt()
        val last =
          (range.groupValues[2].toIntOrNull() ?: (body.size - 1)).coerceAtMost(body.size - 1)
        extra["Content-Range"] = "bytes $first-$last/${body.size}"
        body = body.copyOfRange(first, last + 1)
        status = 206
      }

      val head = buildString {
        append("HTTP/1.1 $status Status\r\n")
        append("Content-Length: ${body.size}\r\n")
        append("Connection: close\r\n")
        for ((name, value) in extra) append("$name: $value\r\n")
        append("\r\n")
      }
      val output = client.getOutputStream()
      output.write(head.toByteArray(Charsets.ISO_8859_1))
      output.write(body)
      output.flush()
    } catch (_: IOException) {
      // The client went away first, as a canceled request does.
    }
  }

  private fun readLine(input: InputStream): String? {
    val line = ByteArrayOutputStream()
    while (true) {
      val byte = input.read()
      if (byte < 0) return null
      if (byte == '\n'.code) break
      if (byte != '\r'.code) line.write(byte)
    }
    return line.toString(Charsets.ISO_8859_1.name())
  }
}

private const val HOST = "127.0.0.1"
private const val TOKEN_HEADER = "X-Mln-Token"
private const val TOKEN_KEY = "x-mln-token"
private val RANGE = Regex("bytes=(\\d+)-(\\d*)")
