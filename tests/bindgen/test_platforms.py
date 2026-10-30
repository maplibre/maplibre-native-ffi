"""A `platforms=` annotation limits a function to the platforms it serves."""

import unittest

from support import emitted, parse

from tools.bindgen.compiler import compile_api
from tools.bindgen.emitters import (
    dart,
    dotnet,
    go,
    kotlin,
    python,
    rust,
    swift,
    typescript,
    zig,
)
from tools.bindgen.model import ModelError
from tools.bindgen.schema import validate

# A map handle with a function that only Apple platforms serve, a browser-only
# function and the record that only it takes, and an Android-only global.
PLATFORM_LIMITED = """
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release");
void mln_map_release(mln_map map);
typedef struct mln_canvas_target { unsigned int size; double scale; } mln_canvas_target;
mln_status mln_map_redraw(mln_map map, mln_diagnostic *out_diagnostic);
BIND("platforms=ios,macos,tvos,visionos")
mln_status mln_map_use_metal(mln_map map, mln_diagnostic *out_diagnostic);
BIND("platforms=web")
mln_status mln_map_attach_canvas(mln_map map, const mln_canvas_target *target, mln_diagnostic *out_diagnostic);
BIND("platforms=android")
mln_status mln_android_start(void *jni_env, mln_diagnostic *out_diagnostic);
"""


def api():
    parsed = parse(PLATFORM_LIMITED)
    validate(parsed)
    return parsed


class PlatformAnnotationTests(unittest.TestCase):
    def test_an_annotation_names_known_platforms_once_in_order(self):
        for platforms, message in (
            ("beos", "unknown platforms 'beos'"),
            ("web,android", "each platform once, in order"),
            ("macos,macos", "each platform once, in order"),
            (
                "android,freebsd,ios,linux,macos,ohos,tvos,visionos,web,windows",
                "restates the default",
            ),
        ):
            header = parse(f"""BIND("platforms={platforms}")
mln_status mln_start(mln_diagnostic *out_diagnostic);
""")
            with (
                self.subTest(platforms=platforms),
                self.assertRaisesRegex(ModelError, message),
            ):
                validate(header)

    def test_a_binding_leaves_out_what_none_of_its_targets_serve(self):
        bound = compile_api(api(), frozenset({"linux", "macos"}))
        self.assertNotIn("mln_map_attach_canvas", bound.operations_by_name)
        self.assertNotIn("mln_android_start", bound.operations_by_name)
        # The record that only the browser-only function took leaves with it.
        self.assertNotIn("mln_canvas_target", bound.values)
        self.assertIn("mln_map_use_metal", bound.operations_by_name)
        browser = compile_api(api(), frozenset({"web"}))
        self.assertIn("mln_canvas_target", browser.values)

    def test_a_binding_that_cannot_select_a_platform_drops_only_what_none_serve(
        self,
    ):
        generated = emitted(python, api())
        operations = generated["python/maplibre_native_ffi/_generated_operations.py"]
        self.assertIn("def use_metal(", operations)
        self.assertIn(
            "def android_start(",
            generated["python/maplibre_native_ffi/api.py"] + operations,
        )
        self.assertNotIn("attach_canvas", operations)
        self.assertNotIn(
            "CanvasTarget", generated["python/maplibre_native_ffi/_generated_values.py"]
        )
        typescript_operations = emitted(typescript, api())[
            "bindings/typescript/src/generated_operations.ts"
        ]
        # The browser module shares the operations, so a browser-only function
        # stays.
        self.assertIn("useMetal(", typescript_operations)
        self.assertIn("attachCanvas(", typescript_operations)
        self.assertNotIn("androidStart(", typescript_operations)
        dart_operations = dart.lower(api()).files[dart.OPERATIONS]
        self.assertIn("useMetal(", dart_operations)
        self.assertNotIn("attachCanvas(", dart_operations)

    def test_rust_compiles_a_function_only_for_the_platforms_it_serves(self):
        source = emitted(rust, api())["crates/maplibre-native-ffi/src/generated/map.rs"]
        self.assertIn(
            '    #[cfg(any(target_os = "ios", target_os = "macos", target_os = "tvos", target_os = "visionos"))]\n'
            "    pub fn use_metal(",
            source,
        )
        self.assertIn('    #[cfg(target_os = "emscripten")]\n    pub', source)
        self.assertIn("    pub fn redraw(", source)
        self.assertNotIn('#[cfg(any(target_os = "android"', source)

    def test_swift_compiles_a_function_only_for_the_platforms_it_serves(self):
        generated = "\n".join(emitted(swift, api()).values())
        self.assertIn(
            "#if os(iOS) || os(macOS) || os(tvOS) || os(visionOS)\nfunc `useMetal`() throws {\n",
            generated,
        )
        self.assertIn("func `redraw`() throws", generated)
        # Android declarations stay behind their platform guard; no Swift
        # target serves the browser.
        self.assertIn(
            "#if os(Android)\npublic func `androidStart`(",
            generated,
        )
        self.assertNotIn("attachCanvas", generated)

    def test_zig_declares_a_function_void_off_the_platforms_it_serves(self):
        source = emitted(zig, api())["maplibre_native_ffi.zig"]
        self.assertIn(
            "pub const useMetal = if (builtin.os.tag == .maccatalyst or "
            "builtin.os.tag == .macos) struct {\n"
            "fn function(self: Map, ",
            source,
        )
        self.assertIn("}.function else {};", source)
        self.assertIn(
            "pub const androidStart = if (builtin.abi.isAndroid()) struct {", source
        )
        self.assertNotIn("attachCanvas", source)

    def test_go_builds_a_function_only_for_the_platforms_it_serves(self):
        generated = emitted(go, api())
        self.assertNotIn("UseMetal", generated["generated_api.go"])
        self.assertIn(
            "func (receiver *MapHandle) Redraw(", generated["generated_api.go"]
        )
        darwin = generated["generated_api_macos.go"]
        self.assertTrue(
            darwin.startswith(
                "// Code generated by tools/bindgen; DO NOT EDIT.\n//go:build darwin\n\npackage maplibre\n"
            )
        )
        self.assertIn("func (receiver *MapHandle) UseMetal(", darwin)
        self.assertIn("//go:build android\n", generated["generated_api_android.go"])
        self.assertNotIn("AttachCanvas", "".join(generated.values()))

    def test_dotnet_marks_the_platforms_that_a_function_serves(self):
        generated = "\n".join(emitted(dotnet, api()).values())
        self.assertIn(
            '    [global::System.Runtime.Versioning.SupportedOSPlatform("macos")]\n'
            "    public void UseMetal(",
            generated,
        )
        self.assertIn(
            '[global::System.Runtime.Versioning.SupportedOSPlatform("android")]\n'
            "    public static void AndroidStart(",
            generated,
        )
        self.assertNotIn("AttachCanvas", generated)

    def test_kotlin_puts_a_function_in_the_source_sets_of_its_targets(self):
        self.assertEqual(
            kotlin.source_sets(frozenset({"ios", "macos", "tvos", "visionos"})),
            ["jvmMain", "appleMain"],
        )
        self.assertEqual(
            kotlin.source_sets(
                frozenset({"android", "linux", "macos", "ohos", "windows"})
            ),
            ["jvmAndroidMain", "androidNativeMain", "macosMain", "linuxMain"],
        )
        self.assertEqual(kotlin.source_sets(frozenset({"web"})), ["wasmJsMain"])
        generated = emitted(kotlin, api())
        root = "org/maplibre/nativeffi"
        common = generated[f"src/commonMain/kotlin/{root}/MapHandle.kt"]
        self.assertIn("public fun redraw(", common)
        self.assertNotIn("useMetal", common)
        self.assertNotIn("attachCanvas", common)
        for source_set in ("jvmMain", "appleMain"):
            stem = source_set.removesuffix("Main")
            self.assertIn(
                "public fun MapHandle.useMetal(",
                generated[
                    f"src/{source_set}/kotlin/{root}/Generated{stem[0].upper()}{stem[1:]}Operations.kt"
                ],
            )
        self.assertIn(
            "public fun MapHandle.attachCanvas(",
            generated[f"src/wasmJsMain/kotlin/{root}/GeneratedWasmJsOperations.kt"],
        )


if __name__ == "__main__":
    unittest.main()
