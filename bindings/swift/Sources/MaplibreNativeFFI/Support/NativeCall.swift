internal import CMaplibreNativeC
import Foundation

// The steps that every generated operation shares.
//
// A generated operation names its native function and passes a closure that
// makes the call with each argument converted in place. These functions admit
// the call inside any enclosing callback, hold the receiver, provide an input
// arena whose storage lasts until the call returns, check the status, hand
// callback registrations to native when it accepts them, and convert the
// result while the receiver is still held. Each reports failures as
// ``MaplibreError``.

/// A public handle whose generated operations run through the steps below.
protocol NativeReceiver: AnyObject {
  associatedtype Native: NativeHandle
  var handle: NativeHandleBox<Native> { get }
}

extension NativeReceiver {
  /// Disposes the handle of an owner that the binding created but no caller
  /// received, without reporting a leak.
  func retireUnreceived() {
    handle.retire()
  }
}

/// How an operation reaches its receiver.
enum NativeAccess {
  /// The call borrows the receiver, which holds off its close.
  case live
  /// The call passes the receiver's issued id without borrowing it.
  case issued
  /// The call borrows the receiver and claims its pending decision, which the
  /// call accepts when it succeeds.
  case claim
}

/// A native call that reports a status into a diagnostic. It receives the
/// receiver's raw id, or 0 without a handle receiver, and the input arena.
typealias NativeStatusCall = (
  UInt64,
  NativeInputArena,
  UnsafeMutablePointer<mln_diagnostic>
) throws -> mln_status

/// A native call that starts work reporting through a completion.
typealias NativeStartCall = (
  UInt64,
  NativeInputArena,
  UnsafePointer<mln_completion>,
  UnsafeMutablePointer<mln_diagnostic>
) throws -> mln_status

extension NativeReceiver {
  private func withNative<T>(
    _ operation: String,
    _ access: NativeAccess,
    _ body: (UInt64, NativeInputArena) throws -> T
  ) throws -> T {
    try NativeCallbackGuard.check(owner: self, operation: operation)
    defer { withExtendedLifetime(self) {} }
    let arena = NativeInputArena()
    defer { withExtendedLifetime(arena) {} }
    if access == .issued {
      return try body(handle.issued.raw, arena)
    }
    let read = try handle.borrow()
    defer { read.end() }
    guard access == .claim else { return try body(read.handle.raw, arena) }
    let claim = try handle.beginClaim()
    defer { claim.end() }
    let value = try body(read.handle.raw, arena)
    claim.accept()
    return value
  }

  /// Calls a status-returning function and returns `result`, which reads the
  /// call's outputs while the receiver is held.
  func nativeInvoke<T>(
    _ operation: String,
    _ access: NativeAccess = .live,
    _ call: NativeStatusCall,
    result: () throws -> T
  ) throws -> T {
    try mapNativeFailure {
      try withNative(operation, access) { raw, arena in
        try checkedCall(call, raw, arena)
        return try result()
      }
    }
  }

  func nativeInvoke(
    _ operation: String,
    _ access: NativeAccess = .live,
    _ call: NativeStatusCall
  ) throws {
    try nativeInvoke(operation, access, call) { () }
  }

  /// Calls a status-returning function like `nativeInvoke`, except that
  /// `absent` reports that native published no output, and returns nil.
  func nativeInvoke<T>(
    _ operation: String,
    _ access: NativeAccess = .live,
    absentOn absent: mln_status,
    _ call: NativeStatusCall,
    result: () throws -> T
  ) throws -> T? {
    var present = true
    return try nativeInvoke(operation, access, { raw, arena, diagnostic in
      let status = try call(raw, arena, diagnostic)
      guard status == absent else { return status }
      present = false
      return MLN_STATUS_OK
    }) { present ? try result() : nil }
  }

  /// Starts work and awaits the value `convert` copies from its completion.
  func nativeStart<T: Sendable>(
    _ operation: String,
    _ access: NativeAccess = .live,
    convert: @escaping (UnsafePointer<mln_completion_result>) throws -> T,
    _ call: NativeStartCall
  ) async throws -> T {
    try await awaitNative {
      try withNative(operation, access) { raw, arena in
        try NativeCompletion.start({ completion, diagnostic in
          try arena.submit { try call(raw, arena, completion, diagnostic) }
        }, convert: convert)
      }
    }
  }

  /// Starts work and awaits its completion's one native value, which `copy`
  /// converts.
  func nativeStart<Raw, T: Sendable>(
    _ operation: String,
    _ access: NativeAccess = .live,
    copying copy: @escaping (Raw) throws -> T,
    _ call: NativeStartCall
  ) async throws -> T {
    try await nativeStart(operation, access, convert: { result in
      try copy(NativeCompletion.value(result))
    }, call)
  }

  /// Starts work and awaits its completion.
  func nativeUnit(
    _ operation: String,
    _ access: NativeAccess = .live,
    _ call: NativeStartCall
  ) async throws {
    try await nativeStart(operation, access, convert: { _ in () }, call)
  }

  /// Starts a command and awaits its disposition, which reports a failed or
  /// cancelled command as data rather than as an error.
  @discardableResult
  func nativeCommand(
    _ operation: String,
    _ access: NativeAccess = .live,
    _ call: NativeStartCall
  ) async throws -> CommandCompletion {
    try await awaitNative {
      try withNative(operation, access) { raw, arena in
        try NativeCompletion.startCommand { completion, diagnostic in
          try arena.submit { try call(raw, arena, completion, diagnostic) }
        }
      }
    }
  }

  /// Lends `body` a value that the receiver lends only inside a native view
  /// scope. `begin` opens the scope and `end` closes it, `get` fills `raw`, and
  /// `body` receives the filled value with the binding scope that expires when
  /// `body` returns.
  func nativeView<Raw, T>(
    _ operation: String,
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
    try NativeCallbackGuard.check(owner: self, operation: operation)
    return try mapNativeFailure {
      let access = try handle.borrow()
      defer { access.end(); withExtendedLifetime(self) {} }
      var token: UnsafeMutableRawPointer?
      try checkStatus { begin(access.handle.raw, &token, $0) }
      let scope = NativeViewScope()
      defer { scope.expire(); end(token) }
      var raw = raw
      try checkStatus { get(access.handle.raw, &raw, $0) }
      return try body(raw, scope)
    }
  }

  /// Closes the receiver once with a status-returning native call. A later
  /// close does nothing.
  func nativeClose(
    _ operation: String,
    _ call: @escaping (UInt64, UnsafeMutablePointer<mln_diagnostic>)
      -> mln_status
  ) throws {
    try NativeCallbackGuard.check(owner: self, operation: operation)
    try mapNativeFailure {
      try handle.closeOnce { live in try checkStatus { call(live.raw, $0) } }
    }
  }

  /// Closes the receiver once with a native call that cannot fail.
  func nativeClose(
    _ operation: String,
    _ call: @escaping (UInt64) -> Void
  ) throws {
    try NativeCallbackGuard.check(owner: self, operation: operation)
    try mapNativeFailure { try handle.closeOnce { live in call(live.raw) } }
  }

  /// Starts closing the receiver, or returns nil when it is already closed.
  func nativeStartClose(
    _ operation: String,
    _ call: @escaping (
      UInt64,
      UnsafePointer<mln_completion>,
      UnsafeMutablePointer<mln_diagnostic>
    ) -> mln_status
  ) throws -> NativeFuture<Void>? {
    try NativeCallbackGuard.check(owner: self, operation: operation)
    var future: NativeFuture<Void>?
    try handle.closeOnce { live in
      future = try NativeCompletion.startUnit { call(live.raw, $0, $1) }
    }
    return future
  }

  /// Starts work that also returns an owner, and pairs the owner that `adopt`
  /// takes with a task that awaits the work, as `combine` puts them together.
  /// The task keeps the owner alive until the work completes.
  func nativeAttach<Owner: AnyObject & Sendable, Attachment>(
    _ operation: String,
    as combine: (Owner, Task<Void, Error>) -> Attachment,
    _ call: NativeStartCall,
    adopt: () throws -> Owner
  ) throws -> Attachment {
    try mapNativeFailure {
      try withNative(operation, .live) { raw, arena in
        let future = try NativeCompletion.startUnit { completion, diagnostic in
          try arena.submit { try call(raw, arena, completion, diagnostic) }
        }
        let owner = try adopt()
        return combine(owner, Task { [owner] in
          defer { withExtendedLifetime(owner) {} }
          try await mapNativeFailure { try await future.value() }
        })
      }
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
  _ operation: String,
  _ body: (NativeInputArena) throws -> T
) throws -> T {
  // An entry point without a receiver can be a program's first call, so it
  // checks the loaded library's C ABI version before anything reaches C.
  if owner == nil { try NativeAbi.ensureCompatible() }
  try NativeCallbackGuard.check(owner: owner, operation: operation)
  let arena = NativeInputArena()
  defer { withExtendedLifetime(arena) {} }
  return try body(arena)
}

/// Calls a status-returning function without a handle receiver and returns
/// `result`. `owner` is the callback-scoped response the call acts on, or nil
/// for a global call.
func nativeInvoke<T>(
  owner: AnyObject? = nil,
  _ operation: String,
  _ call: NativeStatusCall,
  result: () throws -> T
) throws -> T {
  try mapNativeFailure {
    try withNative(owner: owner, operation) { arena in
      try checkedCall(call, 0, arena)
      return try result()
    }
  }
}

func nativeInvoke(
  owner: AnyObject? = nil,
  _ operation: String,
  _ call: NativeStatusCall
) throws {
  try nativeInvoke(owner: owner, operation, call) { () }
}

/// Calls a status-returning function like `nativeInvoke`, except that
/// `absent` reports that native published no output, and returns nil.
func nativeInvoke<T>(
  owner: AnyObject? = nil,
  _ operation: String,
  absentOn absent: mln_status,
  _ call: NativeStatusCall,
  result: () throws -> T
) throws -> T? {
  var present = true
  return try nativeInvoke(owner: owner, operation, { raw, arena, diagnostic in
    let status = try call(raw, arena, diagnostic)
    guard status == absent else { return status }
    present = false
    return MLN_STATUS_OK
  }) { present ? try result() : nil }
}

/// Starts global work and awaits the value `convert` copies from its
/// completion.
func nativeStart<T: Sendable>(
  _ operation: String,
  convert: @escaping (UnsafePointer<mln_completion_result>) throws -> T,
  _ call: NativeStartCall
) async throws -> T {
  try await awaitNative {
    try withNative(owner: nil, operation) { arena in
      try NativeCompletion.start({ completion, diagnostic in
        try arena.submit { try call(0, arena, completion, diagnostic) }
      }, convert: convert)
    }
  }
}

/// Starts global work and awaits its completion's one native value, which
/// `copy` converts.
func nativeStart<Raw, T: Sendable>(
  _ operation: String,
  copying copy: @escaping (Raw) throws -> T,
  _ call: NativeStartCall
) async throws -> T {
  try await nativeStart(operation, convert: { result in
    try copy(NativeCompletion.value(result))
  }, call)
}

/// Starts global work and awaits its completion.
func nativeUnit(_ operation: String, _ call: NativeStartCall) async throws {
  try await nativeStart(operation, convert: { _ in () }, call)
}

/// Starts a global command and awaits its disposition.
@discardableResult
func nativeCommand(
  _ operation: String,
  _ call: NativeStartCall
) async throws -> CommandCompletion {
  try await awaitNative {
    try withNative(owner: nil, operation) { arena in
      try NativeCompletion.startCommand { completion, diagnostic in
        try arena.submit { try call(0, arena, completion, diagnostic) }
      }
    }
  }
}

/// Calls a global function that returns its result rather than a status.
func nativeDirect<T>(
  _ operation: String,
  _ call: (NativeInputArena) throws -> T
) throws -> T {
  try mapNativeFailure {
    try withNative(owner: nil, operation) { arena in try call(arena) }
  }
}
