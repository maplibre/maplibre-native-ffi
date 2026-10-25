// Package maplibre is the low-level Go binding for the MapLibre Native C API.
//
// The first [RenderSessionHandle.ServiceDriverWork] call fixes a caller-driver
// session's graphics thread, and the Go scheduler moves goroutines between OS
// threads. Call [runtime.LockOSThread] on the goroutine that services the
// session and hold it for the session's lifetime.
//
// Acquired frames expose their texture only inside a callback such as
// [AcquiredFrameHandle.WithOpenglTexture], on the callback's OS thread. Keep
// servicing a caller-driver render session while its attachment or detachment
// is pending.
//
// Calls from inside a log or Vulkan queue-lock callback return
// [StatusInvalidState]. A callback may wait for a submission's acceptance but
// not for its completion, which can deadlock native work that needs the
// callback to return. A scoped response such as
// [HttpHeaderTransformResponseScope] works only during its callback, on the
// callback's thread.
//
// Each call of a Go callback enters the Go runtime from a native thread. A host
// whose wake or queue lock is already a C function passes it as a native
// registration from [NewNativeWake] or [NewNativeQueueLock], with the C function
// pointers and context as uintptr values. The first call that carries a native
// registration consumes it, and a later call returns [StatusInvalidState].
// Native code releases the context of a call that it accepts, and the binding
// calls the release for any other call.
//
// JSON crosses as a string. An absent JSON result, such as an undefined layer
// property, is "", since present JSON is never empty.
//
// A callback whose C type returns a status returns an error instead. Nil
// answers [StatusOk], and a failure [Status] or an [*Error] with one, or an
// error that wraps either, answers that status on purpose. Any other error is
// a bug: the binding logs it with [log/slog.Error] on the default logger and
// answers the callback's failure status.
//
// The binding recovers a callback's panic, returns the callback's failure value
// to native, and logs the panic with [log/slog.Error] on the default logger. Go
// cleanup disposes an owner that the host abandoned and logs it with
// [log/slog.Warn].
package maplibre
