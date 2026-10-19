#if canImport(Darwin)
  import Darwin
#elseif canImport(Glibc)
  import Glibc
#endif
import Foundation

/// An HTTP server on the loopback interface that answers each GET with the
/// body for its path, or 404, and records the paths it was asked for.
/// Resource transforms apply only on the built-in HTTP stack, so a transform
/// test needs a server it can see requests reach.
final class LoopbackServer: @unchecked Sendable {
  let port: UInt16

  private let socket: Int32
  private let bodies: [String: Data]
  private let paths = LockedBox<[String]>([])
  private let stopped = LockedBox(false)
  private let finished = DispatchSemaphore(value: 0)

  init(bodies: [String: Data]) throws {
    self.bodies = bodies
    #if canImport(Glibc)
      let fd = Glibc.socket(AF_INET, Int32(SOCK_STREAM.rawValue), 0)
    #else
      let fd = Darwin.socket(AF_INET, SOCK_STREAM, 0)
    #endif
    guard fd >= 0 else { throw LoopbackServerError.socket(errno) }
    var address = sockaddr_in()
    #if canImport(Darwin)
      address.sin_len = UInt8(MemoryLayout<sockaddr_in>.size)
    #endif
    address.sin_family = sa_family_t(AF_INET)
    address.sin_addr.s_addr = inet_addr("127.0.0.1")
    var length = socklen_t(MemoryLayout<sockaddr_in>.size)
    let bound = withUnsafeMutablePointer(to: &address) {
      $0.withMemoryRebound(to: sockaddr.self, capacity: 1) {
        bind(fd, $0, length) == 0 && listen(fd, 16) == 0
          && getsockname(fd, $0, &length) == 0
      }
    }
    guard bound else {
      let error = errno
      close(fd)
      throw LoopbackServerError.socket(error)
    }
    socket = fd
    port = UInt16(bigEndian: address.sin_port)
    Thread { [self] in serve() }.start()
  }

  /// The server's URL for `path`, which starts with a slash.
  func url(_ path: String) -> String {
    "http://127.0.0.1:\(port)\(path)"
  }

  /// The paths requested so far, in arrival order.
  var requestedPaths: [String] {
    paths.value
  }

  /// Stops accepting connections and closes the socket.
  func stop() {
    stopped.update { $0 = true }
    finished.wait()
    close(socket)
  }

  private func serve() {
    defer { finished.signal() }
    while !stopped.value {
      var ready = pollfd(fd: socket, events: Int16(POLLIN), revents: 0)
      guard poll(&ready, 1, 20) > 0 else { continue }
      let client = accept(socket, nil, nil)
      guard client >= 0 else { continue }
      respond(on: client)
      close(client)
    }
  }

  private func respond(on client: Int32) {
    var timeout = timeval(tv_sec: 5, tv_usec: 0)
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, &timeout,
               socklen_t(MemoryLayout<timeval>.size))
    #if canImport(Darwin)
      var one: Int32 = 1
      setsockopt(client, SOL_SOCKET, SO_NOSIGPIPE, &one,
                 socklen_t(MemoryLayout<Int32>.size))
      let sendFlags: Int32 = 0
    #else
      let sendFlags = Int32(MSG_NOSIGNAL)
    #endif

    var request = Data()
    var buffer = [UInt8](repeating: 0, count: 4096)
    while request.range(of: Data("\r\n\r\n".utf8)) == nil {
      let count = recv(client, &buffer, buffer.count, 0)
      guard count > 0 else { return }
      request.append(contentsOf: buffer[..<count])
    }
    let line = String(decoding: request, as: UTF8.self)
      .prefix { $0 != "\r" }
      .split(separator: " ")
    guard line.count >= 2 else { return }
    let path = String(line[1])
    paths.update { $0.append(path) }

    let body = bodies[path]
    let status = body == nil ? "404 Not Found" : "200 OK"
    let head = [
      "HTTP/1.1 \(status)",
      "Content-Type: application/json",
      "Content-Length: \(body?.count ?? 0)",
      "Connection: close",
      "",
      "",
    ]
    var response = Data(head.joined(separator: "\r\n").utf8)
    response.append(body ?? Data())
    response.withUnsafeBytes { bytes in
      var sent = 0
      while sent < bytes.count {
        let count = send(client, bytes.baseAddress! + sent, bytes.count - sent,
                         sendFlags)
        guard count > 0 else { return }
        sent += count
      }
    }
  }
}

enum LoopbackServerError: Error {
  case socket(Int32)
}
