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
/// zone's error handler. For the same reason, render sessions take no queue
/// lock, so a Vulkan session's graphics queue belongs to the session alone.
///
/// A pending completion keeps its isolate alive, and a callback registration
/// does not. To wait for a callback, keep the isolate alive yourself, for
/// example with a `ReceivePort` that you close once the callback runs.
library;

export 'src/error/maplibre_exception.dart';
export 'src/render/native_pointer.dart';
export 'src/runtime/runtime.dart';
export 'src/values.dart';
