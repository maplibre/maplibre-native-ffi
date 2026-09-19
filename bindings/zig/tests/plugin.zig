const testing = @import("std").testing;

const maplibre = @import("maplibre_native_ffi");

test "loading a nonexistent plugin library fails with a diagnostic" {
    try testing.expectError(
        error.NativeError,
        maplibre.loadPlugin(
            testing.allocator,
            "/nonexistent/libmaplibre-test-plugin.so",
            "mln_test_plugin_register",
        ),
    );
}
