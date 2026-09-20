package maplibre

import (
	"errors"
	"testing"
)

func TestLoadPluginNonexistentLibraryFails(t *testing.T) {
	if err := LoadPlugin("/nonexistent/libmln-plugin-no-such-library.dylib", "mln_plugin_entry"); !errors.Is(err, ErrNative) {
		t.Fatalf("LoadPlugin(nonexistent) error = %v, want ErrNative", err)
	}
	if err := LoadPlugin("", "mln_plugin_entry"); !errors.Is(err, ErrInvalidArgument) {
		t.Fatalf("LoadPlugin(empty path) error = %v, want ErrInvalidArgument", err)
	}
}
