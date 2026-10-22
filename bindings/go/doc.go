// Package maplibre is the low-level Go binding for the MapLibre Native C API.
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
// The binding recovers a callback's panic, returns the callback's failure value
// to native, and logs the panic with [log/slog.Error] on the default logger. Go
// cleanup disposes an owner that the host abandoned and logs it with
// [log/slog.Warn].
package maplibre
