"""Shared fixtures, header parsing, and probe toolchains for generator tests."""

from __future__ import annotations

import functools
import hashlib
import os
import re
import shutil
import subprocess
import sys
import unittest
from pathlib import Path
from tempfile import TemporaryDirectory

from tools.bindgen.frontend import parse_headers
from tools.bindgen.model import Api

ROOT = Path(__file__).resolve().parents[2]
FIXTURES = Path(__file__).resolve().parent / "fixtures"
PROTOCOLS = FIXTURES / "protocols.h"
PROTOCOLS_STUB = FIXTURES / "protocols_stub.c"
NATIVE_INCLUDE = ROOT / "third_party/maplibre-native/include"
REAL_CLANG_ARGS = (f"-I{NATIVE_INCLUDE}",)

# CI sets this so that a probe whose toolchain is missing fails the job.
REQUIRE_TOOLCHAINS = os.environ.get("MLN_BINDGEN_REQUIRE_TOOLCHAINS") == "1"


def protocol_header(
    source: str = "", *, groups: tuple[str, ...] = (), defines: tuple[str, ...] = ()
) -> str:
    """The protocol fixture with `groups` enabled, followed by `source`."""
    lines = [f"#define MLN_PROTOCOL_{group.upper()}" for group in groups]
    lines.extend(f"#define {define}" for define in defines)
    return "\n".join([*lines, PROTOCOLS.read_text(), source, ""])


def protocol_groups() -> tuple[str, ...]:
    """The fixture's declaration groups, in file order."""
    names = re.findall(
        r"^#ifdef MLN_PROTOCOL_(\w+)$", PROTOCOLS.read_text(), re.MULTILINE
    )
    return tuple(
        dict.fromkeys(name.lower() for name in names if name != "STANDARD_TYPES")
    )


def parse(
    source: str = "",
    *,
    groups: tuple[str, ...] = (),
    defines: tuple[str, ...] = (),
    header: str = "api.h",
) -> Api:
    """Parse `source` after the protocol fixture as the one public header."""
    return parse_sources(
        {header: protocol_header(source, groups=groups, defines=defines)}
    )


def parse_sources(files: dict[str, str], clang_args: tuple[str, ...] = ()) -> Api:
    """Parse headers given as text, reusing the model of identical input."""
    with TemporaryDirectory() as directory:
        root = Path(directory)
        for name, text in files.items():
            (root / name).parent.mkdir(parents=True, exist_ok=True)
            (root / name).write_text(text)
        return parse_directory(root, clang_args)


def parse_directory(directory: Path, clang_args: tuple[str, ...] = ()) -> Api:
    """Parse a header directory once per distinct content and arguments.

    The model records locations relative to the directory, so identical input
    in another directory produces an equal model. Models are immutable, so
    tests share them.
    """
    key = _digest(directory, clang_args)
    if key not in _MODELS:
        _MODELS[key] = parse_headers(directory, clang_args=clang_args)
    return _MODELS[key]


def real_api() -> Api:
    """The repository's own public headers."""
    return parse_directory(ROOT / "include", REAL_CLANG_ARGS)


_MODELS: dict[str, Api] = {}


def _digest(directory: Path, clang_args: tuple[str, ...]) -> str:
    digest = hashlib.sha256()
    for argument in clang_args:
        digest.update(argument.encode() + b"\0")
    for path in sorted(directory.rglob("*")):
        if path.is_file() and path.suffix in {".h", ".toml"}:
            digest.update(path.relative_to(directory).as_posix().encode() + b"\0")
            digest.update(path.read_bytes() + b"\0")
    return digest.hexdigest()


def require_tool(
    test: unittest.TestCase, tool: str, directory: Path = ROOT
) -> list[str]:
    """The command prefix that runs `tool` with the tools configured at `directory`.

    A missing toolchain skips the test with the tool's name, or fails it when
    MLN_BINDGEN_REQUIRE_TOOLCHAINS=1.
    """
    command = _tool_command(tool, directory)
    if command is None:
        message = f"{tool} is not installed for {directory.relative_to(ROOT)}"
        if REQUIRE_TOOLCHAINS:
            test.fail(f"{message}; MLN_BINDGEN_REQUIRE_TOOLCHAINS=1 requires it")
        test.skipTest(message)
    return command


@functools.cache
def _tool_command(tool: str, directory: Path) -> list[str] | None:
    """Prefer the mise-configured tool; fall back to one on PATH."""
    candidates = []
    if shutil.which("mise"):
        candidates.append(["mise", "exec", "--no-deps", "--", tool])
    if shutil.which(tool):
        candidates.append([tool])
    for command in candidates:
        try:
            subprocess.run(
                [*command, "--version"],
                cwd=directory,
                env={**os.environ, "MISE_EXEC_AUTO_INSTALL": "false"},
                capture_output=True,
                check=True,
                timeout=60,
            )
        except OSError, subprocess.SubprocessError:
            continue
        if tool in {"swift", "swiftc"} and sys.platform.startswith("linux"):
            # The Linux toolchain links a libxml2 the host may lack.
            return [
                "bash",
                "-c",
                f'source "{ROOT}/.mise/bin/swift-libxml2-env.sh" && exec "$@"',
                tool,
                *command,
            ]
        return command
    return None


def run(
    test: unittest.TestCase,
    command: list[str],
    cwd: Path,
    *,
    timeout: float = 120,
    env: dict[str, str] | None = None,
) -> subprocess.CompletedProcess:
    """Run a probe step and fail the test with its output when it fails."""
    result = subprocess.run(
        command,
        cwd=cwd,
        capture_output=True,
        text=True,
        timeout=timeout,
        check=False,
        env=None if env is None else {**os.environ, **env},
    )
    test.assertEqual(
        result.returncode,
        0,
        f"{' '.join(command)}\n{result.stdout}{result.stderr}",
    )
    return result
