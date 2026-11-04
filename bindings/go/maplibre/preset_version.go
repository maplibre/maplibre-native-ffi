package maplibre

import (
	"fmt"
	"runtime/debug"
	"strings"
)

// bindingModule is the module that versions this package. The release tag
// versions it and each preset module alike.
const bindingModule = "maplibre.org/maplibre-native-ffi"

// checkPresetVersion compares the release that a preset module was published
// for with this module's release in the program's build info. It compares only
// when the build info names a release version for this module, so a local
// checkout, a replacement directory, or a pseudo-version skips the check.
func checkPresetVersion(preset string, info *debug.BuildInfo) error {
	binding := bindingModuleVersion(info)
	if !isReleaseVersion(binding) || preset == binding {
		return nil
	}
	return fmt.Errorf(
		"maplibre: the preset module is from release %s, but %s is %s; require both at the same version",
		preset, bindingModule, binding,
	)
}

func bindingModuleVersion(info *debug.BuildInfo) string {
	if info == nil {
		return ""
	}
	if info.Main.Path == bindingModule {
		return info.Main.Version
	}
	for _, dependency := range info.Deps {
		if dependency.Path != bindingModule {
			continue
		}
		if dependency.Replace != nil {
			return dependency.Replace.Version
		}
		return dependency.Version
	}
	return ""
}

// isReleaseVersion reports whether version has a release's form, vE.YYYYMM.R,
// rather than (devel), a pseudo-version, or one with build metadata.
func isReleaseVersion(version string) bool {
	fields := strings.Split(strings.TrimPrefix(version, "v"), ".")
	if !strings.HasPrefix(version, "v") || len(fields) != 3 {
		return false
	}
	for _, field := range fields {
		if field == "" || strings.Trim(field, "0123456789") != "" {
			return false
		}
	}
	return true
}
