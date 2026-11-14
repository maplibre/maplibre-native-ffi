"""Prepares fixtures for, builds, runs, and compares the C API benchmarks.

    bench.py fixtures
    bench.py run [PRESET] [--side SIDE] [--cmake-arg ARG]... [options]
    bench.py compare BASE [PRESET] [--candidate SIDE] [options]

A side is what one set of numbers measures:

- `worktree`: this checkout, with its uncommitted changes.
- a Git ref: that commit, checked out in a worktree under build/bench that the
  next comparison reuses, with this checkout's benchmarks/ laid over it.
- `build:DIR`: a CMake build tree of the repository, built already.
- `install:DIR`: a native install for this host, such as another
  implementation's.

Extra CMake arguments configure a ref or the worktree in a build directory of
their own. Only the fixtures step and a new worktree's `mise deps` use the
network, and neither runs while anything measures.
"""

from __future__ import annotations

import argparse
import base64
import colorsys
import concurrent.futures
import dataclasses
import datetime
import hashlib
import json
import math
import os
import platform
import re
import shutil
import statistics
import struct
import subprocess
import sys
import tempfile
import urllib.request
import zlib
from pathlib import Path, PurePosixPath

HARNESS = Path(__file__).resolve().parent
ROOT = HARNESS.parent
MANIFEST = HARNESS / "fixtures.json"
BENCH_DIR = ROOT / "build" / "bench"
GOOGLE_BENCHMARK = BENCH_DIR / "google-benchmark"
# The Google Benchmark commit that the harness builds against, the one that
# MapLibre Native vendored when the harness came about.
GOOGLE_BENCHMARK_URL = "https://github.com/google/benchmark.git"
GOOGLE_BENCHMARK_COMMIT = "344117638c8ff7e239044fd0fa7085839fc03021"
JSON_BEGIN = "MLN_FFI_BENCH_JSON_BEGIN"
JSON_END = "MLN_FFI_BENCH_JSON_END"
IMAGE_BEGIN = "MLN_FFI_BENCH_IMAGE_BEGIN "
IMAGE_END = "MLN_FFI_BENCH_IMAGE_END"
# Every case once takes a few minutes on a host; an emulator takes longer.
RUN_TIMEOUT_SECONDS = 3600


def log(message: str) -> None:
    print(f"bench: {message}", file=sys.stderr, flush=True)


def git(root: Path, *arguments: str) -> str:
    return subprocess.run(
        ["git", *arguments], cwd=root, check=True, stdout=subprocess.PIPE, text=True
    ).stdout.strip()


def run(command: list[str], cwd: Path, env: dict[str, str] | None = None) -> None:
    log("$ " + " ".join(command))
    subprocess.run(command, cwd=cwd, env=env, check=True)


def timestamp() -> str:
    return datetime.datetime.now().strftime("%Y%m%d-%H%M%S")


def write_json(path: Path, data: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(data, indent=2) + "\n")


# Fixtures


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fetch(url: str, target: Path, expected: str) -> None:
    request = urllib.request.Request(
        url, headers={"User-Agent": "maplibre-native-ffi-benchmarks"}
    )
    with urllib.request.urlopen(request, timeout=60) as response:
        data = response.read()
    actual = hashlib.sha256(data).hexdigest()
    if actual != expected:
        raise SystemExit(
            f"{url} has sha256 {actual}, but fixtures.json pins {expected}"
        )
    target.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.NamedTemporaryFile(dir=target.parent, delete=False) as temporary:
        temporary.write(data)
    os.replace(temporary.name, target)


GLYPH_RANGE = re.compile(r"[0-9]+-[0-9]+\.pbf")


def prepare_glyphs(glyphs: dict, directory: Path) -> None:
    """Extracts one font's glyph ranges from the pinned font archive, unless a
    stamp says that this archive's are there. The archive is untrusted, so
    only its plain range files under the font's directory are kept."""
    font = glyphs["font"]
    stamp = directory / font / ".sha256"
    if stamp.is_file() and stamp.read_text().strip() == glyphs["sha256"]:
        return
    log(f"downloading {glyphs['url']}")
    BENCH_DIR.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=BENCH_DIR) as temporary:
        archive = Path(temporary) / "fonts.tar.gz"
        fetch(glyphs["url"], archive, glyphs["sha256"])

        names, lines = (
            subprocess.run(
                ["tar", flags, str(archive)], check=True, stdout=subprocess.PIPE
            )
            .stdout.decode()
            .splitlines()
            for flags in ("-tzf", "-tvzf")
        )
        # Refuses paths that leave the extraction directory, and links where
        # the font's files go.
        for name, line in zip(names, lines, strict=True):
            path = PurePosixPath(name)
            link = path.parts[:1] == (font,) and line[:1] not in ("-", "d")
            if path.is_absolute() or ".." in path.parts or ":" in name or link:
                raise SystemExit(f"{glyphs['url']} holds an unsafe entry: {name}")
        extracted = Path(temporary) / "extracted"
        extracted.mkdir()
        run(["tar", "-xzf", str(archive), "-C", str(extracted), font], cwd=ROOT)
        target = directory / font
        if target.exists():
            shutil.rmtree(target)
        target.mkdir(parents=True)
        for path in (extracted / font).iterdir():
            if (
                path.is_file()
                and not path.is_symlink()
                and GLYPH_RANGE.fullmatch(path.name)
            ):
                shutil.copyfile(path, target / path.name)
    stamp.write_text(glyphs["sha256"] + "\n")


def prepare_fixtures() -> Path:
    """Downloads the pinned files that are missing or changed, and writes the
    harness's own files beside them."""
    manifest = json.loads(MANIFEST.read_text())
    directory = BENCH_DIR / "fixtures" / manifest["version"]
    missing = {
        name: digest
        for name, digest in manifest["files"].items()
        if not (directory / name).is_file() or sha256(directory / name) != digest
    }
    if missing:
        log(f"downloading {len(missing)} fixture files into {directory}")
        with concurrent.futures.ThreadPoolExecutor(8) as pool:
            futures = [
                pool.submit(
                    fetch, f"{manifest['base_url']}/{name}", directory / name, digest
                )
                for name, digest in missing.items()
            ]
            for future in futures:
                future.result()
    prepare_glyphs(manifest["glyphs"], directory / "basemap" / "glyphs")
    shutil.copyfile(HARNESS / "style.json", directory / "style.json")
    write_sprite(directory / "basemap")
    (directory / "version").write_text(manifest["version"] + "\n")
    return directory


# The style's icons, which the snapshot lacks: one marker of its own color per
# name, so that sprite loading, the icon atlas, and icon collision run as they
# do for a basemap's icons.
SPRITE_ICONS = (
    "food",
    "cafe",
    "bar",
    "shop",
    "school",
    "health",
    "bank",
    "parking",
    "bus",
    "rail",
    "park",
    "worship",
    "culture",
    "lodging",
    "toilets",
    "poi",
)
SPRITE_ICON_SIZE = 17


def write_sprite(directory: Path) -> None:
    """Writes each icon as a colored disc in a white ring."""
    columns, size = 4, SPRITE_ICON_SIZE
    width = columns * size
    height = math.ceil(len(SPRITE_ICONS) / columns) * size
    pixels = bytearray(width * height * 4)
    center = (size - 1) / 2
    index_json = {}
    for index, name in enumerate(SPRITE_ICONS):
        color = bytes(
            round(channel * 255)
            for channel in colorsys.hsv_to_rgb(index / len(SPRITE_ICONS), 0.7, 0.8)
        )
        left, top = (index % columns) * size, (index // columns) * size
        for y in range(size):
            for x in range(size):
                distance = math.hypot(x - center, y - center) / center
                if distance <= 1.0:
                    at = ((top + y) * width + left + x) * 4
                    pixel = color if distance <= 0.72 else b"\xff\xff\xff"
                    pixels[at : at + 4] = pixel + b"\xff"
        index_json[name] = {
            "x": left,
            "y": top,
            "width": size,
            "height": size,
            "pixelRatio": 1,
        }
    directory.mkdir(parents=True, exist_ok=True)
    (directory / "sprite.png").write_bytes(encode_png(width, height, bytes(pixels)))
    write_json(directory / "sprite.json", index_json)


# Sides


@dataclasses.dataclass
class Side:
    """One configuration to measure, and once built, how to run it."""

    label: str
    spec: str
    cmake_args: list[str]
    checkout: Path | None = None
    commit: str | None = None
    build_dir: Path | None = None
    install: Path | None = None
    descriptor: dict | None = None

    def describe(self) -> str:
        text = self.spec
        if self.commit:
            text += f" ({self.commit[:12]})"
        if self.cmake_args:
            text += " " + " ".join(self.cmake_args)
        return text


def checkout_env(root: Path) -> dict[str, str]:
    """The environment for mise commands in `root`. A checkout under this one's
    build directory stops mise from reading this checkout's configuration, and
    trusts its own for this call alone."""
    kept = {
        "MISE_ENV",
        "MISE_DATA_DIR",
        "MISE_CACHE_DIR",
        "MISE_CONFIG_DIR",
        "MISE_STATE_DIR",
    }
    env = {
        name: value
        for name, value in os.environ.items()
        if name in kept or not name.startswith(("MISE_", "usage_"))
    }
    if root != ROOT:
        env["MISE_CEILING_PATHS"] = str(root.parent)
        env["MISE_TRUSTED_CONFIG_PATHS"] = str(root)
    return env


def prepare_worktree(path: Path, commit: str) -> None:
    """Checks `commit` out in the persistent worktree at `path`, and lays this
    checkout's benchmarks/ over it."""
    if not (path / ".git").exists():
        path.parent.mkdir(parents=True, exist_ok=True)
        run(["git", "worktree", "add", "--detach", str(path), commit], cwd=ROOT)
    elif git(path, "rev-parse", "HEAD") != commit or git(
        path, "status", "--porcelain", "--untracked-files=no", "--", ":!benchmarks"
    ):
        run(["git", "checkout", "--detach", "--force", commit], cwd=path)
    overlay = path / "benchmarks"
    if overlay.exists():
        shutil.rmtree(overlay)
    shutil.copytree(HARNESS, overlay, ignore=shutil.ignore_patterns("__pycache__"))
    # git submodule update skips the engine; mise deps checks it out at the
    # commit's pin and applies the commit's patches.
    run(["mise", "deps"], cwd=path, env=checkout_env(path))


def resolve_sides(specs: list[tuple[str, str, list[str]]]) -> list[Side]:
    """Turns (label, spec, CMake arguments) into sides, and checks out the refs.
    Sides at one commit share a worktree."""
    sides = []
    worktrees: dict[str, Path] = {}
    for label, spec, cmake_args in specs:
        side = Side(label, spec, list(cmake_args))
        if spec == "worktree":
            side.checkout = ROOT
            side.commit = git(ROOT, "rev-parse", "HEAD")
        elif spec.startswith("build:"):
            side.build_dir = Path(spec.removeprefix("build:")).resolve()
        elif spec.startswith("install:"):
            side.install = Path(spec.removeprefix("install:")).resolve()
            check_host_install(side.install)
        else:
            commit = git(ROOT, "rev-parse", "--verify", f"{spec}^{{commit}}")
            side.commit = commit
            if commit not in worktrees:
                slot = "candidate" if label == "candidate" else "baseline"
                worktrees[commit] = BENCH_DIR / slot
                prepare_worktree(worktrees[commit], commit)
            side.checkout = worktrees[commit]
        if side.cmake_args and side.checkout is None:
            raise SystemExit(f"{spec}: CMake arguments need a ref or the worktree")
        sides.append(side)
    return sides


def check_host_install(install: Path) -> None:
    """An install side builds the harness with this host's compiler, so it
    must be an install for this host."""
    artifact = install / "share" / "maplibre-native-c" / "artifact.json"
    if not artifact.is_file():
        raise SystemExit(f"install:{install} has no {artifact}")
    platform_id = json.loads(artifact.read_text())["platform"]["id"]
    machine = platform.machine().lower()
    arch = {"x86_64": "x64", "amd64": "x64", "aarch64": "arm64"}.get(machine, machine)
    prefixes = {
        "linux": ("linux-gnu-",),
        "darwin": ("macos-",),
        "windows": ("windows-", "mingw-"),
        "freebsd": ("freebsd-",),
    }.get(platform.system().lower(), ())
    if not (platform_id.startswith(prefixes) and platform_id.endswith("-" + arch)):
        raise SystemExit(
            f"install:{install} is for {platform_id}, but install: sides run on "
            "this host only; for another target, pass a ref or build:DIR with "
            "that target's preset"
        )


def mise_exec(root: Path) -> list[str]:
    wrapper = root / ".mise" / "bin" / "with-sccache-basedirs"
    return ["mise", "exec", "--", *([str(wrapper)] if wrapper.exists() else [])]


def cache_value(build_dir: Path, name: str) -> str | None:
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return None
    for line in cache.read_text(errors="replace").splitlines():
        key, _, value = line.partition("=")
        if key.split(":")[0] == name:
            return value
    return None


def build_library(side: Side, preset: str) -> None:
    """Builds and installs the library for a ref or the worktree."""
    assert side.checkout is not None
    root = side.checkout
    env = checkout_env(root)
    if not side.cmake_args:
        side.build_dir = root / "build" / preset
        run(["mise", "run", "build", preset], cwd=root, env=env)
        return
    # The same preset with other settings needs a build directory, and an
    # install, of its own.
    digest = hashlib.sha256("\0".join(side.cmake_args).encode()).hexdigest()[:10]
    side.build_dir = root / "build" / f"bench-{preset}-{digest}"
    build = str(side.build_dir)
    prefix = mise_exec(root)
    install = f"-DCMAKE_INSTALL_PREFIX={side.build_dir / 'install'}"
    run(
        [*prefix, "cmake", "--preset", preset, "-B", build, install, *side.cmake_args],
        cwd=root,
        env=env,
    )
    run(
        [*prefix, "cmake", "--build", build, "--target", "install"]
        + ["--config", "Release", "--parallel"],
        cwd=root,
        env=env,
    )
    run(
        [*prefix, "cmake", "--install", build, "--component", "test-graphics"],
        cwd=root,
        env=env,
    )


def build_harness(side: Side, preset: str, fixtures: Path) -> None:
    """Builds the harness into the side's build, or against its install, and
    reads how to run it."""
    # Every side builds the harness against this checkout's copy of Google
    # Benchmark, which the first run fetches at its pinned commit.
    if not (GOOGLE_BENCHMARK / "src" / "benchmark.cc").is_file():
        GOOGLE_BENCHMARK.mkdir(parents=True, exist_ok=True)
        run(["git", "init", "--quiet"], cwd=GOOGLE_BENCHMARK)
        run(
            ["git", "fetch", "--quiet", "--depth", "1", GOOGLE_BENCHMARK_URL]
            + [GOOGLE_BENCHMARK_COMMIT],
            cwd=GOOGLE_BENCHMARK,
        )
        run(["git", "checkout", "--quiet", "FETCH_HEAD"], cwd=GOOGLE_BENCHMARK)
    common = [
        f"-DMLN_BENCH_FIXTURE_DIR={fixtures}",
        f"-DMLN_BENCH_GOOGLE_BENCHMARK_DIR={GOOGLE_BENCHMARK}",
    ]
    if side.install is not None:
        digest = hashlib.sha256(str(side.install).encode()).hexdigest()[:10]
        side.build_dir = BENCH_DIR / f"standalone-{digest}"
        configure = [
            "cmake",
            "-S",
            str(HARNESS),
            "-B",
            str(side.build_dir),
            "-G",
            "Ninja",
            "-DCMAKE_BUILD_TYPE=Release",
            f"-DMLN_BENCH_INSTALL={side.install}",
            "-DMLN_BENCH_VULKAN_INCLUDE_DIR="
            + str(ROOT / "third_party" / "Vulkan-Headers" / "include"),
            *common,
        ]
    else:
        assert side.build_dir is not None
        hook = (side.checkout or ROOT) / "benchmarks" / "cmake" / "hook.cmake"
        # The cache keeps the setting, so it names a stub that includes the
        # hook only while the hook exists, and a later build of the tree at a
        # commit without benchmarks/ configures as if it had never been there.
        stub = side.build_dir / "mln-bench-hook.cmake"
        stub.parent.mkdir(parents=True, exist_ok=True)
        stub.write_text(
            "# Written by benchmarks/bench.py; see benchmarks/cmake/hook.cmake.\n"
            f"include([[{hook.as_posix()}]] OPTIONAL)\n"
        )
        configure = [
            "cmake",
            f"-DCMAKE_PROJECT_maplibre_native_c_INCLUDE={stub}",
            *common,
            str(side.build_dir),
        ]
    cwd = side.checkout or ROOT
    env = checkout_env(cwd)
    prefix = mise_exec(ROOT)
    run([*prefix, *configure], cwd=cwd, env=env)
    run(
        [*prefix, "cmake", "--build", str(side.build_dir), "--target", "mln_ffi_bench"]
        + ["--config", "Release", "--parallel"],
        cwd=cwd,
        env=env,
    )
    descriptor = side.build_dir / "benchmarks" / "bench.json"
    if not descriptor.is_file():
        raise SystemExit(f"{descriptor} is missing; the harness did not configure")
    side.descriptor = json.loads(descriptor.read_text())
    if side.install is None:
        side.install = Path(
            cache_value(side.build_dir, "CMAKE_INSTALL_PREFIX")
            or side.build_dir / "install"
        )


def build_side(side: Side, preset: str, fixtures: Path) -> None:
    if side.checkout is not None:
        build_library(side, preset)
    build_harness(side, preset, fixtures)


# Running


def harness_arguments(args: argparse.Namespace, images: bool) -> list[str]:
    arguments = [
        f"--benchmark_repetitions={args.repetitions}",
        f"--mln_iteration_scale={args.iteration_scale}",
        f"--mln_hz={args.hz}",
        f"--mln_target={args.target}",
    ]
    if not images:
        arguments.append("--mln_images=0")
    if args.filter:
        arguments.append(f"--benchmark_filter={args.filter}")
    return arguments


def apply_environment(env: dict[str, str], entries: list[str]) -> None:
    """Applies the native suites' environment, as CTest would: VAR=value, or
    VAR=path_list_prepend:directory."""
    for entry in entries:
        name, _, value = entry.partition("=")
        operation, _, operand = value.partition(":")
        if operation == "path_list_prepend":
            current = env.get(name)
            env[name] = operand + (os.pathsep + current if current else "")
        else:
            env[name] = value


BASE64_LINE = re.compile(r"[A-Za-z0-9+/]+={0,2}")


def parse_images(output: str) -> dict[str, bytes]:
    """Reads the frames from a run's output. A runner that merges the
    harness's stderr into its stdout, such as hdc's shell, can put a log line
    inside a frame, so lines that aren't base64 are left out, and a frame that
    still doesn't decode is dropped."""
    images = {}
    lines = iter(output.splitlines())
    for line in lines:
        if not line.startswith(IMAGE_BEGIN):
            continue
        name = line.removeprefix(IMAGE_BEGIN).strip()
        encoded = []
        for part in lines:
            part = part.strip()
            if part == IMAGE_END:
                break
            if BASE64_LINE.fullmatch(part):
                encoded.append(part)
        else:
            log(f"left out the frame of {name}: the output ends inside it")
            break
        try:
            images[name] = base64.b64decode("".join(encoded), validate=True)
        except ValueError as error:
            log(f"left out the frame of {name}: {error}")
    return images


def load_average() -> list[float] | None:
    # Windows has no load average.
    if not hasattr(os, "getloadavg"):
        return None
    return [round(value, 2) for value in os.getloadavg()]


def run_harness(
    side: Side, fixtures: Path, arguments: list[str], images_dir: Path | None
) -> dict:
    """Runs one side's harness once, and returns its report."""
    assert side.descriptor is not None and side.install is not None
    descriptor = side.descriptor
    artifact = json.loads(
        (side.install / "share" / "maplibre-native-c" / "artifact.json").read_text()
    )
    platform_id = artifact["platform"]["id"]
    env = dict(os.environ)
    env.setdefault("MISE_MONOREPO_ROOT", str(ROOT))
    env["MLN_FFI_DEVICE_INSTALL"] = str(side.install)
    env["MLN_FFI_DEVICE_TIMEOUT"] = str(RUN_TIMEOUT_SECONDS)
    # The Android and OpenHarmony runners push this directory to the device and
    # point the variable at the copy; the other runners share this file system.
    env["MLN_FFI_TEST_FIXTURE_DIR"] = str(fixtures)
    apply_environment(env, descriptor["environment"])
    arguments = list(arguments)
    if not platform_id.startswith(("android-", "ohos-")):
        arguments.append(f"--mln_fixture_dir={fixtures}")
    runner = str(ROOT / "tools" / "devices" / "run")
    # The device boots first, so its boot is in no number.
    subprocess.run([runner, "--boot"], env=env, check=True)
    command = [runner, descriptor["executable"], *arguments]
    load_before = load_average()
    log(f"{side.label}: $ " + " ".join(command))
    completed = subprocess.run(
        command, check=False, env=env, stdout=subprocess.PIPE, text=True
    )
    output = completed.stdout.replace("\r", "")
    if completed.returncode != 0 or JSON_BEGIN not in output:
        sys.stderr.write(output[-20000:])
        raise SystemExit(f"{side.label}: the harness exited {completed.returncode}")
    report = json.loads(output.split(JSON_BEGIN, 1)[1].split(JSON_END, 1)[0])
    if images_dir is not None:
        for name, png in parse_images(output).items():
            images_dir.mkdir(parents=True, exist_ok=True)
            (images_dir / (name.replace("/", "--") + ".png")).write_bytes(png)
    report["mln_run"] = {
        "side": side.label,
        "spec": side.spec,
        "commit": side.commit,
        "dirty": bool(
            side.checkout == ROOT
            and git(ROOT, "status", "--porcelain", "--untracked-files=no")
        ),
        "cmake_args": side.cmake_args,
        "install": str(side.install),
        "platform": platform_id,
        "load_average_before": load_before,
        "load_average_after": load_average(),
    }
    return report


# Statistics

# Counters that grow when things get better. Every other number is a cost.
HIGHER_IS_BETTER = {"items_per_second", "bytes_per_second", "frames"}
# The metrics that get verdicts. The others print with --verbose, as clues to
# where a change in these comes from.
LEAD = (
    "cpu_time",
    "real_time",
    "allocs",
    "peak_rss_mib",
    "heap_mib",
    "frame_p95_ms",
    "frame_p99_ms",
    "missed_deadline_pct",
    "longest_stall_ms",
    "stale_frame_pct",
    "partial_frame_pct",
    "complete_after_ms",
    "items_per_second",
)
# The fields of a run that aren't measurements, and cpu_time, which
# entry_metrics() reads on its own.
NOT_METRICS = {
    "name",
    "family_index",
    "per_family_instance_index",
    "run_name",
    "run_type",
    "repetitions",
    "repetition_index",
    "threads",
    "iterations",
    "time_unit",
    "cpu_time",
}


def case_name(run_name: str) -> str:
    """A case's name without the settings that every case shares."""
    for suffix in (
        "/iterations:",
        "/repeats:",
        "/min_time:",
        "/process_time",
        "/real_time",
    ):
        at = run_name.find(suffix)
        if at >= 0:
            run_name = run_name[:at]
    return run_name


def entry_metrics(report: dict, entry: dict) -> dict[str, float]:
    metrics = {}
    # Without a process CPU clock, Google Benchmark's CPU time is the benchmark
    # thread's alone.
    if report["context"].get("mln_process_cpu") == "true":
        metrics["cpu_time"] = float(entry["cpu_time"])
    for name, value in entry.items():
        if name not in NOT_METRICS and isinstance(value, (int, float)):
            metrics[name] = float(value)
    return metrics


def labels(reports: list[dict]) -> dict[str, str]:
    """The notes that cases attached to their results, such as a fallback to
    offscreen rendering."""
    found = {}
    for report in reports:
        for entry in report["benchmarks"]:
            if entry.get("label"):
                found[case_name(entry["run_name"])] = entry["label"]
    return found


def render_setup(label: str | None) -> str:
    """The parts of a case's label that say how it rendered, as opposed to
    notes such as a missing frame."""
    return "; ".join(
        part for part in (label or "").split("; ") if part.startswith("offscreen")
    )


def collect(reports: list[dict]) -> tuple[dict, dict, dict]:
    """Each case's samples by metric, its time unit, and why it has none. A
    sample is one process's median over its repetitions, because repetitions
    in one process share its heap, caches, and threads, so only separate
    processes are independent."""
    samples: dict[str, dict[str, list[float]]] = {}
    units: dict[str, str] = {}
    missing: dict[str, str] = {}
    for report in reports:
        runs: dict[str, dict[str, list[float]]] = {}
        for entry in report["benchmarks"]:
            if entry.get("run_type") != "iteration":
                continue
            case = case_name(entry["run_name"])
            if entry.get("error_occurred"):
                missing[case] = f"error: {entry.get('error_message', '')}"
                continue
            if entry.get("skipped"):
                missing[case] = f"skipped: {entry.get('skip_message', '')}"
                continue
            units[case] = entry.get("time_unit", "us")
            for name, value in entry_metrics(report, entry).items():
                runs.setdefault(case, {}).setdefault(name, []).append(value)
        for case, metrics in runs.items():
            for name, values in metrics.items():
                samples.setdefault(case, {}).setdefault(name, []).append(
                    statistics.median(values)
                )
    for case in samples:
        missing.pop(case, None)
    return samples, units, missing


def mann_whitney_p(a: list[float], b: list[float]) -> float:
    """The two-sided p-value of a Mann-Whitney U test: exact when the samples
    are small and untied, and otherwise the normal approximation with a tie
    correction."""
    n1, n2 = len(a), len(b)
    pooled = sorted([(value, 0) for value in a] + [(value, 1) for value in b])
    ranks = [0.0] * len(pooled)
    ties = []
    index = 0
    while index < len(pooled):
        end = index
        while end + 1 < len(pooled) and pooled[end + 1][0] == pooled[index][0]:
            end += 1
        for position in range(index, end + 1):
            ranks[position] = (index + end) / 2 + 1
        ties.append(end - index + 1)
        index = end + 1
    rank_a = sum(rank for rank, (_, side) in zip(ranks, pooled) if side == 0)
    u = rank_a - n1 * (n1 + 1) / 2
    u_min = min(u, n1 * n2 - u)
    if all(count == 1 for count in ties) and n1 * n2 <= 400:
        # counts[i][j][k] is the number of orderings of i and j values with
        # U = k.
        counts = [[[0] * (n1 * n2 + 1) for _ in range(n2 + 1)] for _ in range(n1 + 1)]
        for i in range(n1 + 1):
            for j in range(n2 + 1):
                if i == 0 or j == 0:
                    counts[i][j][0] = 1
                    continue
                for k in range(i * j + 1):
                    counts[i][j][k] = (counts[i - 1][j][k - j] if k >= j else 0) + (
                        counts[i][j - 1][k]
                    )
        total = math.comb(n1 + n2, n1)
        tail = sum(counts[n1][n2][k] for k in range(int(u_min) + 1)) / total
        return min(1.0, 2 * tail)
    size = n1 + n2
    tie_term = sum(count**3 - count for count in ties) / (size * (size - 1))
    variance = n1 * n2 / 12 * ((size + 1) - tie_term)
    if variance <= 0:
        return 1.0
    z = (abs(u - n1 * n2 / 2) - 0.5) / math.sqrt(variance)
    return min(1.0, math.erfc(max(z, 0.0) / math.sqrt(2)))


def holm(p_values: list[float], alpha: float) -> list[bool]:
    """Which of a family of tests reject, by Holm's step-down method, which
    holds the chance of any false rejection in the family under `alpha`."""
    order = sorted(range(len(p_values)), key=lambda index: p_values[index])
    rejected = [False] * len(p_values)
    for rank, index in enumerate(order):
        if not p_values[index] < alpha / (len(p_values) - rank):
            break
        rejected[index] = True
    return rejected


def sides_differ(base: list[dict], candidate: list[dict]) -> list[str]:
    """Reasons that no number of the two sides compares: they ran on
    different GPUs, or one on a software rasterizer."""
    reasons = []
    for key in ("mln_gpu_device", "mln_software_gpu", "mln_render_target"):
        values = [
            sorted({str(report["context"].get(key)) for report in reports})
            for reports in (base, candidate)
        ]
        if values[0] != values[1]:
            reasons.append(f"{key} differs: base {values[0]}, candidate {values[1]}")
    return reasons


def compare_reports(
    base: list[dict], candidate: list[dict], alpha: float, threshold: float
) -> dict:
    base_samples, units, base_missing = collect(base)
    candidate_samples, candidate_units, candidate_missing = collect(candidate)
    units.update(candidate_units)
    base_labels, candidate_labels = labels(base), labels(candidate)
    mismatch = sides_differ(base, candidate)
    rows = []
    for case in sorted(set(base_samples) & set(candidate_samples)):
        # A case that rendered differently on the two sides, such as offscreen
        # on one because it lacks the ring, measured different work.
        comparable = not mismatch and render_setup(
            base_labels.get(case)
        ) == render_setup(candidate_labels.get(case))
        case_rows: list[dict] = []
        for metric, base_values in base_samples[case].items():
            values = candidate_samples[case].get(metric)
            if not values:
                continue
            base_median = statistics.median(base_values)
            candidate_median = statistics.median(values)
            change = (
                (candidate_median - base_median) / abs(base_median)
                if base_median
                else (0.0 if candidate_median == 0 else math.inf)
            )
            case_rows.append(
                {
                    "case": case,
                    "metric": metric,
                    "unit": units[case],
                    "base_median": base_median,
                    "candidate_median": candidate_median,
                    "change": change,
                    "p_value": mann_whitney_p(base_values, values),
                    "verdict": "" if comparable else "not comparable",
                    "samples": [len(base_values), len(values)],
                }
            )
        # Only the leading metrics get verdicts, as one family per case, so
        # that the dozens of counters that a case reports can't pass by
        # chance. A verdict also needs a change past the threshold, because
        # with enough runs a change too small to matter is significant.
        tested = [row for row in case_rows if comparable and row["metric"] in LEAD]
        for row, rejected in zip(
            tested, holm([row["p_value"] for row in tested], alpha)
        ):
            if rejected and abs(row["change"]) >= threshold and row["change"]:
                higher = row["candidate_median"] > row["base_median"]
                better = higher == (row["metric"] in HIGHER_IS_BETTER)
                row["verdict"] = "better" if better else "worse"
        rows += case_rows
    missing = []
    for case in sorted(
        set(base_samples)
        | set(candidate_samples)
        | set(base_missing)
        | set(candidate_missing)
    ):
        if case in base_samples and case in candidate_samples:
            continue
        missing.append(
            {
                "case": case,
                "base": "measured"
                if case in base_samples
                else base_missing.get(case, "not run"),
                "candidate": "measured"
                if case in candidate_samples
                else candidate_missing.get(case, "not run"),
            }
        )
    notes = [f"no verdicts: {reason}" for reason in mismatch]
    for case in sorted(set(base_labels) | set(candidate_labels)):
        base_setup = render_setup(base_labels.get(case))
        candidate_setup = render_setup(candidate_labels.get(case))
        if base_setup != candidate_setup:
            notes.append(
                f"{case}: not comparable, the sides rendered differently: base "
                f"{base_setup or 'as usual'}; candidate "
                f"{candidate_setup or 'as usual'}. Pass --target offscreen to "
                "render offscreen on both"
            )
        elif base_labels.get(case) != candidate_labels.get(case):
            notes.append(
                f"{case}: base {base_labels.get(case) or 'as usual'}; "
                f"candidate {candidate_labels.get(case) or 'as usual'}"
            )
    return {"rows": rows, "missing": missing, "notes": notes}


def print_comparison(comparison: dict, verbose: bool) -> None:
    shown = [row for row in comparison["rows"] if verbose or row["metric"] in LEAD]
    width = max([len(row["case"]) for row in shown] + [4])
    metric_width = max([len(row["metric"]) for row in shown] + [6])
    print(
        f"{'case':<{width}}  {'metric':<{metric_width}}  {'base':>11}  "
        f"{'candidate':>11}  {'change':>8}  {'p':>6}"
    )
    for row in shown:
        change = row["change"]
        change_text = f"{change * 100:+7.1f}%" if math.isfinite(change) else "     n/a"
        metric = row["metric"]
        if metric in ("cpu_time", "real_time"):
            metric = f"{metric} ({row['unit']})"
        print(
            f"{row['case']:<{width}}  {metric:<{metric_width}}  "
            f"{row['base_median']:>11.4g}  {row['candidate_median']:>11.4g}  "
            f"{change_text}  {row['p_value']:>6.3f}  {row['verdict']}"
        )
    for note in comparison["notes"]:
        print(note)
    for entry in comparison["missing"]:
        print(
            f"{entry['case']}: missing; base {entry['base']}; "
            f"candidate {entry['candidate']}"
        )
    print(
        "\nEach side's sample is one run's median over its repetitions, and the "
        "table shows the median of those. A verdict needs a change past the "
        "threshold that a Mann-Whitney U test finds significant, Holm-corrected "
        "across the case's leading metrics. Pass --verbose for every counter."
    )


def print_summary(report: dict) -> None:
    samples, units, missing = collect([report])
    context = report["context"]
    print(
        f"GPU: {context.get('mln_gpu_device')} ({context.get('mln_gpu_type')}, "
        f"{context.get('mln_gpu_driver')}); load "
        f"{report['mln_run']['load_average_before']}"
    )
    for case, metrics in samples.items():
        parts = [
            f"{name}={statistics.median(values):.4g}"
            for name, values in metrics.items()
            if name in LEAD
        ]
        print(f"{case} ({units[case]}): " + " ".join(parts))
    for case, reason in missing.items():
        print(f"{case}: {reason}")
    for case, label in labels([report]).items():
        print(f"{case}: {label}")


# Images


def decode_png(data: bytes) -> tuple[int, int, bytearray]:
    """Decodes the harness's PNGs: 8-bit RGBA, each row filtered with Up."""
    at = 8
    idat = b""
    width = height = 0
    while at < len(data):
        (length,) = struct.unpack(">I", data[at : at + 4])
        kind = data[at + 4 : at + 8]
        body = data[at + 8 : at + 8 + length]
        if kind == b"IHDR":
            width, height = struct.unpack(">II", body[:8])
        elif kind == b"IDAT":
            idat += body
        at += 12 + length
    raw = zlib.decompress(idat)
    stride = width * 4
    pixels = bytearray(stride * height)
    previous = bytes(stride)
    low = int.from_bytes(b"\x7f" * stride, "big")
    high = int.from_bytes(b"\x80" * stride, "big")
    for row in range(height):
        start = row * (stride + 1)
        if raw[start] != 2:
            raise ValueError(f"PNG filter {raw[start]}")
        # Adds the row above byte by byte, without carries between bytes.
        a = int.from_bytes(raw[start + 1 : start + 1 + stride], "big")
        b = int.from_bytes(previous, "big")
        line = (((a & low) + (b & low)) ^ ((a ^ b) & high)).to_bytes(stride, "big")
        pixels[row * stride : (row + 1) * stride] = line
        previous = line
    return width, height, pixels


def encode_png(width: int, height: int, pixels: bytes) -> bytes:
    stride = width * 4
    raw = b"".join(
        b"\x00" + pixels[row * stride : (row + 1) * stride] for row in range(height)
    )

    def chunk(kind: bytes, body: bytes) -> bytes:
        return (
            struct.pack(">I", len(body))
            + kind
            + body
            + struct.pack(">I", zlib.crc32(kind + body))
        )

    header = struct.pack(">IIBBBBB", width, height, 8, 6, 0, 0, 0)
    return (
        b"\x89PNG\r\n\x1a\n"
        + chunk(b"IHDR", header)
        + chunk(b"IDAT", zlib.compress(raw, 6))
        + chunk(b"IEND", b"")
    )


# A channel that differs by more than this counts as a different pixel, which
# leaves out antialiasing that differs between GPUs and drivers.
PIXEL_THRESHOLD = 32


def compare_images(base_dir: Path, candidate_dir: Path, out_dir: Path) -> list[dict]:
    """Summarizes how each frame differs between the sides, and writes an
    image of each difference: changed pixels in red over a faded copy of the
    base frame."""
    results = []
    names = sorted(
        {path.name for path in base_dir.glob("*.png")}
        | {path.name for path in candidate_dir.glob("*.png")}
    )
    for name in names:
        base_path = base_dir / name
        candidate_path = candidate_dir / name
        case = name.removesuffix(".png").replace("--", "/")
        if not base_path.exists() or not candidate_path.exists():
            side = "base" if not base_path.exists() else "candidate"
            results.append({"case": case, "note": f"no frame from the {side}"})
            continue
        bw, bh, base = decode_png(base_path.read_bytes())
        cw, ch, candidate = decode_png(candidate_path.read_bytes())
        if (bw, bh) != (cw, ch):
            results.append(
                {"case": case, "note": f"sizes differ: {bw}x{bh} and {cw}x{ch}"}
            )
            continue
        changed = 0
        total_difference = 0
        diff = bytearray(len(base))
        for at in range(0, len(base), 4):
            delta = max(abs(base[at + c] - candidate[at + c]) for c in range(4))
            total_difference += sum(
                abs(base[at + c] - candidate[at + c]) for c in range(3)
            )
            if delta > PIXEL_THRESHOLD:
                changed += 1
                diff[at : at + 4] = b"\xff\x00\x00\xff"
            else:
                gray = 255 - (255 - sum(base[at : at + 3]) // 3) // 4
                diff[at : at + 4] = bytes((gray, gray, gray, 255))
        pixels = bw * bh
        out_dir.mkdir(parents=True, exist_ok=True)
        (out_dir / name).write_bytes(encode_png(bw, bh, bytes(diff)))
        results.append(
            {
                "case": case,
                "changed_pct": 100.0 * changed / pixels,
                "mean_abs_difference": total_difference / (pixels * 3),
                "diff": str(out_dir / name),
            }
        )
    return results


def print_image_comparison(results: list[dict]) -> None:
    if not results:
        return
    print(
        "\nLast frames (a pixel changes when a channel differs by more than "
        f"{PIXEL_THRESHOLD}):"
    )
    for result in results:
        if "note" in result:
            print(f"  {result['case']}: {result['note']}")
        else:
            print(
                f"  {result['case']}: {result['changed_pct']:.2f}% of pixels changed, "
                f"mean difference {result['mean_abs_difference']:.2f}"
            )


# Commands


def command_fixtures(_: argparse.Namespace) -> None:
    print(prepare_fixtures())


def command_run(args: argparse.Namespace) -> None:
    fixtures = prepare_fixtures()
    (side,) = resolve_sides([("run", args.side, args.cmake_arg)])
    build_side(side, args.preset, fixtures)
    out = (
        Path(args.out)
        if args.out
        else BENCH_DIR / "runs" / f"{args.preset}-{timestamp()}"
    )
    report = run_harness(
        side,
        fixtures,
        harness_arguments(args, images=not args.no_images),
        out / "images",
    )
    write_json(out / "report.json", report)
    print_summary(report)
    print(f"\nWrote {out}")


def command_compare(args: argparse.Namespace) -> None:
    fixtures = prepare_fixtures()
    sides = resolve_sides(
        [
            ("base", args.base, args.base_cmake_arg),
            ("candidate", args.candidate, args.candidate_cmake_arg),
        ]
    )
    # Sides that would build in one directory build once.
    built: dict[tuple, Side] = {}
    for side in sides:
        key = (side.spec if side.checkout is None else side.checkout, *side.cmake_args)
        if key in built:
            same = built[key]
            side.build_dir, side.install = same.build_dir, same.install
            side.descriptor = same.descriptor
            continue
        build_side(side, args.preset, fixtures)
        built[key] = side
    out = BENCH_DIR / "compare" / f"{args.preset}-{timestamp()}"
    reports: dict[str, list[dict]] = {"base": [], "candidate": []}
    # Interleaved, so drift in the machine's load reaches both sides alike, in
    # blocks of A B B A, so that each side runs first equally often. The first
    # round keeps the frames.
    for round_index in range(args.rounds):
        for side in sides if round_index % 4 in (0, 3) else sides[::-1]:
            log(f"round {round_index + 1} of {args.rounds}: {side.label}")
            first = round_index == 0
            report = run_harness(
                side,
                fixtures,
                harness_arguments(args, images=first),
                out / side.label / "images" if first else None,
            )
            write_json(out / side.label / f"round-{round_index + 1}.json", report)
            reports[side.label].append(report)
    comparison = compare_reports(
        reports["base"], reports["candidate"], args.alpha, args.threshold
    )
    comparison["images"] = compare_images(
        out / "base" / "images", out / "candidate" / "images", out / "diff"
    )
    comparison["sides"] = {side.label: side.describe() for side in sides}
    comparison["gpu"] = {
        side: reports[side][0]["context"].get("mln_gpu_device") for side in reports
    }
    comparison["load_average"] = {
        side: [report["mln_run"]["load_average_before"] for report in reports[side]]
        for side in reports
    }
    write_json(out / "comparison.json", comparison)
    print(f"base:      {sides[0].describe()}")
    print(f"candidate: {sides[1].describe()}")
    print(f"GPU:       {comparison['gpu']}")
    print(f"load:      {comparison['load_average']}\n")
    print_comparison(comparison, args.verbose)
    print_image_comparison(comparison["images"])
    print(f"\nWrote {out}")


SIDE = (
    "a Git ref, worktree, build:DIR for a build tree, or install:DIR for a host install"
)
CMAKE_ARG = "a CMake argument for that side, such as -DNAME=VALUE; repeatable"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    commands = parser.add_subparsers(dest="command", required=True)
    commands.add_parser("fixtures", help="download the pinned fixtures")

    def add_options(subparser: argparse.ArgumentParser) -> None:
        subparser.add_argument(
            "preset",
            nargs="?",
            # The mise tasks name the host's preset.
            default=os.environ.get("MLN_BENCH_HOST_PRESET"),
            help="native CMake preset",
        )
        subparser.add_argument("--filter", default="", help="case name regex")
        subparser.add_argument("--repetitions", type=int, default=3)
        subparser.add_argument(
            "--iteration-scale",
            type=float,
            default=1.0,
            help="multiplies every case's iteration count",
        )
        subparser.add_argument(
            "--hz", type=int, default=60, help="the display refresh rate"
        )
        subparser.add_argument(
            "--target",
            choices=("ring", "offscreen"),
            default="ring",
            help="render into a ring of the host's textures, or offscreen",
        )

    run_parser = commands.add_parser("run", help="build and run one side")
    add_options(run_parser)
    run_parser.add_argument("--side", default="worktree", help=SIDE)
    run_parser.add_argument("--cmake-arg", action="append", default=[], help=CMAKE_ARG)
    run_parser.add_argument("--no-images", action="store_true")
    run_parser.add_argument("--out", help="the run's directory")

    compare_parser = commands.add_parser(
        "compare", help="compare two sides, interleaved"
    )
    compare_parser.add_argument("base", help=SIDE)
    add_options(compare_parser)
    compare_parser.add_argument("--candidate", default="worktree", help=SIDE)
    compare_parser.add_argument(
        "--base-cmake-arg", action="append", default=[], help=CMAKE_ARG
    )
    compare_parser.add_argument(
        "--candidate-cmake-arg", action="append", default=[], help=CMAKE_ARG
    )
    compare_parser.add_argument(
        "--rounds", type=int, default=6, help="an even number of rounds"
    )
    compare_parser.add_argument("--alpha", type=float, default=0.05)
    compare_parser.add_argument(
        "--threshold",
        type=float,
        default=0.05,
        help="the smallest relative change that gets a verdict",
    )
    compare_parser.add_argument(
        "--verbose", action="store_true", help="also print numbers without verdicts"
    )

    args = parser.parse_args()
    if args.command != "fixtures" and not args.preset:
        parser.error("pass a preset")
    if args.command == "compare" and (args.rounds < 2 or args.rounds % 2):
        parser.error("--rounds must be even, so each side runs first equally often")
    {"fixtures": command_fixtures, "run": command_run, "compare": command_compare}[
        args.command
    ](args)


if __name__ == "__main__":
    main()
