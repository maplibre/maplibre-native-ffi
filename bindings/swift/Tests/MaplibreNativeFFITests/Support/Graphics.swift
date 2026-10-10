import Foundation
import GraphicsSupport
@testable import MaplibreNativeFFI
import Testing

/// A device or context from tests/graphics for the backend this build of the
/// native library provides. A build provides one native backend, so every
/// render test runs on it rather than being skipped.
final class TestGraphics: @unchecked Sendable {
  let backend: Int
  private let graphics: OpaquePointer
  private let context: mln_test_graphics_context

  init() throws {
    let backends = try Maplibre.supportedRenderBackendMask()
    backend = if backends.contains(.metal) {
      MLN_TEST_GRAPHICS_BACKEND_METAL
    } else if backends.contains(.vulkan) {
      MLN_TEST_GRAPHICS_BACKEND_VULKAN
    } else {
      MLN_TEST_GRAPHICS_BACKEND_EGL
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

  /// Makes an EGL context current on the calling thread, which then drives a
  /// shared session. Metal and Vulkan have no current context.
  func makeCurrentIfNeeded() throws {
    guard backend == MLN_TEST_GRAPHICS_BACKEND_EGL else { return }
    guard mln_test_graphics_make_current(graphics) else {
      throw FixtureError(String(cString: mln_test_graphics_last_error()))
    }
  }

  /// Attaches a session-owned texture target. A core-worker EGL session owns
  /// a context on the fixture's display, because a worker cannot share the
  /// host's; a caller-driven one joins the fixture context's share group.
  func attachOwnedTexture(
    map: MapHandle,
    options: RenderSessionAttachOptions,
    extent: RenderTargetExtent = RenderTargetExtent(
      width: 32,
      height: 32,
      scaleFactor: 1
    )
  ) throws -> RenderSessionAttachment {
    switch backend {
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      return try map.metalOwnedTextureAttach(
        descriptor: MetalOwnedTextureDescriptor(
          extent: extent,
          context: MetalContextDescriptor(device: pointer(context.metal_device))
        ),
        options: options
      )
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      return try map.vulkanOwnedTextureAttach(
        descriptor: VulkanOwnedTextureDescriptor(
          extent: extent,
          context: VulkanContextDescriptor(
            instance: pointer(context.vulkan_instance),
            physicalDevice: pointer(context.vulkan_physical_device),
            device: pointer(context.vulkan_device),
            graphicsQueue: pointer(context.vulkan_queue),
            graphicsQueueFamilyIndex: context.vulkan_queue_family_index,
            getInstanceProcAddr: pointer(context.vulkan_get_instance_proc_addr),
            getDeviceProcAddr: pointer(context.vulkan_get_device_proc_addr)
          )
        ),
        options: options
      )
    default:
      let dedicated = options.driver == .coreWorker
      return try map.openglOwnedTextureAttach(
        descriptor: OpenglOwnedTextureDescriptor(
          extent: extent,
          context: OpenglContextDescriptor(
            ownership: dedicated ? .dedicated : .shared,
            data: .egl(EglContextDescriptor(
              display: pointer(context.egl_display),
              config: pointer(context.egl_config),
              shareContext: dedicated ? .null : pointer(context.egl_context),
              clientApi: .gles
            ))
          )
        ),
        options: options
      )
    }
  }

  /// Opens the acquired frame's texture view for the build's backend.
  func withTextureView<Result>(
    of frame: AcquiredFrameHandle,
    _ body: (any OwnedTextureFrameView) throws -> Result
  ) throws -> Result {
    switch backend {
    case MLN_TEST_GRAPHICS_BACKEND_METAL:
      try frame.withMetalTexture { try body($0) }
    case MLN_TEST_GRAPHICS_BACKEND_VULKAN:
      try frame.withVulkanTexture { try body($0) }
    default:
      try frame.withOpenglTexture { try body($0) }
    }
  }

  private func pointer(_ address: UnsafeMutableRawPointer?) -> NativePointer {
    NativePointer(bitPattern: UInt(bitPattern: address))
  }
}

/// The part every backend's owned-texture frame view shares.
protocol OwnedTextureFrameView: AnyObject {
  var width: UInt32 { get throws }
  var height: UInt32 { get throws }
}

extension MetalOwnedTextureFrameView: OwnedTextureFrameView {}
extension VulkanOwnedTextureFrameView: OwnedTextureFrameView {}
extension OpenglOwnedTextureFrameView: OwnedTextureFrameView {}

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
  func drainFrameCopies() throws -> [RenderFrameResult] {
    guard let batch = try drainFrameResults() else { return [] }
    defer { try? batch.close() }
    return try batch.get().results
  }

  /// Demands frames until one renders, and returns its result. Each demand
  /// forces a frame, and a demand whose frame does not render, such as one
  /// before the style loads, is followed by another.
  @discardableResult
  func awaitRenderedFrame(
    sourceLocation: SourceLocation = #_sourceLocation
  ) async throws -> RenderFrameResult? {
    var pending: UInt64?
    var rendered: RenderFrameResult?
    try await awaitCondition(
      "a rendered frame",
      sourceLocation: sourceLocation
    ) {
      if pending == nil {
        let token = try getSnapshot().latestDemandToken + 1
        try requestFrame(demand: FrameDemand(flags: [], token: token))
        pending = token
      }
      for result in try drainFrameCopies() where result.token == pending {
        guard result.disposition == .rendered else {
          pending = nil
          continue
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

  /// Starts servicing `session` on this thread.
  func service(_ session: RenderSessionHandle) {
    condition.withLock {
      self.session = session
      workPending = true
      condition.signal()
    }
  }

  /// Runs `body` on this thread and returns its result.
  func perform<Value: Sendable>(
    _ body: @escaping @Sendable () throws -> Value
  ) async throws -> Value {
    try await withCheckedThrowingContinuation { continuation in
      condition.withLock {
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
        if target != nil { workPending = false }
        return (work, target, stopping && work.isEmpty)
      }
      if stop { break }
      for task in work {
        task()
      }
      if let target {
        _ = try? target.serviceDriverWork(maxWork: 0)
        Pulse.shared.signal()
      }
    }
    finished.signal()
  }
}
