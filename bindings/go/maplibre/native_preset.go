//go:build !mln_pkgconfig

package maplibre

// Without the mln_pkgconfig tag, the package compiles against include/, which
// the module carries, and a preset module links the C library. The preset
// module defines mln_go_preset_version as the release it was published for, so
// a program without a preset module, or with two for one platform, fails to
// link.

/*
#cgo CPPFLAGS: -I${SRCDIR}/../../../include
extern const char mln_go_preset_version[];
static const char *mln_go_preset_version_string(void) { return mln_go_preset_version; }
*/
import "C"

import "runtime/debug"

func init() {
	info, _ := debug.ReadBuildInfo()
	if err := checkPresetVersion(C.GoString(C.mln_go_preset_version_string()), info); err != nil {
		panic(err)
	}
}
