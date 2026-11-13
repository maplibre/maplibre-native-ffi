"""Check the Python extension's raw declarations against the C library."""

import importlib.util
import re
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import ROOT, emitted, real_api, require_tool, run

from tools.bindgen.emitters import python, python_sys

SYS = ROOT / "bindings/python/sys/src"


def check_exports():
    """tools/lint/check-exports.py, which lists the library's exports."""
    spec = importlib.util.spec_from_file_location(
        "check_exports", ROOT / "tools/lint/check-exports.py"
    )
    assert spec is not None and spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class PythonSysTests(unittest.TestCase):
    def test_declarations_cover_every_exported_function(self):
        """The crate declares exactly the functions the library exports, the
        delivery protocol and binding-support exports included."""
        exports = check_exports()
        expected = exports.declared_functions()
        declared = re.findall(
            r"\bpub fn (mln_\w+)\(",
            (SYS / "lib.rs").read_text() + (SYS / "generated.rs").read_text(),
        )
        self.assertEqual(len(declared), len(set(declared)))
        self.assertEqual(set(declared), expected)
        self.assertEqual(python_sys.declarations(real_api()).unsupported, {})

    def test_declarations_match_the_c_layout(self):
        """Every emitted record, handle, and enum has the size, alignment, and
        field offsets that the C compiler gives the real headers."""
        api = real_api()
        layout = {
            "mln_diagnostic": ("size", "message"),
            **python_sys.declarations(api).layout(),
        }
        records = {record.name: record.kind for record in api.records}
        with TemporaryDirectory() as directory:
            root = Path(directory)
            crate = root / "sys"
            crate.mkdir()
            (crate / "lib.rs").write_text((SYS / "lib.rs").read_text())
            (crate / "generated.rs").write_text(emitted(python, api)[python_sys.PATH])
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
                        f'writeln!(out, "{name}.{field} {{}}", std::mem::offset_of!(sys::{name}, {python_sys.identifier(field)})).unwrap();'
                    )
                    c_lines.append(
                        f'fprintf(out, "{name}.{field} %zu\\n", offsetof({c_type}, {field}));'
                    )
            (root / "main.rs").write_text(
                "use std::io::Write;\nuse std::mem::{align_of, size_of};\n"
                "use maplibre_native_ffi_python_sys as sys;\nfn main() {\n"
                "    let mut out = std::fs::File::create(std::env::args().nth(1).unwrap()).unwrap();\n    "
                + "\n    ".join(rust_lines)
                + "\n}\n"
            )
            (root / "main.c").write_text(
                "#include <stddef.h>\n#include <stdio.h>\n"
                "#include <maplibre_native_c.h>\n"
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
                "--crate-name=maplibre_native_ffi_python_sys",
                str(crate / "lib.rs"),
                "--out-dir",
                str(root),
                cwd=root,
            )
            rustc.run(
                self,
                "--edition=2024",
                "--extern",
                f"maplibre_native_ffi_python_sys={root / 'libmaplibre_native_ffi_python_sys.rlib'}",
                str(root / "main.rs"),
                "-o",
                str(root / "rust-layout"),
                cwd=root,
            )
            require_tool(self, "clang").run(
                self,
                "-std=c2x",
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
