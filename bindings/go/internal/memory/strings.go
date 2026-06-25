package memory

import (
	"bytes"
	"errors"
	"runtime"
	"unsafe"
)

var errEmbeddedNUL = errors.New("string contains embedded NUL")

// EmbeddedNulError returns the stable error used for null-terminated C string
// inputs that would be truncated by C.
func EmbeddedNulError() error {
	return errEmbeddedNUL
}

// CString stores a call-scoped NUL-terminated UTF-8 string in Go memory.
type CString struct {
	bytes []byte
}

// NewCString creates a call-scoped NUL-terminated string and rejects embedded
// NUL bytes.
func NewCString(value string) (CString, error) {
	if bytes.IndexByte([]byte(value), 0) >= 0 {
		return CString{}, errEmbeddedNUL
	}
	data := make([]byte, len(value)+1)
	copy(data, value)
	return CString{bytes: data}, nil
}

// Ptr returns a pointer suitable for a call-scoped const char* argument.
func (c CString) Ptr() unsafe.Pointer {
	if len(c.bytes) == 0 {
		return nil
	}
	return unsafe.Pointer(&c.bytes[0])
}

// Len returns the length excluding the terminating NUL byte.
func (c CString) Len() int {
	if len(c.bytes) == 0 {
		return 0
	}
	return len(c.bytes) - 1
}

// KeepAlive keeps the backing bytes reachable until this point after a cgo call.
func (c CString) KeepAlive() {
	runtime.KeepAlive(c.bytes)
}

// StringView stores call-scoped UTF-8 bytes for an explicit-length C string
// view. Embedded NUL bytes are preserved because C receives the length.
type StringView struct {
	bytes []byte
}

// NewStringView copies a Go string into call-scoped bytes for an explicit-length
// native string view.
func NewStringView(value string) StringView {
	return StringView{bytes: []byte(value)}
}

// Data returns the first byte pointer, or nil for an empty view.
func (view StringView) Data() unsafe.Pointer {
	if len(view.bytes) == 0 {
		return nil
	}
	return unsafe.Pointer(&view.bytes[0])
}

// Len returns the explicit byte length of the view.
func (view StringView) Len() int {
	return len(view.bytes)
}

// Bytes returns the copied view bytes.
func (view StringView) Bytes() []byte {
	return view.bytes
}

// KeepAlive keeps the backing bytes reachable until this point after a cgo call.
func (view StringView) KeepAlive() {
	runtime.KeepAlive(view.bytes)
}
