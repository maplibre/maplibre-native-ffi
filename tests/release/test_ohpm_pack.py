from __future__ import annotations

import json
import os
import pathlib
import shutil
import subprocess
import tarfile
import tempfile
import unittest

from sbom import REPOSITORY, component, document


class OhpmPackTest(unittest.TestCase):
    def test_facade_and_runtime_archives(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = pathlib.Path(temporary)
            binding = root / "bindings/typescript"
            (binding / "scripts").mkdir(parents=True)
            shutil.copyfile(
                REPOSITORY / "bindings/typescript/scripts/pack-ohpm.sh",
                binding / "scripts/pack-ohpm.sh",
            )
            shutil.copytree(REPOSITORY / "bindings/typescript/ohpm", binding / "ohpm")
            for name in ("LICENSE", "CMakePresets.json", "tools"):
                (root / name).symlink_to(REPOSITORY / name)
            # The existing ArkTS suite tests the real bundle. Supply only its
            # output files here to exercise packing without a JavaScript build.
            binaries = root / "bin"
            binaries.mkdir()
            bundler = binaries / "pnpm"
            bundler.write_text(
                '#!/bin/sh\nmkdir -p "$MAPLIBRE_NATIVE_FFI_OHPM_BUNDLE"\n'
                'touch "$MAPLIBRE_NATIVE_FFI_OHPM_BUNDLE/index.js" '
                '"$MAPLIBRE_NATIVE_FFI_OHPM_BUNDLE/index.d.ts"\n'
            )
            bundler.chmod(0o755)
            presets = ("ohos-arm64", "ohos-x64")
            for preset in presets:
                install = binding / "native/build" / preset / "install"
                for name in (
                    "libmaplibre_native_ffi.so",
                    "libmaplibre-native-c.so",
                    "libc++_shared.so",
                    "include/maplibre_native_c.h",
                    "licenses/native.txt",
                ):
                    target = install / name
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.touch()
                native = component(preset, "0.0.0", "BSD-2-Clause")
                native["properties"] = [
                    {"name": "maplibre:source-commit", "value": "a" * 40}
                ]
                inventory = install / "sbom/maplibre-native-ffi.cdx.json"
                inventory.parent.mkdir()
                inventory.write_text(json.dumps(document(native, [], {})))
            output = root / "out"
            subprocess.run(
                ["bash", str(binding / "scripts/pack-ohpm.sh"), str(output), *presets],
                env={
                    **os.environ,
                    "PATH": f"{binaries}{os.pathsep}{os.environ['PATH']}",
                },
                check=True,
                capture_output=True,
            )
            names = ("native-ffi", "native-ffi-runtime")
            self.assertEqual(
                {p.name for p in output.iterdir()},
                {f"{name}-0.0.0.har" for name in names},
            )
            module_names = set()
            for name in names:
                with (
                    self.subTest(package=name),
                    tarfile.open(output / f"{name}-0.0.0.har") as archive,
                ):
                    manifest = json.load(
                        archive.extractfile("package/oh-package.json5")
                    )
                    module = json.load(
                        archive.extractfile("package/src/main/module.json")
                    )
                    self.assertEqual(manifest["name"], f"@maplibre/{name}")
                    self.assertEqual(
                        module["app"]["minAPIVersion"], manifest["compatibleSdkVersion"]
                    )
                    module_names.add(module["module"]["name"])
                    # OHPM rejects a package whose README has no line that
                    # installs it.
                    readme = archive.extractfile("package/README.md").read().decode()
                    self.assertIn(f"ohpm install @maplibre/{name}", readme.splitlines())
                    files = archive.getnames()
                    if name == "native-ffi":
                        self.assertIn("package/index.js", files)
                        self.assertFalse(any(p.endswith(".so") for p in files))
                        self.assertNotIn("nativeComponents", manifest)
                        self.assertIn(
                            "libmaplibre_native_ffi.so", manifest["dependencies"]
                        )
                    else:
                        # One runtime carries every ABI that the presets name.
                        for abi in ("arm64-v8a", "x86_64"):
                            self.assertIn(
                                f"package/libs/{abi}/libmaplibre_native_ffi.so", files
                            )
                        self.assertNotIn("package/index.js", files)
                        self.assertNotIn("dependencies", manifest)
            self.assertEqual(len(module_names), 2)
