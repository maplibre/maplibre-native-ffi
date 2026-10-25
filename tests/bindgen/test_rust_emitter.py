"""Compile header-driven Rust bindings with the binding's own runtime and run them."""

import shutil
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    PROTOCOLS_STUB,
    ROOT,
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
DISPOSE_WITH_COMPLETION = """
typedef unsigned long long mln_session BIND("kind=handle;release=mln_session_dispose;dispose=mln_session_dispose");
BIND("execution=lifecycle")
mln_status mln_session_dispose(mln_session session, const mln_completion *completion, mln_diagnostic *out_diagnostic);
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
  mln_reading_batch* out_batch BIND("direction=out"),
  mln_reading_view* out_view BIND("direction=out"),
  mln_diagnostic* out_diagnostic
);
"""


class RustEmitterTests(unittest.TestCase):
    def test_a_drain_returns_its_slice_and_releases_its_batch(self):
        api = parse(SLICE_DRAIN, defines=("MLN_PROTOCOL_STANDARD_TYPES",))
        validate(api)
        generated = rust.generate(api)
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
        # Neither the batch nor its view is public.
        self.assertNotIn(prefix + "reading_batch.rs", generated)
        self.assertNotIn("ReadingView", generated[prefix + "values.rs"])

    def test_a_callback_receives_its_slice_argument_as_a_vec(self):
        values = rust.generate(real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        self.assertIn("Fn(Vec<RuntimeEvent>) + Send + Sync", values)
        self.assertIn(
            "convert::copy_strided(view.events, view.event_count, view.event_size)",
            values,
        )
        self.assertNotIn("EventBatchView", values)

    def test_a_callback_documents_its_arguments_and_returns_a_bool_result(self):
        values = rust.generate(real_api())[
            "crates/maplibre-native-ffi/src/generated/values.rs"
        ]
        # A closure bound cannot name its arguments, so the field's doc does.
        self.assertIn(
            "    /// The callback receives (`severity`, `event`, `code`, `message`).\n"
            "    pub callback: std::sync::Arc<dyn ",
            values,
        )
        self.assertIn("Fn(LogSeverity, LogEvent, i64, &str) -> bool", values)
        self.assertIn('callback::invoke("mln_log_callback", false, ||', values)

    def test_a_status_callback_separates_conversion_from_host_failure(self):
        values = rust.generate(real_api())[
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

    def test_a_dispose_with_a_completion_releases_and_disposes(self):
        api = parse(
            DISPOSE_WITH_COMPLETION, defines=("MLN_PROTOCOL_COMPLETION_RUNTIME",)
        )
        validate(api)
        self.assertEqual(rust.coverage(api)["unsupported"], {})
        source = rust.generate(api)[
            "crates/maplibre-native-ffi/src/generated/session.rs"
        ]
        # Drop and the sync dispose pass no completion; the release passes one.
        self.assertIn(
            "dispose |raw| maplibre_core::check(|out_diagnostic| unsafe { sys::mln_session_dispose(raw, std::ptr::null(), out_diagnostic) })",
            source,
        )
        # The async release reaches native on a closed handle, and the sync
        # dispose of a closed handle does nothing.
        self.assertIn(
            "pub fn release(&self) -> Result<NativeFuture<()>> {\n"
            "        self.inner.close(|session| {",
            source,
        )
        self.assertIn("call.complete(", source)
        self.assertIn(
            "pub fn dispose(&self) -> Result<()> {\n"
            "        self.inner.close_once(|session| {",
            source,
        )
        self.assertIn("call.discard(", source)

    def test_only_runtime_event_sources_document_their_id_as_a_source(self):
        api = parse(
            """
typedef enum mln_runtime_event_source_type : unsigned {
  MLN_RUNTIME_EVENT_SOURCE_MAP = 1,
} mln_runtime_event_source_type;
typedef unsigned long long mln_map BIND("kind=handle;release=mln_map_release");
typedef unsigned long long mln_query BIND("kind=handle;release=mln_query_release");
void mln_map_release(mln_map map);
void mln_query_release(mln_query query);
"""
        )
        validate(api)
        generated = rust.generate(api)
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
                "default_registration",
                "absent_handle",
                "absent_value",
            ),
            defines=defines,
        )
        validate(api)
        self.assertEqual(
            set(rust.coverage(api)["generated"]),
            {
                "mln_c_version",
                "mln_probe_batch_points",
                "mln_probe_batch_scores",
                "mln_probe_zeros_json",
                "mln_keyword_combine",
                "mln_map_match",
                "mln_map_move",
                "mln_map_release",
                "mln_probe_hooks_default",
                "mln_probe_parcel_release",
                "mln_probe_read_level",
                "mln_probe_take_parcel",
            },
        )
        with TemporaryDirectory() as directory:
            root = Path(directory)
            files = rust.generate(api)
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
            core_crate = root / "crates/maplibre-native-ffi-core/src"
            safe_crate = root / "crates/maplibre-native-ffi/src"
            shutil.copy(SYS / "lib.rs", sys_crate / "lib.rs")
            shutil.copytree(
                CRATES / "maplibre-native-ffi-core/src", core_crate, dirs_exist_ok=True
            )
            shutil.copy(PROBE / "core.rs", core_crate / "lib.rs")
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
            for name, crate, extern in (
                ("maplibre_native_ffi_sys", sys_crate, ()),
                (
                    "maplibre_native_ffi_core",
                    core_crate,
                    ("maplibre_native_ffi_sys",),
                ),
            ):
                rustc.run(
                    self,
                    "--edition=2024",
                    "--crate-type=lib",
                    f"--crate-name={name}",
                    f"-Ldependency={root}",
                    *(
                        argument
                        for dependency in extern
                        for argument in (
                            "--extern",
                            f"{dependency}={root / f'lib{dependency}.rlib'}",
                        )
                    ),
                    str(crate / "lib.rs"),
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
                *(
                    argument
                    for dependency in (
                        "maplibre_native_ffi_sys",
                        "maplibre_native_ffi_core",
                    )
                    for argument in (
                        "--extern",
                        f"{dependency}={root / f'lib{dependency}.rlib'}",
                    )
                ),
                *(f"-Clink-arg={item}" for item in objects),
                str(safe_crate / "lib.rs"),
                "-o",
                str(binary),
                cwd=root,
            )
            run(self, [str(binary)], root)


class RustSysTests(unittest.TestCase):
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
            (crate / "generated.rs").write_text(rust.generate(api)[rust_sys.PATH])
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
                f"-I{ROOT / 'third_party/maplibre-native/include'}",
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
