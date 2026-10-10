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


class RustEmitterTests(unittest.TestCase):
    def test_generated_bindings_run_with_the_handwritten_runtime(self):
        defines = (
            "MLN_PROTOCOL_MAP_RELEASE",
            "MLN_PROTOCOL_COMPLETION_RUNTIME",
            "MLN_PROTOCOL_ABI_VERSION",
        )
        api = parse(COLLISIONS, groups=("keywords",), defines=defines)
        validate(api)
        self.assertEqual(
            set(rust.coverage(api)["generated"]),
            {
                "mln_c_version",
                "mln_keyword_combine",
                "mln_map_match",
                "mln_map_move",
                "mln_map_release",
            },
        )
        with TemporaryDirectory() as directory:
            root = Path(directory)
            files = rust.generate(api)
            for path, source in files.items():
                (root / path).parent.mkdir(parents=True, exist_ok=True)
                (root / path).write_text(source)
            sys_crate = root / "crates/maplibre-native-ffi-sys/src"
            core_crate = root / "crates/maplibre-native-ffi-core/src"
            safe_crate = root / "crates/maplibre-native-ffi/src"
            shutil.copy(SYS / "lib.rs", sys_crate / "lib.rs")
            shutil.copytree(CRATES / "maplibre-native-ffi-core/src", core_crate)
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
