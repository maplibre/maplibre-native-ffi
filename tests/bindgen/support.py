"""Shared fixtures, header parsing, and probe toolchains for generator tests."""

from __future__ import annotations

import functools
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import unittest
from dataclasses import dataclass
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


# Fixture switches that change the base declarations rather than add a shape.
_BASE_SWITCHES = {"STANDARD_TYPES", "COMPLETION_RUNTIME", "ABI_VERSION"}


def protocol_groups() -> tuple[str, ...]:
    """The fixture's declaration groups, in file order."""
    names = re.findall(
        r"^#ifdef MLN_PROTOCOL_(\w+)$", PROTOCOLS.read_text(), re.MULTILINE
    )
    return tuple(
        dict.fromkeys(name.lower() for name in names if name not in _BASE_SWITCHES)
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
    in another directory produces an equal model. Tests share each model, so
    neither a test nor the generator may change one. The dataclasses are
    frozen, but their `metadata` dicts are not, so code that adjusts metadata
    copies it first.
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


@dataclass(frozen=True)
class Tool:
    """A probe toolchain executable and the environment it runs in."""

    executable: str
    environment: tuple[tuple[str, str], ...]

    @property
    def env(self) -> dict[str, str]:
        return dict(self.environment)

    def run(
        self,
        test: unittest.TestCase,
        *arguments: str,
        cwd: Path,
        timeout: float = 120,
        env: dict[str, str] | None = None,
        output: Path | None = None,
    ) -> None:
        run(
            test,
            [self.executable, *arguments],
            cwd,
            timeout=timeout,
            env={**self.env, **(env or {})},
            output=output,
        )


def require_tool(test: unittest.TestCase, tool: str, directory: Path = ROOT) -> Tool:
    """The `tool` that the mise configuration at `directory` selects.

    The tool carries that configuration's environment, so it runs from any
    directory. A missing toolchain skips the test with the tool's name, or
    fails it when MLN_BINDGEN_REQUIRE_TOOLCHAINS=1.
    """
    found = _find_tool(tool, directory)
    if found is None:
        message = f"{tool} is not installed for {directory.relative_to(ROOT)}"
        if REQUIRE_TOOLCHAINS:
            test.fail(f"{message}; MLN_BINDGEN_REQUIRE_TOOLCHAINS=1 requires it")
        test.skipTest(message)
    return found


@functools.cache
def _find_tool(tool: str, directory: Path) -> Tool | None:
    environment = dict(os.environ)
    if shutil.which("mise"):
        loaded = subprocess.run(
            ["mise", "env", "--json"],
            cwd=directory,
            capture_output=True,
            text=True,
            check=False,
        )
        if loaded.returncode == 0:
            environment.update(json.loads(loaded.stdout))
        if tool in {"swift", "swiftc"} and sys.platform.startswith("linux"):
            # The Linux toolchain links a libxml2 that newer distributions lack;
            # .mise/bin/swift-libxml2-env.sh explains the pinned copy.
            libxml2 = subprocess.run(
                ["mise", "where", "conda:libxml2"],
                cwd=directory,
                capture_output=True,
                text=True,
                check=False,
            )
            if libxml2.returncode == 0:
                paths = [f"{libxml2.stdout.strip()}/lib"]
                paths += filter(None, [environment.get("LD_LIBRARY_PATH")])
                environment["LD_LIBRARY_PATH"] = os.pathsep.join(paths)
    executable = shutil.which(tool, path=environment.get("PATH"))
    if executable is None:
        return None
    try:
        subprocess.run(
            [executable, "version" if tool in {"go", "zig"} else "--version"],
            env=environment,
            capture_output=True,
            check=True,
            timeout=60,
        )
    except OSError, subprocess.SubprocessError:
        return None
    return Tool(executable, tuple(sorted(environment.items())))


def run(
    test: unittest.TestCase,
    command: list[str],
    cwd: Path,
    *,
    timeout: float = 120,
    env: dict[str, str] | None = None,
    output: Path | None = None,
) -> None:
    """Run a probe step and fail the test with its output when it fails.

    `output` collects the step's output in a file instead of a pipe, for tools
    that stall writing to a pipe.
    """
    with open(output or os.devnull, "w+") as log:
        result = subprocess.run(
            command,
            cwd=cwd,
            stdout=log if output else subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            timeout=timeout,
            check=False,
            env=None if env is None else {**os.environ, **env},
        )
        if output:
            log.seek(0)
            result.stdout = log.read()
    test.assertEqual(result.returncode, 0, f"{' '.join(command)}\n{result.stdout}")
