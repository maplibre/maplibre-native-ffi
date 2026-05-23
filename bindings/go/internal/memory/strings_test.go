package memory

import (
	"errors"
	"testing"
)

func TestNewCStringRejectsEmbeddedNUL(t *testing.T) {
	_, err := NewCString("style\x00url")
	if !errors.Is(err, EmbeddedNulError()) {
		t.Fatalf("NewCString embedded NUL error = %v, want EmbeddedNulError", err)
	}
}

func TestNewCStringAppendsTerminator(t *testing.T) {
	cstr, err := NewCString("style")
	if err != nil {
		t.Fatal(err)
	}
	if cstr.Ptr() == nil {
		t.Fatal("CString Ptr() is nil")
	}
	if got := cstr.Len(); got != 5 {
		t.Fatalf("CString Len() = %d, want 5", got)
	}
	cstr.KeepAlive()
}

func TestStringViewPreservesEmbeddedNULAndLength(t *testing.T) {
	view := NewStringView("a\x00b")
	if view.Data() == nil {
		t.Fatal("StringView Data() is nil")
	}
	if got := view.Len(); got != 3 {
		t.Fatalf("StringView Len() = %d, want 3", got)
	}
	if got := string(view.Bytes()); got != "a\x00b" {
		t.Fatalf("StringView Bytes() = %q, want embedded NUL preserved", got)
	}
	view.KeepAlive()
}

func TestEmptyStringViewUsesNilDataWithZeroLength(t *testing.T) {
	view := NewStringView("")
	if view.Data() != nil {
		t.Fatal("empty StringView Data() is non-nil")
	}
	if got := view.Len(); got != 0 {
		t.Fatalf("empty StringView Len() = %d, want 0", got)
	}
}
