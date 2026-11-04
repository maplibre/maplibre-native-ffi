//go:build mlntest

package maplibre

import (
	"runtime/debug"
	"strings"
	"testing"
)

// A preset module's init runs this check on the release that the module was
// published for, and panics on a mismatch.
func TestPresetVersionMismatchIsRejected(t *testing.T) {
	dependency := func(version string, replace *debug.Module) *debug.BuildInfo {
		return &debug.BuildInfo{
			Main: debug.Module{Path: "app", Version: "(devel)"},
			Deps: []*debug.Module{{Path: bindingModule, Version: version, Replace: replace}},
		}
	}
	cases := []struct {
		name     string
		info     *debug.BuildInfo
		mismatch bool
	}{
		{"same release", dependency("v0.202610.3", nil), false},
		{"other release", dependency("v0.202610.2", nil), true},
		{"replaced by another release", dependency("v0.202610.3", &debug.Module{Path: bindingModule, Version: "v0.202610.2"}), true},
		{"replaced by a directory", dependency("v0.202610.2", &debug.Module{Path: "../maplibre-native-ffi"}), false},
		{"pseudo-version", dependency("v0.202610.4-0.20261104000000-0123456789ab", nil), false},
		{"main module", &debug.BuildInfo{Main: debug.Module{Path: bindingModule, Version: "(devel)"}}, false},
		{"main module with local changes", &debug.BuildInfo{Main: debug.Module{Path: bindingModule, Version: "v0.202610.2+dirty"}}, false},
		{"no build info", nil, false},
	}
	for _, c := range cases {
		t.Run(c.name, func(t *testing.T) {
			err := checkPresetVersion("v0.202610.3", c.info)
			if !c.mismatch {
				if err != nil {
					t.Fatalf("checkPresetVersion = %v, want nil", err)
				}
				return
			}
			if err == nil || !strings.Contains(err.Error(), "v0.202610.3") || !strings.Contains(err.Error(), "v0.202610.2") {
				t.Fatalf("checkPresetVersion = %v, want an error naming both releases", err)
			}
		})
	}
}
