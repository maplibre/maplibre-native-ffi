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
/// A JSON value, typed `Object?`, is any value that `jsonEncode` takes, so
/// `setStyleLayerProperty('roads', 'line-color', 'red')` needs no inner
/// quotes. A JSON parameter is always a value: `null` is JSON null, such as the
/// null that restores a layer property's default. In a record member, `null` is
/// absent, which native code reads as empty, as for a filter, an expression
/// input, or cluster properties, where JSON null means nothing. A value that
/// `jsonEncode` rejects throws its error before the call reaches native code.
/// Returned JSON is the value that `jsonDecode` reads, and absent JSON is
/// `null`, as JSON null is. A value compares its JSON members by content.
///
/// On Android, the app's host code must call `mln_android_init` with a
/// `JNIEnv*` and a `Context` before Dart creates a runtime, for example from a
/// JNI function that the Flutter activity calls in `onCreate`. The binding has
/// no Dart form of that call, because an isolate cannot safely obtain either
/// pointer.
///
/// A pending completion keeps its isolate alive, and a callback registration
/// does not. To wait for a callback, keep the isolate alive yourself, for
/// example with a `ReceivePort` that you close once the callback runs.
library;

export 'src/error/maplibre_exception.dart';
export 'src/render/native_pointer.dart';
export 'src/runtime/runtime.dart';
export 'src/values.dart';
