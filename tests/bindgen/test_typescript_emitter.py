"""Check a TypeScript drain and run the generated addon under Node."""

import shutil
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from support import (
    FIXTURES,
    PROTOCOLS_STUB,
    ROOT,
    emitted,
    parse_sources,
    protocol_header,
    require_tool,
)

from tools.bindgen.emitters import typescript
from tools.bindgen.schema import validate

BINDING = ROOT / "bindings/typescript"
PROBE = FIXTURES / "probes/typescript"

# The groups that protocols_stub.c implements, with the completion types that
# the addon runtime compiles against, and the groups that the probe's own
# results_stub.c implements.
GROUPS = (
    "caller_sized_output",
    "caller_sized_text",
    "values",
    "keywords",
    "defaults",
    "absent_handle",
    "strided_records",
    "map_close",
    "tagged_union",
    "completion_results",
    "counted_inline_array",
)
DEFINES = ("MLN_PROTOCOL_COMPLETION_RUNTIME", "MLN_PROTOCOL_ABI_VERSION")


class TypeScriptEmitterTests(unittest.TestCase):
    def run_probe(self, script: str):
        """Builds the generated addon against the protocol stubs and runs
        `script` from the probe directory under Node."""
        node = require_tool(self, "node", BINDING)
        clang = require_tool(self, "clang")
        headers = BINDING / "node_modules/node-api-headers/include"
        self.assertTrue(
            (headers / "node_api.h").exists(),
            "install the TypeScript binding's packages for node-api-headers",
        )
        header = protocol_header(groups=GROUPS, defines=DEFINES)
        api = parse_sources({"api.h": header})
        validate(api)
        self.assertEqual(typescript.lower(api).unsupported, {})
        with TemporaryDirectory() as directory:
            root = Path(directory)
            for name, text in emitted(typescript, api).items():
                target = root / Path(name).name
                if "/src/" in name:
                    target = root / "src" / Path(name).name
                target.parent.mkdir(parents=True, exist_ok=True)
                target.write_text(text)
            (root / "api.h").write_text(header)
            shutil.copytree(PROBE / "maplibre_native_c", root / "maplibre_native_c")
            shutil.copy(PROBE / "maplibre_native_c.h", root)
            shutil.copytree(BINDING / "src/internal", root / "src/internal")
            shutil.copy(BINDING / "src/index.ts", root / "src")
            shutil.copy(PROBE / script, root / "src")
            (root / "src/package.json").write_text('{"type": "module"}\n')
            # The stub implements the same groups with the completion types.
            stub = root / "stub.c"
            stub.write_text(
                "".join(f"#define {define}\n" for define in DEFINES)
                + f'#include "{PROTOCOLS_STUB}"\n'
            )
            addon = root / "probe.node"
            clang.run(
                self,
                "-std=c11",
                "-shared",
                "-fPIC",
                "-Wall",
                "-Wextra",
                "-Wno-unused-parameter",
                "-Werror",
                "-DNAPI_VERSION=8",
                *(("-undefined", "dynamic_lookup") if sys.platform == "darwin" else ()),
                f"-I{root}",
                f"-I{FIXTURES}",
                f"-I{BINDING / 'native'}",
                f"-I{headers}",
                str(root / "generated_addon.c"),
                str(root / "generated_values.c"),
                str(BINDING / "native/realm.c"),
                str(BINDING / "native/runtime.c"),
                str(PROBE / "queue_stub.c"),
                str(PROBE / "results_stub.c"),
                str(stub),
                "-o",
                str(addon),
                cwd=root,
            )
            node.run(
                self,
                str(root / "src" / script),
                cwd=root,
                env={"MAPLIBRE_NATIVE_FFI_ADDON": str(addon)},
            )

    def test_protocol_probe_runs_under_node(self):
        self.run_probe("main.ts")

    def test_generated_decoders_keep_unknown_arms_and_read_at_the_reported_stride(
        self,
    ):
        self.run_probe("decoders.ts")


if __name__ == "__main__":
    unittest.main()
