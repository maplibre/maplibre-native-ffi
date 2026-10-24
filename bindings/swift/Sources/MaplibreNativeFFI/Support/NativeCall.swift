internal import MaplibreNativeC
import Foundation

// Copy borrowed outputs while their storage is held. Input storage and callback
// registrations stay live until the native call settles.

protocol NativeReceiver: AnyObject {
  associatedtype Native: NativeHandle
  var handle: NativeHandleState<Native> { get }
}

extension NativeReceiver {
  /// Disposes the handle of an owner that the binding created but no caller
  /// received, without reporting a leak.
  func retireUnreceived() {
    handle.retire()
  }
}

/// A native call that reports a status into a diagnostic. It receives the
/// receiver's raw id, or 0 without a handle receiver, and the input arena.
typealias NativeStatusCall = (
  UInt64,
  NativeInputArena,
  UnsafeMutablePointer<mln_diagnostic>
) throws -> mln_status

typealias NativeStartCall = (
  UInt64,
  NativeInputArena,
  UnsafePointer<mln_completion>,
  UnsafeMutablePointer<mln_diagnostic>
) throws -> mln_status

extension NativeReceiver {
  private func withNative<T>(
    _ body: (UInt64, NativeInputArena) throws -> T
  ) throws -> T {
    defer { withExtendedLifetime(self) {} }
    let arena = NativeInputArena()
    defer { withExtendedLifetime(arena) {} }
    return try handle.withHandle { try body($0.raw, arena) }
  }

  func nativeInvoke<T>(
    _ call: NativeStatusCall,
    result: () throws -> T
  ) throws -> T {
    try withNative { raw, arena in
      try checkedCall(call, raw, arena)
      return try result()
    }
  }

  func nativeInvoke(
    _ call: NativeStatusCall
  ) throws {
    try nativeInvoke(call) { () }
  }

  /// Calls a status-returning function like `nativeInvoke`, except that
  /// `absent` reports that native published no output, and returns nil.
  func nativeInvoke<T>(
    absentOn absent: mln_status,
    _ call: NativeStatusCall,
    result: () throws -> T
  ) throws -> T? {
    var present = true
    return try nativeInvoke({ raw, arena, diagnostic in
      let status = try call(raw, arena, diagnostic)
      guard status == absent else { return status }
      present = false
      return MLN_STATUS_OK
    }) { present ? try result() : nil }
  }

  func nativeStart<T: Sendable>(
    convert: @escaping (UnsafePointer<mln_completion_result>) throws -> T,
    _ call: NativeStartCall
  ) async throws -> T {
    try await withNative { raw, arena in
      try NativeCompletion.start({ completion, diagnostic in
        try arena.submit { try call(raw, arena, completion, diagnostic) }
      }, convert: convert)
    }.value()
  }

  func nativeStart<Raw, T: Sendable>(
    copying copy: @escaping (Raw) throws -> T,
    _ call: NativeStartCall
  ) async throws -> T {
    try await nativeStart(convert: { result in
      try copy(NativeCompletion.value(result))
    }, call)
  }

  func nativeUnit(
    _ call: NativeStartCall
  ) async throws {
    try await nativeStart(convert: { _ in () }, call)
  }

  /// Starts a command and awaits its commit. A failed or cancelled command
  /// throws `MaplibreError`.
  @discardableResult
  func nativeCommand(
    _ call: NativeStartCall
  ) async throws -> CommandCompletion {
    try await withNative { raw, arena in
      try NativeCompletion.startCommand { completion, diagnostic in
        try arena.submit { try call(raw, arena, completion, diagnostic) }
      }
    }.value()
  }

  /// Lends `body` a value that the receiver lends only inside a native view
  /// scope. `begin` opens the scope and `end` closes it, `get` fills `raw`, and
  /// `body` receives the filled value with the binding scope that expires when
  /// `body` returns.
  func nativeView<Raw, T>(
    reading raw: Raw,
    begin: (
      UInt64,
      UnsafeMutablePointer<UnsafeMutableRawPointer?>,
      UnsafeMutablePointer<mln_diagnostic>
    ) -> mln_status,
    end: (UnsafeMutableRawPointer?) -> Void,
    get: (
      UInt64,
      UnsafeMutablePointer<Raw>,
      UnsafeMutablePointer<mln_diagnostic>
    ) -> mln_status,
    _ body: (Raw, NativeViewScope) throws -> T
  ) throws -> T {
    defer { withExtendedLifetime(self) {} }
    var token: UnsafeMutableRawPointer?
    try checkStatus { begin(handle.value.raw, &token, $0) }
    let scope = NativeViewScope()
    defer { scope.expire(); end(token) }
    var raw = raw
    try checkStatus { get(handle.value.raw, &raw, $0) }
    return try body(raw, scope)
  }

  func nativeClose(
    _ call: @escaping (UInt64, UnsafeMutablePointer<mln_diagnostic>)
      -> mln_status
  ) throws {
    try handle.closeOnce { live in try checkStatus { call(live.raw, $0) } }
  }

  func nativeClose(
    _ call: @escaping (UInt64) -> Void
  ) {
    handle.closeUnguarded { live in call(live.raw) }
  }

  /// Closes a batch whose release can't fail, but which refuses to close
  /// while a copy reads it.
  func nativeGuardedClose(
    _ call: @escaping (UInt64) -> Void
  ) throws {
    try handle.closeOnce { live in call(live.raw) }
  }

  func nativeStartClose(
    _ call: @escaping (
      UInt64,
      UnsafePointer<mln_completion>,
      UnsafeMutablePointer<mln_diagnostic>
    ) -> mln_status
  ) throws -> NativeFuture<Void>? {
    var future: NativeFuture<Void>?
    try handle.closeOnce(idempotent: false) { live in
      future = try NativeCompletion.startUnit { call(live.raw, $0, $1) }
    }
    return future
  }

  /// Starts work that also returns an owner, and pairs the owner that `adopt`
  /// takes with a task that awaits the work, as `combine` puts them together.
  /// The task keeps the owner alive until the work completes.
  func nativeAttach<Owner: AnyObject & Sendable, Attachment>(
    as combine: (Owner, Task<Void, Error>) -> Attachment,
    _ call: NativeStartCall,
    adopt: () throws -> Owner
  ) throws -> Attachment {
    try withNative { raw, arena in
      let future = try NativeCompletion.startUnit { completion, diagnostic in
        try arena.submit { try call(raw, arena, completion, diagnostic) }
      }
      let owner = try adopt()
      return combine(owner, Task { [owner] in
        defer { withExtendedLifetime(owner) {} }
        try await future.value()
      })
    }
  }
}

private func checkedCall(
  _ call: NativeStatusCall,
  _ raw: UInt64,
  _ arena: NativeInputArena
) throws {
  try checkStatus { diagnostic in
    try arena.submit { try call(raw, arena, diagnostic) }
  }
}

private func withNative<T>(
  owner: AnyObject?,
  _ body: (NativeInputArena) throws -> T
) throws -> T {
  // An entry point without a receiver can be a program's first call, so it
  // checks the loaded library's C ABI version before anything reaches C.
  if owner == nil {
    try NativeAbi.ensureCompatible()
  }
  let arena = NativeInputArena()
  defer { withExtendedLifetime(arena) {} }
  return try body(arena)
}

func nativeInvoke<T>(
  owner: AnyObject? = nil,
  _ call: NativeStatusCall,
  result: () throws -> T
) throws -> T {
  try withNative(owner: owner) { arena in
    try checkedCall(call, 0, arena)
    return try result()
  }
}

func nativeInvoke(
  owner: AnyObject? = nil,
  _ call: NativeStatusCall
) throws {
  try nativeInvoke(owner: owner, call) { () }
}

/// Calls a status-returning function like `nativeInvoke`, except that
/// `absent` reports that native published no output, and returns nil.
func nativeInvoke<T>(
  owner: AnyObject? = nil,
  absentOn absent: mln_status,
  _ call: NativeStatusCall,
  result: () throws -> T
) throws -> T? {
  var present = true
  return try nativeInvoke(owner: owner, { raw, arena, diagnostic in
    let status = try call(raw, arena, diagnostic)
    guard status == absent else { return status }
    present = false
    return MLN_STATUS_OK
  }) { present ? try result() : nil }
}

func nativeStart<T: Sendable>(
  convert: @escaping (UnsafePointer<mln_completion_result>) throws -> T,
  _ call: NativeStartCall
) async throws -> T {
  try await withNative(owner: nil) { arena in
    try NativeCompletion.start({ completion, diagnostic in
      try arena.submit { try call(0, arena, completion, diagnostic) }
    }, convert: convert)
  }.value()
}

func nativeStart<Raw, T: Sendable>(
  copying copy: @escaping (Raw) throws -> T,
  _ call: NativeStartCall
) async throws -> T {
  try await nativeStart(convert: { result in
    try copy(NativeCompletion.value(result))
  }, call)
}

func nativeUnit(_ call: NativeStartCall) async throws {
  try await nativeStart(convert: { _ in () }, call)
}

@discardableResult
func nativeCommand(
  _ call: NativeStartCall
) async throws -> CommandCompletion {
  try await withNative(owner: nil) { arena in
    try NativeCompletion.startCommand { completion, diagnostic in
      try arena.submit { try call(0, arena, completion, diagnostic) }
    }
  }.value()
}

func nativeDirect<T>(
  _ call: (NativeInputArena) throws -> T
) throws -> T {
  try withNative(owner: nil) { arena in try call(arena) }
}
