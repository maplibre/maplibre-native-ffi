"""Compile header-driven Rust bindings with the binding's own runtime and run them."""

import importlib.util
import re
import shutil
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    PROTOCOLS_STUB,
    ROOT,
    emitted,
    parse,
    real_api,
    require_tool,
    run,
)

from tools.bindgen.emitters import rust, rust_sys
from tools.bindgen.emitters.rust import native_identifier
from tools.bindgen.schema import validate

CRATES = ROOT / "bindings/rust/crates"
SYS = CRATES / "maplibre-native-ffi-sys/src"
PROBE = FIXTURES / "probes/rust"

# Declarations the probe adds to the protocol fixture: a record and a command
# whose names collide with Rust keywords and the generated operations' locals.
COLLISIONS = """
typedef struct mln_new_point { double type; double self; double str; } mln_new_point;
BIND("execution=query;result=mln_new_point")
mln_status mln_map_match(mln_map map, const mln_completion *completion, mln_diagnostic *out_diagnostic);
BIND("execution=command")
mln_status mln_map_move(mln_map map, mln_new_point call, mln_new_point value, mln_new_point future, const mln_completion *completion, mln_diagnostic *out_diagnostic);
"""

# The functions that COLLISIONS declares, which the probe links but calls only
# to release its map.
COLLISIONS_STUB = """
#include <stdint.h>
typedef struct { double type, self, str; } point;
void mln_map_release(uint64_t map) { (void)map; }
int mln_map_match(uint64_t map, const void* completion, void* diagnostic) {
  (void)map; (void)completion; (void)diagnostic;
  return -4;
}
int mln_map_move(uint64_t map, point call, point value, point future,
                 const void* completion, void* diagnostic) {
  (void)map; (void)call; (void)value; (void)future; (void)completion;
  (void)diagnostic;
  return -4;
}
"""

# A dispose that is also its handle's release and takes an optional completion.
RELEASE_AND_DISPOSE = """
typedef unsigned long long mln_session BIND("kind=handle;release=mln_session_release;dispose=mln_session_dispose");
BIND("execution=lifecycle")
mln_status mln_session_release(mln_session session, const mln_completion *completion, mln_diagnostic *out_diagnostic);
mln_status mln_session_dispose(mln_session session, mln_diagnostic *out_diagnostic);
"""


# A drain that publishes an owned batch of strided readings with a slice view
# of it, as the render session's frame-result drain does.
SLICE_DRAIN = """
typedef struct mln_reading {
  double value;
} mln_reading;
typedef struct mln_reading_view {
  uint32_t size;
  uint32_t reading_size;
  const mln_reading* readings BIND("length=reading_count;stride=reading_size");
  size_t reading_count;
} mln_reading_view BIND("kind=slice");
typedef unsigned long long mln_reading_batch BIND(
  "kind=handle;release=mln_reading_batch_release;"
  "dispose=mln_reading_batch_release"
);
void mln_reading_batch_release(mln_reading_batch batch);
BIND("execution=event_batch")
mln_status mln_drain_readings(
  mln_reading_batch* out_batch BIND("direction=out;nullable=true"),
  mln_reading_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
"""


class RustEmitterTests(unittest.TestCase):
    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        api = parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        validate(api)
        generated = emitted(rust, api)
        prefix = "crates/maplibre-native-ffi/src/generated/"
        source = generated[prefix + "global.rs"]
        self.assertIn("pub fn drain_readings() -> Result<Vec<Reading>> {", source)
        # Nothing queued reads as empty. Otherwise the batch's guard releases
        # it once the copy returns, whether or not the copy succeeds.
        self.assertIn("    return Ok(Vec::new());", source)
        self.assertIn(
            "let _batch = unsafe { crate::handle::Drained::new(out_batch, "
            "sys::mln_reading_batch_release) };\n"
            "        Ok(unsafe { convert::copy_strided(out_view.readings, "
            "out_view.reading_count, out_view.reading_size) }?)",
            source,
        )
        self.assertNotIn(prefix + "reading_batch.rs", generated)
        self.assertNotIn("ReadingView", generated[prefix + "values.rs"])

    def test_a_callback_receives_its_slice_argument_as_a_vec(self):
        values = emitted(rust, real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        self.assertIn("Fn(Vec<RuntimeEvent>) + Send + Sync", values)
        self.assertIn(
            "convert::copy_strided(view.events, view.event_count, view.event_size)",
            values,
        )
        self.assertNotIn("EventBatchView", values)

    def test_an_integer_that_names_a_graphics_object_makes_its_call_unsafe(self):
        # Native code uses the objects that these integers name as it would
        # pointers. Add each new integer that names a graphics object here.
        api = real_api()
        typedefs = {typedef.name: typedef for typedef in api.typedefs}
        fields = {
            (record.name, field.name): field
            for record in api.records
            for field in record.fields
        }
        self.assertEqual(
            "true",
            typedefs["mln_vulkan_non_dispatchable_handle"].metadata.get(
                "native_object"
            ),
        )
        for name in [
            ("mln_gpu_sync", "handle"),
            ("mln_open_gl_borrowed_texture", "texture"),
            ("mln_web_gl_context_descriptor", "context"),
        ]:
            with self.subTest(field=name):
                self.assertEqual("true", fields[name].metadata.get("native_object"))
        files = emitted(rust, api)
        session = files["crates/maplibre-native-ffi/src/generated/render_session.rs"]
        frame = files["crates/maplibre-native-ffi/src/generated/acquired_frame.rs"]
        # A target names the host's objects, and a consumer sync carries only an
        # integer handle, with no pointer.
        self.assertIn("pub unsafe fn set_target(", session)
        self.assertIn("pub unsafe fn release(", frame)
        self.assertIn("pub fn resize(", session)

    def test_a_plain_variant_is_copied_into_its_union_without_parentheses(self):
        api = parse(groups=("value_union",))
        validate(api)
        values = emitted(rust, api)[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        # Clippy denies the parentheses that a bare `(*item)` would keep.
        self.assertIn("raw.data.count = *item;", values)
        self.assertIn("raw.data.text = to_native(&(*item), arena)?;", values)

    def test_a_record_with_a_required_member_takes_it_in_place_of_a_default(self):
        values = emitted(rust, real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        self.assertIn(
            "#[derive(Debug, Clone, PartialEq)]\npub struct MapOptions<'a> {", values
        )
        # The others start at zero, which native reads as their defaults.
        self.assertIn(
            "pub fn new(initial_extent: LogicalExtent) -> Self { Self { "
            "initial_extent, map_mode: Default::default(), "
            "fast_pfor_enabled: Default::default(), event_mask: None, "
            "tile_cache_enabled: None, fallback_font_faces: Default::default() } }",
            values,
        )
        self.assertIn(
            "#[derive(Debug, Clone, PartialEq, Default)]\npub struct RuntimeOptions<'a> {",
            values,
        )

    def test_the_c_version_is_a_plain_return_that_checks_no_abi_version(self):
        source = emitted(rust, real_api())[
            "crates/maplibre-native-ffi/src/generated/global.rs"
        ]
        self.assertIn(
            "pub fn c_version() -> u32 {\n        let mut call = Call::global();\n"
            "        let value = call.run(|_| unsafe { sys::mln_c_version() });",
            source,
        )
        self.assertIn(
            "pub fn supported_render_backend_mask() -> RenderBackendFlag {\n"
            "        let mut call = Call::global();\n"
            "        crate::abi::require_abi_version();",
            source,
        )

    def test_a_factory_is_associated_and_a_global_names_its_verb_first(self):
        generated = emitted(rust, real_api())
        prefix = "crates/maplibre-native-ffi/src/generated/"
        self.assertIn("impl RuntimeHandle {", generated[prefix + "runtime.rs"])
        self.assertIn(
            "    pub fn create(options: Option<&RuntimeOptions<'_>>) -> Result<RuntimeHandle> {",
            generated[prefix + "runtime.rs"],
        )
        source = generated[prefix + "global.rs"]
        self.assertNotIn("fn runtime_create(", source)
        self.assertIn(
            "    pub fn set_log_callback(handler: LogHandler) -> Result<()> {",
            source,
        )

    def test_a_callback_documents_its_arguments(self):
        values = emitted(rust, real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        # A closure bound cannot name its arguments, so the field's doc does.
        self.assertIn(
            "    /// The callback receives (`severity`, `event`, `code`, `message`).\n"
            "    pub callback: std::sync::Arc<dyn ",
            values,
        )

    def test_docs_name_the_rust_symbols_of_c_declarations(self):
        values = emitted(rust, real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        self.assertIn("`MapHandle::cancel_camera_transitions()`", values)
        self.assertIn("`MapHandle::cancel_camera_transitions_with_id()`", values)
        self.assertIn("`GesturePhase::Begin` or `GesturePhase::Cancel`", values)
        self.assertNotIn("`mln_map_cancel_camera_transitions()`", values)

    def test_a_status_callback_separates_conversion_from_host_failure(self):
        values = emitted(rust, real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        # A conversion's `?` fails the outer result, which is always reported;
        # host code's result is the inner one.
        self.assertIn(
            '"mln_http_header_transform_callback", sys::MLN_STATUS_NATIVE_ERROR, '
            "|| { Ok(callback::require(&state.callback)?(",
            values,
        )
        self.assertIn(
            "    /// An `Err` of `Error::Status` with a failure status answers native\n",
            values,
        )

    def test_a_release_waits_and_a_dispose_does_not(self):
        api = parse(RELEASE_AND_DISPOSE, defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",))
        validate(api)
        self.assertEqual(rust.lower(api).unsupported, {})
        source = emitted(rust, api)[
            "crates/maplibre-native-ffi/src/generated/session.rs"
        ]
        # Drop disposes, and only the release passes a completion.
        self.assertIn(
            "dispose |raw| crate::error::check(|out_diagnostic| unsafe { sys::mln_session_dispose(raw, out_diagnostic) })",
            source,
        )
        # Both consume the owner, and a refusal hands it back.
        self.assertIn(
            "pub fn release(self) -> std::result::Result<NativeFuture<()>, ReleaseError<Self>> {\n"
            "        self.consume(|session| {",
            source,
        )
        self.assertIn("call.complete(", source)
        self.assertIn(
            "pub fn dispose(self) -> std::result::Result<(), ReleaseError<Self>> {\n"
            "        self.consume(|session| {",
            source,
        )
        self.assertIn("sys::mln_session_dispose(session, out_diagnostic)", source)

    def test_a_release_keeps_after_release_functions_reachable(self):
        api = parse(
            RELEASE_AND_DISPOSE
            + """
BIND("after_release=true")
mln_status mln_session_service(mln_session session, mln_diagnostic *out_diagnostic);
""",
            defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",),
        )
        validate(api)
        self.assertEqual(rust.lower(api).unsupported, {})
        source = emitted(rust, api)[
            "crates/maplibre-native-ffi/src/generated/session.rs"
        ]
        # The release and the dispose consume the owner and hand back a value
        # that names the retiring handle without owning it.
        self.assertIn(
            "pub fn release(self) -> std::result::Result<(NativeFuture<()>, RetiringSession), ReleaseError<Self>> {",
            source,
        )
        self.assertIn(".map(|value| (value, RetiringSession(session)))", source)
        self.assertIn(
            "pub fn dispose(self) -> std::result::Result<RetiringSession, ReleaseError<Self>> {",
            source,
        )
        self.assertIn(".map(|()| RetiringSession(session))", source)
        self.assertIn("pub struct RetiringSession(sys::mln_session);", source)
        retiring = source[source.index("impl RetiringSession {") :]
        self.assertIn("pub fn service(&self) -> Result<()> {", retiring)
        self.assertIn("let mut call = Call::new(self.0);", retiring)

    def test_only_runtime_event_sources_document_their_id_as_a_source(self):
        api = parse(
            """
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release");
typedef unsigned long long mln_query BIND("kind=handle;release=mln_query_release");
void mln_map_release(mln_map map);
void mln_query_release(mln_query query);
"""
        )
        validate(api)
        generated = emitted(rust, api)
        prefix = "crates/maplibre-native-ffi/src/generated/"
        self.assertIn(
            "    /// Returns the native handle value, which runtime events report as their source.\n"
            "    fn id;\n",
            generated[prefix + "map.rs"],
        )
        self.assertIn(
            "    /// Returns the native handle value.\n    fn id;\n",
            generated[prefix + "query.rs"],
        )

    def test_generated_bindings_run_with_the_handwritten_runtime(self):
        defines = (
            "MLN_PROTOCOL_MAP_RELEASE",
            "MLN_PROTOCOL_COMPLETION_RUNTIME",
            "MLN_PROTOCOL_ABI_VERSION",
        )
        api = parse(
            COLLISIONS,
            groups=(
                "caller_sized_output",
                "caller_sized_text",
                "keywords",
                "absent_handle",
            ),
            defines=defines,
        )
        validate(api)
        self.assertEqual(
            set(rust.lower(api).generated),
            {
                "mln_c_version",
                "mln_probe_batch_points",
                "mln_probe_batch_scores",
                "mln_probe_zeros_json",
                "mln_probe_zeros_text",
                "mln_keyword_combine",
                "mln_map_match",
                "mln_map_move",
                "mln_map_release",
                "mln_probe_parcel_release",
                "mln_probe_take_parcel",
            },
        )
        with TemporaryDirectory() as directory:
            root = Path(directory)
            files = emitted(rust, api)
            # Parameters keep their C names, and the body's locals step aside.
            self.assertIn(
                "call: NewPoint, value: NewPoint, future: NewPoint",
                " ".join(
                    files["crates/maplibre-native-ffi/src/generated/map.rs"].split()
                ),
            )
            for path, source in files.items():
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text(source)
            sys_crate = root / "crates/maplibre-native-ffi-sys/src"
            safe_crate = root / "crates/maplibre-native-ffi/src"
            shutil.copy(SYS / "lib.rs", sys_crate / "lib.rs")
            for source in (CRATES / "maplibre-native-ffi/src").glob("*.rs"):
                shutil.copy(source, safe_crate / source.name)
            shutil.copy(PROBE / "probe.rs", safe_crate / "probe.rs")
            with (safe_crate / "lib.rs").open("a") as lib:
                lib.write("\nmod probe;\n\nfn main() {\n    probe::run();\n}\n")

            clang = require_tool(self, "clang")
            objects = []
            for name, source in (
                ("protocols_stub", PROTOCOLS_STUB),
                ("collisions_stub", None),
            ):
                if source is None:
                    source = root / f"{name}.c"
                    source.write_text(COLLISIONS_STUB)
                objects.append(root / f"{name}.o")
                clang.run(
                    self,
                    "-c",
                    "-fPIC",
                    *(f"-D{define}" for define in defines),
                    f"-I{FIXTURES}",
                    str(source),
                    "-o",
                    str(objects[-1]),
                    cwd=root,
                )
            rustc = require_tool(self, "rustc")
            rustc.run(
                self,
                "--edition=2024",
                "--crate-type=lib",
                "--crate-name=maplibre_native_ffi_sys",
                str(sys_crate / "lib.rs"),
                "--out-dir",
                str(root),
                cwd=root,
            )
            binary = root / "probe"
            rustc.run(
                self,
                "--edition=2024",
                "--crate-name=maplibre_native_ffi",
                f"-Ldependency={root}",
                "--extern",
                f"maplibre_native_ffi_sys={root / 'libmaplibre_native_ffi_sys.rlib'}",
                *(f"-Clink-arg={item}" for item in objects),
                str(safe_crate / "lib.rs"),
                "-o",
                str(binary),
                cwd=root,
            )
            run(self, [str(binary)], root)


def check_exports():
    """tools/lint/check-exports.py, which lists the library's exports."""
    spec = importlib.util.spec_from_file_location(
        "check_exports", ROOT / "tools/lint/check-exports.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class RustSysTests(unittest.TestCase):
    def test_declarations_cover_every_exported_function(self):
        """The crate declares exactly the functions the library exports, the
        delivery protocol and binding-support exports included."""
        exports = check_exports()
        expected = exports.declared_functions() | exports.EXTRA_EXPORTS
        declared = re.findall(
            r"\bpub fn (mln_\w+)\(",
            (SYS / "lib.rs").read_text() + (SYS / "generated.rs").read_text(),
        )
        self.assertEqual(len(declared), len(set(declared)))
        self.assertEqual(set(declared), expected)
        self.assertEqual(rust_sys.declarations(real_api()).unsupported, {})

    def test_declarations_match_the_c_layout(self):
        """Every emitted record, handle, and enum has the size, alignment, and
        field offsets that the C compiler gives the real headers."""
        api = real_api()
        layout = {
            "mln_diagnostic": ("size", "message"),
            **rust_sys.declarations(api).layout(),
        }
        records = {record.name: record.kind for record in api.records}
        with TemporaryDirectory() as directory:
            root = Path(directory)
            crate = root / "sys"
            crate.mkdir()
            (crate / "lib.rs").write_text((SYS / "lib.rs").read_text())
            (crate / "generated.rs").write_text(emitted(rust, api)[rust_sys.PATH])
            rust_lines, c_lines = [], []
            for name, fields in sorted(layout.items()):
                c_type = (
                    f"{records.get(name, 'struct')} {name}"
                    if name in records or name == "mln_diagnostic"
                    else name
                )
                rust_lines.append(
                    f'writeln!(out, "{name} {{}} {{}}", size_of::<sys::{name}>(), align_of::<sys::{name}>()).unwrap();'
                )
                c_lines.append(
                    f'fprintf(out, "{name} %zu %zu\\n", sizeof({c_type}), _Alignof({c_type}));'
                )
                for field in fields:
                    rust_lines.append(
                        f'writeln!(out, "{name}.{field} {{}}", std::mem::offset_of!(sys::{name}, {native_identifier(field)})).unwrap();'
                    )
                    c_lines.append(
                        f'fprintf(out, "{name}.{field} %zu\\n", offsetof({c_type}, {field}));'
                    )
            (root / "main.rs").write_text(
                "use std::io::Write;\nuse std::mem::{align_of, size_of};\n"
                "use maplibre_native_ffi_sys as sys;\nfn main() {\n"
                "    let mut out = std::fs::File::create(std::env::args().nth(1).unwrap()).unwrap();\n    "
                + "\n    ".join(rust_lines)
                + "\n}\n"
            )
            (root / "main.c").write_text(
                "#include <stddef.h>\n#include <stdio.h>\n"
                "#include <maplibre_native_c.h>\n"
                "#include <maplibre_native_c/plugin.h>\n"
                "int main(int argc, char** argv) {\n  (void)argc;\n"
                '  FILE* out = fopen(argv[1], "w");\n  '
                + "\n  ".join(c_lines)
                + "\n  return fclose(out);\n}\n"
            )
            rustc = require_tool(self, "rustc")
            rustc.run(
                self,
                "--edition=2024",
                "--crate-type=lib",
                "--crate-name=maplibre_native_ffi_sys",
                str(crate / "lib.rs"),
                "--out-dir",
                str(root),
                cwd=root,
            )
            rustc.run(
                self,
                "--edition=2024",
                "--extern",
                f"maplibre_native_ffi_sys={root / 'libmaplibre_native_ffi_sys.rlib'}",
                str(root / "main.rs"),
                "-o",
                str(root / "rust-layout"),
                cwd=root,
            )
            require_tool(self, "clang").run(
                self,
                "-std=c2x",
                f"-I{ROOT / 'include'}",
                f"-I{ROOT / 'include'}",
                str(root / "main.c"),
                "-o",
                str(root / "c-layout"),
                cwd=root,
            )
            run(self, [str(root / "rust-layout"), str(root / "rust.txt")], root)
            run(self, [str(root / "c-layout"), str(root / "c.txt")], root)
            rust_layout = (root / "rust.txt").read_text().splitlines()
            c_layout = (root / "c.txt").read_text().splitlines()
            self.assertGreater(len(c_layout), len(layout))
            self.assertEqual(rust_layout, c_layout)
