package maplibre

/*
#include <maplibre_native_c/plugin.h>
*/
import "C"

// LoadPlugin loads a layer plugin shared library and registers its style layer
// types process-wide. path is the UTF-8 path of the plugin library, and
// entryPoint names the exported entry point that the library calls with the
// process-wide register function. The library stays loaded for the process
// lifetime, because registration retains the plugin's callbacks.
//
// LoadPlugin is callable from any goroutine. Call it before any style that
// uses the plugin's layer types loads. Loading an identical plugin again
// succeeds.
//
// Errors: ErrInvalidArgument when path or entryPoint is empty, and ErrNative
// with the OS or plugin diagnostic when the library fails to load, the entry
// point fails to resolve, or registration fails.
func LoadPlugin(path string, entryPoint string) error {
	pathView := newCStringView(path)
	defer pathView.free()
	entryPointView := newCStringView(entryPoint)
	defer entryPointView.free()
	return checkNative(func() int32 {
		return int32(C.mln_plugin_load_library(pathView.raw(), entryPointView.raw()))
	})
}
