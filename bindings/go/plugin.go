package maplibre

/*
#include <stdint.h>
#include <maplibre_native_c/plugin.h>
static uintptr_t plugin_register_function_v1(void) {
    return (uintptr_t)mln_plugin_get_register_function_v1();
}
*/
import "C"

// PluginRegisterFunctionV1 returns the process-lifetime address of the v1 plugin
// registration function. Pass it to the plugin's own registration entry point
// before loading dependent styles.
func PluginRegisterFunctionV1() NativePointer {
	return NativePointer(C.plugin_register_function_v1())
}
