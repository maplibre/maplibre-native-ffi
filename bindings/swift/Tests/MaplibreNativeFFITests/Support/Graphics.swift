import Foundation
import GraphicsSupport
@testable import MaplibreNativeFFI
import Testing
#if canImport(Metal)
  import Metal
#endif

/// A device or context from tests/graphics for the backend this build of the
/// native library provides. A build provides one native backend, so every
/// render test runs on it rather than being skipped.
final class TestGraphics: @unchecked Sendable {
  let backend: Int
  private let graphics: OpaquePointer
  private let context: mln_test_graphics_context

  init() throws {
    let backends = supportedRenderBackendMask()
    backend = if backends.contains(.metal) {
      MLN_TEST_GRAPHICS_BACKEND_METAL
    } else if backends.contains(.vulkan) {
      MLN_TEST_GRAPHICS_BACKEND_VULKAN
    } else {
      #if os(Windows)
        MLN_TEST_GRAPHICS_BACKEND_WGL
      #else
        MLN_TEST_GRAPHICS_BACKEND_EGL
      #endif
    }
    guard let graphics = mln_test_graphics_create(UInt32(backend)) else {
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
    var context = mln_test_graphics_context()
    guard mln_test_graphics_get_context(graphics, &context) else {
      mln_test_graphics_destroy(graphics)
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
    self.graphics = graphics
    self.context = context
  }

  deinit { mln_test_graphics_destroy(graphics) }

  /// Whether the backend is EGL or WGL, whose sessions need a thread with the
  /// context current.
  var usesCurrentContext: Bool {
    backend == MLN_TEST_GRAPHICS_BACKEND_EGL ||
      backend == MLN_TEST_GRAPHICS_BACKEND_WGL
  }

  /// Makes an EGL or WGL context current on the calling thread, which then
  /// drives a shared session. Metal and Vulkan have no current context.
  func makeCurrentIfNeeded() throws {
    guard usesCurrentContext else { return }
    guard mln_test_graphics_make_current(graphics) else {
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
  }

  /// Creates `count` textures of `width` x `height` for a borrowed ring. Call
  /// it before an EGL or WGL context is current on another thread.
  func createTextures(
    count: Int,
    width: UInt32 = 32,
    height: UInt32 = 32
  ) throws -> [TestTexture] {
    var textures: [TestTexture] = []
    do {
      for _ in 0 ..< count {
        guard let texture = mln_test_graphics_texture_create(
          graphics,
          width,
          height
        ) else {
          throw FixtureError(String(cString: mln_test_graphics_last_error()))
        }
        try textures.append(TestTexture(graphics: self, texture: texture))
      }
    } catch {
      textures.forEach { $0.destroy() }
      throw error
    }
    return textures
  }

  /// Attaches a session that renders into `textures`, a borrowed ring. An
  /// OpenGL session shares the fixture's context, so it needs a caller-driven
  /// session on a thread with that context current.
  func attachBorrowedTexture(
    map: MapHandle,
    textures: [TestTexture],
    options: RenderSessionAttachOptions
  ) throws -> NewRenderSession {
    guard let info = textures.last?.info else {
      throw FixtureError("a borrowed ring needs a texture")
    }
    let extent = LogicalExtent(
      width: info.width,
      height: info.height,
      scaleFactor: 1
    )
    switch backend {
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      #if canImport(Metal)
        return try map.attachMetalBorrowedTexture(
          descriptor: MetalBorrowedTextureDescriptor(
            extent: extent,
            physicalWidth: info.width,
            physicalHeight: info.height,
            textures: textures.map {
              MetalBorrowedTexture(texture: pointer($0.info.metal_texture))
            }
          ),
          options: options
        )
      #else
        throw FixtureError("a Metal backend needs the Metal framework")
      #endif
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      #if os(Android) || os(Linux) || os(macOS) || os(Windows)
        return try map.attachVulkanBorrowedTexture(
          descriptor: VulkanBorrowedTextureDescriptor(
            extent: extent,
            physicalWidth: info.width,
            physicalHeight: info.height,
            context: VulkanContextDescriptor(
              instance: pointer(context.vulkan_instance),
              physicalDevice: pointer(context.vulkan_physical_device),
              device: pointer(context.vulkan_device),
              graphicsQueue: pointer(context.vulkan_queue),
              graphicsQueueFamilyIndex: context.vulkan_queue_family_index,
              getInstanceProcAddr: pointer(context
                .vulkan_get_instance_proc_addr),
              getDeviceProcAddr: pointer(context.vulkan_get_device_proc_addr)
            ),
            textures: textures.map {
              VulkanBorrowedTexture(
                image: $0.info.vulkan_image,
                imageView: $0.info.vulkan_image_view
              )
            },
            format: info.format,
            finalLayout: info.vulkan_final_layout
          ),
          options: options
        )
      #else
        throw FixtureError(
          "a Vulkan backend needs Android, Linux, macOS, or Windows"
        )
      #endif
    case MLN_TEST_GRAPHICS_BACKEND_WGL:
      return try map.attachOpenGlBorrowedTexture(
        descriptor: OpenGlBorrowedTextureDescriptor(
          extent: extent,
          physicalWidth: info.width,
          physicalHeight: info.height,
          context: OpenGlContextDescriptor(
            ownership: .shared,
            data: .wgl(WglContextDescriptor(
              deviceContext: pointer(context.wgl_device_context),
              shareContext: pointer(context.wgl_context)
            ))
          ),
          textures: textures.map {
            OpenGlBorrowedTexture(texture: $0.info.opengl_texture)
          },
          target: info.opengl_target
        ),
        options: options
      )
    default:
      return try map.attachOpenGlBorrowedTexture(
        descriptor: OpenGlBorrowedTextureDescriptor(
          extent: extent,
          physicalWidth: info.width,
          physicalHeight: info.height,
          context: OpenGlContextDescriptor(
            ownership: .shared,
            data: .egl(EglContextDescriptor(
              display: pointer(context.egl_display),
              config: pointer(context.egl_config),
              shareContext: pointer(context.egl_context),
              clientApi: .gles
            ))
          ),
          textures: textures.map {
            OpenGlBorrowedTexture(texture: $0.info.opengl_texture)
          },
          target: info.opengl_target
        ),
        options: options
      )
    }
  }

  func withTextureView<Result>(
    of frame: AcquiredFrameHandle,
    _ body: (any TextureFrameView) throws -> Result
  ) throws -> Result {
    switch backend {
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      #if canImport(Metal)
        try frame.withMetalTexture { try body($0) }
      #else
        throw FixtureError("a Metal backend needs the Metal framework")
      #endif
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      #if os(Android) || os(Linux) || os(macOS) || os(Windows)
        try frame.withVulkanTexture { try body($0) }
      #else
        throw FixtureError(
          "a Vulkan backend needs Android, Linux, macOS, or Windows"
        )
      #endif
    default:
      try frame.withOpenGlTexture { try body($0) }
    }
  }

  #if canImport(Metal)
    /// The fixture's Metal device, which the fixture owns.
    var metalDevice: any MTLDevice {
      Unmanaged<AnyObject>.fromOpaque(context.metal_device!)
        .takeUnretainedValue() as! any MTLDevice
    }
  #endif

  private func pointer(_ address: UnsafeMutableRawPointer?) -> NativePointer {
    NativePointer(bitPattern: UInt(bitPattern: address))
  }
}

/// The part every backend's texture frame view shares.
protocol TextureFrameView: AnyObject {
  var width: UInt32 { get }
  var height: UInt32 { get }
  /// Reads the texture object that the frame lends, which the view's scope
  /// guards.
  func readTexture() throws
}

extension MetalTextureFrameView: TextureFrameView {
  func readTexture() throws {
    _ = try texture
  }
}

extension VulkanTextureFrameView: TextureFrameView {
  func readTexture() throws {
    _ = try image
  }
}

extension OpenGlTextureFrameView: TextureFrameView {
  func readTexture() throws {
    _ = try texture
  }
}

/// A texture from tests/graphics that a borrowed ring names. Destroy it once
/// the session that renders into it has retired. It keeps its graphics
/// object alive.
final class TestTexture: @unchecked Sendable {
  let info: mln_test_graphics_texture_info
  private let graphics: TestGraphics
  private var texture: OpaquePointer?

  fileprivate init(graphics: TestGraphics, texture: OpaquePointer) throws {
    var info = mln_test_graphics_texture_info()
    guard mln_test_graphics_texture_get_info(texture, &info) else {
      mln_test_graphics_texture_destroy(texture)
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
    self.info = info
    self.graphics = graphics
    self.texture = texture
  }

  /// Destroys the texture. An EGL or WGL texture needs its context current on
  /// the calling thread or on none. A second call does nothing.
  func destroy() {
    mln_test_graphics_texture_destroy(texture)
    texture = nil
  }
}

struct FixtureError: Error, CustomStringConvertible {
  let description: String

  init(_ description: String) {
    self.description = description
  }
}

/// A frame wake that pulses, so frame waits wake as soon as a result is
/// drainable.
let pulsingFrameWake = Wake(callback: { Pulse.shared.signal() })

extension RenderSessionHandle {
  /// Demands frames until one renders, and returns its result. Each demand
  /// forces a frame, and a demand whose frame does not render, such as one
  /// before the style loads, is followed at once by another, whose result
  /// pulses the wait.
  @discardableResult
  func awaitRenderedFrame(
    sourceLocation: SourceLocation = #_sourceLocation
  ) async throws -> RenderFrameResult? {
    var token: UInt64 = 1
    var rendered: RenderFrameResult?
    try requestFrame(demand: FrameDemand(flags: [.force], token: token))
    try await awaitCondition(
      "a rendered frame",
      sourceLocation: sourceLocation
    ) {
      for result in try drainFrameResults() where result.token == token {
        guard result.disposition == .rendered else {
          token += 1
          try requestFrame(demand: FrameDemand(flags: [.force], token: token))
          return false
        }
        rendered = result
        return true
      }
      return false
    }
    return rendered
  }
}

/// A host render thread for a caller-driven session: a thread the test owns
/// that services the session's driver work whenever its driver-work wake
/// fires, and runs the host graphics calls the test hands it.
final class RenderThread: @unchecked Sendable {
  private let condition = NSCondition()
  private var tasks: [() -> Void] = []
  private var session: RenderSessionHandle?
  private var workPending = false
  private var stopping = false
  private let finished = DispatchSemaphore(value: 0)

  init() {
    let thread = Thread { [self] in run() }
    thread.name = "test render thread"
    thread.start()
  }

  /// The wake to attach with. It may fire before ``service(_:)`` names the
  /// session, and the thread services the session once it has one.
  var driverWorkWake: Wake {
    Wake(callback: { [weak self] in self?.notifyWork() })
  }

  func service(_ session: RenderSessionHandle) {
    condition.withLock {
      self.session = session
      workPending = true
      condition.signal()
    }
  }

  /// Runs `body` on this thread and returns its result. Throws once the
  /// thread has stopped.
  func perform<Value: Sendable>(
    _ body: @escaping @Sendable () throws -> Value
  ) async throws -> Value {
    try await withCheckedThrowingContinuation { continuation in
      condition.withLock {
        guard !stopping else {
          continuation.resume(
            throwing: FixtureError("the render thread has stopped")
          )
          return
        }
        tasks.append { continuation.resume(with: Result { try body() }) }
        condition.signal()
      }
    }
  }

  /// Stops servicing and waits for the thread to end. A second call does
  /// nothing.
  func stop() {
    let first = condition.withLock {
      defer { stopping = true }
      condition.signal()
      return !stopping
    }
    guard first else { return }
    #expect(isSignalled(finished))
  }

  private func notifyWork() {
    condition.withLock {
      workPending = true
      condition.signal()
    }
    Pulse.shared.signal()
  }

  private func run() {
    while true {
      let (work, target, stop) = condition.withLock {
        while !stopping, tasks.isEmpty, !(workPending && session != nil) {
          condition.wait()
        }
        let work = tasks
        tasks.removeAll()
        let target = workPending ? session : nil
        if target != nil {
          workPending = false
        }
        return (work, target, stopping && work.isEmpty)
      }
      if stop {
        break
      }
      for task in work {
        task()
      }
      if let target {
        _ = try? target.serviceDriverWork()
        Pulse.shared.signal()
      }
    }
    finished.signal()
  }
}
