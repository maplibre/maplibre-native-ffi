/// Low-level Dart bindings for the MapLibre Native C API.
///
/// Dart can resume an isolate on another native thread after any `await`, so
/// an isolate cannot own a caller-driven render session. Render through
/// core-worker sessions: Metal, Vulkan, or an EGL context that the session
/// creates for itself. That EGL context shares nothing with the host, so it
/// renders to a surface or offers its texture as readback. On Windows, render
/// through Vulkan.
///
/// Native code never waits on a Dart callback. Resource providers, the log
/// callback, and cancel callbacks run on the registering isolate after native
/// code has moved on, and an exception that one throws goes to the registering
/// zone's error handler.
///
/// Native code in the host, such as a Flutter plugin's platform code, can
/// supply a registration's function pointers, context, and release through the
/// `native` constructors. `Wake.native` wakes the host's native code without a
/// post to the isolate. `QueueLock.native` is the only form of a queue lock,
/// because native code relies on the lock being held when its callback
/// returns. The first call that carries a native registration consumes it, and
/// a second call throws `MaplibreException` with `Status.invalidState`. Native
/// code releases the context of a call that it accepts, and the binding calls
/// the release for a call that native code does not accept.
///
/// A pending completion keeps its isolate alive, and a callback registration
/// does not. To wait for a callback, keep the isolate alive yourself, for
/// example with a `ReceivePort` that you close once the callback runs.
library;

export 'src/error/maplibre_exception.dart';
export 'src/render/native_pointer.dart';
export 'src/runtime/runtime.dart';
export 'src/values.dart';
