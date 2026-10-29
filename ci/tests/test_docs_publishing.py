"""Exercise release version selection and deployment URL reporting without publishing."""

from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import tempfile
import tomllib
import unittest
from pathlib import Path
from unittest import mock

from tools.ci.plan import ROOT
from tools.release.release_version import parse_version


class VersionsTest(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location(
            "docs_versions", ROOT / "docs/scripts/generate-versions.py"
        )
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.output = Path(self.directory.name) / "versions.json"
        patcher = mock.patch.object(self.module, "OUTPUT_PATH", self.output)
        patcher.start()
        self.addCleanup(patcher.stop)

    def test_release_version_does_not_change_when_newer_tags_exist(self):
        with (
            mock.patch.dict(os.environ, {"DOCS_VERSION": "0.202610.0"}),
            mock.patch.object(self.module, "release_tags") as tags,
        ):
            self.module.main()
        tags.assert_not_called()
        self.assertEqual(json.loads(self.output.read_text()), {"version": "0.202610.0"})

    def test_preview_uses_latest_release_version(self):
        tags = {
            "v0.202610.0": parse_version("0.202610.0"),
            "v0.202610.1": parse_version("0.202610.1"),
        }
        with (
            mock.patch.dict(os.environ, {}, clear=True),
            mock.patch.object(self.module, "release_tags", return_value=tags),
        ):
            self.module.main()
        self.assertEqual(json.loads(self.output.read_text()), {"version": "0.202610.1"})


class DeploymentTest(unittest.TestCase):
    def deploy(self, deployment: dict, *, exit_code: int = 0):
        task = tomllib.loads((ROOT / "docs/mise.toml").read_text())["tasks"][
            "deploy-cloudflare"
        ]
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            wrangler = root / "wrangler"
            wrangler.write_text(
                "#!/bin/sh\n"
                'printf "%s\\n" "$@" > "$DEPLOYMENT_ARGS"\n'
                'printf "%s\\n" "$DEPLOYMENT_JSON" > "$WRANGLER_OUTPUT_FILE_PATH"\n'
                'exit "$DEPLOYMENT_EXIT"\n'
            )
            wrangler.chmod(0o755)
            output, summary, args = (
                root / name for name in ("output", "summary", "args")
            )
            env = {
                **os.environ,
                "PATH": f"{root}{os.pathsep}{os.environ['PATH']}",
                "usage_branch": "pr-123",
                "GITHUB_OUTPUT": str(output),
                "GITHUB_STEP_SUMMARY": str(summary),
                "DEPLOYMENT_JSON": json.dumps(deployment),
                "DEPLOYMENT_ARGS": str(args),
                "DEPLOYMENT_EXIT": str(exit_code),
            }
            done = subprocess.run(
                ["bash", "-e", "-c", task["run"]],
                env=env,
                cwd=root,
                capture_output=True,
                text=True,
                check=False,
            )
            return (
                done.returncode,
                output.read_text() if output.exists() else "",
                summary.read_text() if summary.exists() else "",
                args.read_text().splitlines(),
            )

    def test_reports_branch_alias_and_keeps_the_site_base_path(self):
        code, output, summary, args = self.deploy(
            {
                "type": "pages-deploy-detailed",
                "alias": "https://pr-123.example.pages.dev",
                "url": "https://abc.example.pages.dev",
            }
        )
        self.assertEqual(code, 0)
        url = "https://pr-123.example.pages.dev/maplibre-native-ffi/"
        self.assertEqual(output, f"url={url}\n")
        self.assertIn(url, summary)
        self.assertEqual(
            args,
            [
                "pages",
                "deploy",
                "build/cloudflare-pages",
                "--project-name=maplibre-native-ffi",
                "--branch=pr-123",
            ],
        )

    def test_reports_deployment_url_when_no_branch_alias_exists(self):
        code, output, _, _ = self.deploy(
            {"type": "pages-deploy-detailed", "url": "https://abc.example.pages.dev"}
        )
        self.assertEqual(code, 0)
        self.assertEqual(
            output, "url=https://abc.example.pages.dev/maplibre-native-ffi/\n"
        )

    def test_failed_deployment_does_not_report_a_success_url(self):
        code, output, summary, _ = self.deploy({}, exit_code=1)
        self.assertNotEqual(code, 0)
        self.assertEqual(output, "")
        self.assertEqual(summary, "")
