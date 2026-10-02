//go:build unix && cgo

package testsupport

/*
#cgo linux LDFLAGS: -ldl
#include <dlfcn.h>
#include <stddef.h>

static int mln_go_test_write_coverage_profile(void) {
	int (*write_profile)(void) = (int (*)(void))dlsym(
		RTLD_DEFAULT, "mln_ffi_coverage_write_profile"
	);
	return write_profile != NULL ? write_profile() : 0;
}
*/
import "C"

import "errors"

// WriteCoverageProfile writes the native library's coverage profile when the
// process loaded a coverage build, and does nothing otherwise. Go ends a
// process with _exit, which skips the exit hook that writes the profile.
func WriteCoverageProfile() error {
	if C.mln_go_test_write_coverage_profile() != 0 {
		return errors.New("the native library could not write its coverage profile")
	}
	return nil
}
