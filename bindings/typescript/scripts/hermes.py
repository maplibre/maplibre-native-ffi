"""Build and run the TypeScript suite in official Hermes, without an app framework."""

import argparse
import hashlib
import json
import os
import platform
import plistlib
import queue
import re
import shutil
import subprocess
import tarfile
import threading
import time
import tomllib
import uuid
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
BINDING = ROOT / "bindings/typescript"
FIXTURE = BINDING / "test/hermes"
ARTIFACTS = json.loads(Path(__file__).with_name("hermes-artifacts.json").read_text())
DEPS = ROOT / "build/hermes/deps" / ARTIFACTS["version"]
ABIS = {"arm": "armeabi-v7a", "arm64": "arm64-v8a", "x86": "x86", "x64": "x86_64"}


def artifact(name):
    spec = ARTIFACTS[name]
    destination = DEPS / name
    if destination.is_dir():
        return destination
    DEPS.mkdir(parents=True, exist_ok=True)
    archive = DEPS / f"{name}.download"
    subprocess.run(
        [
            "curl",
            "--fail",
            "--location",
            "--retry",
            "3",
            "--output",
            str(archive),
            spec["url"],
        ],
        check=True,
    )
    with archive.open("rb") as stream:
        checksum = hashlib.file_digest(stream, "sha256").hexdigest()
    if checksum != spec["sha256"]:
        raise RuntimeError(f"Hermes {name} checksum mismatch")
    temporary = DEPS / f"{name}.extract"
    shutil.rmtree(temporary, ignore_errors=True)
    temporary.mkdir()
    if name in {"android", "fbjni"}:
        with zipfile.ZipFile(archive) as bundle:
            bundle.extractall(temporary)
    else:
        with tarfile.open(archive) as bundle:
            bundle.extractall(temporary, filter="data")
    if name == "source":
        (source,) = temporary.iterdir()
        source.rename(destination)
        temporary.rmdir()
    else:
        temporary.rename(destination)
    archive.unlink()
    return destination


def apple_framework(preset, artifact_root):
    framework = (
        artifact_root / "destroot/Library/Frameworks/universal/hermesvm.xcframework"
    )
    info = plistlib.loads((framework / "Info.plist").read_bytes())
    target_platform = preset.split("-")[0].replace("visionos", "xros")
    variant = (
        "maccatalyst"
        if "-maccatalyst-" in preset
        else "simulator"
        if "-simulator-" in preset
        else None
    )
    (library,) = [
        entry
        for entry in info["AvailableLibraries"]
        if entry["SupportedPlatform"] == target_platform
        and entry.get("SupportedPlatformVariant") == variant
        and "arm64" in entry["SupportedArchitectures"]
    ]
    return framework / library["LibraryIdentifier"] / library["LibraryPath"]


def android_abi(preset):
    return ABIS[preset.split("-")[1]]


def android_tools():
    sdk = Path(os.environ["ANDROID_HOME"])
    config = tomllib.loads((ROOT / ".config/mise.toml").read_text())["vars"]
    return sdk, sdk / "build-tools" / config["android_build_tools_version"], config


def package_android(preset, work, hermes, fbjni):
    sdk, tools, config = android_tools()
    abi = android_abi(preset)
    android_jar = (
        sdk
        / "platforms"
        / f"android-{config['android_system_image_api']}"
        / "android.jar"
    )
    classes = work / "classes"
    classes.mkdir(exist_ok=True)
    subprocess.run(
        [
            "javac",
            "-source",
            "17",
            "-target",
            "17",
            "-classpath",
            str(android_jar),
            "-d",
            str(classes),
            str(FIXTURE / "android/MainActivity.java"),
        ],
        check=True,
    )
    dex = work / "dex"
    dex.mkdir(exist_ok=True)
    subprocess.run(
        [
            str(tools / "d8"),
            "--min-api",
            "24",
            "--lib",
            str(android_jar),
            "--output",
            str(dex),
            *(str(path) for path in classes.rglob("*.class")),
        ],
        check=True,
    )
    apk = work / "unsigned.apk"
    subprocess.run(
        [
            str(tools / "aapt2"),
            "link",
            "-I",
            str(android_jar),
            "--manifest",
            str(FIXTURE / "android/AndroidManifest.xml"),
            "--version-code",
            "1",
            "--version-name",
            "1",
            "-o",
            str(apk),
        ],
        check=True,
    )
    ndk = sdk / "ndk" / config["android_ndk_version"]
    (cxx,) = (ndk / "toolchains/llvm/prebuilt").glob("*/sysroot/usr/lib")
    triples = {
        "armeabi-v7a": "arm-linux-androideabi",
        "arm64-v8a": "aarch64-linux-android",
        "x86": "i686-linux-android",
        "x86_64": "x86_64-linux-android",
    }
    libraries = [
        work / "libhermes-test.so",
        work / "jsi/libjsi.so",
        ROOT / "build" / preset / "install/lib/libmaplibre-native-c.so",
        hermes / "jni" / abi / "libhermesvm.so",
        fbjni / "jni" / abi / "libfbjni.so",
        cxx / triples[abi] / "libc++_shared.so",
    ]
    with zipfile.ZipFile(apk, "a") as bundle:
        for path in dex.glob("*.dex"):
            bundle.write(path, path.name)
        for path in libraries:
            bundle.write(path, f"lib/{abi}/{path.name}")
    aligned = work / "aligned.apk"
    subprocess.run(
        [str(tools / "zipalign"), "-f", "-p", "4", str(apk), str(aligned)], check=True
    )
    keystore = DEPS / "test.keystore"
    if not keystore.exists():
        subprocess.run(
            [
                "keytool",
                "-genkeypair",
                "-keystore",
                str(keystore),
                "-storepass",
                "android",
                "-keypass",
                "android",
                "-alias",
                "test",
                "-dname",
                "CN=Hermes binding tests",
                "-keyalg",
                "RSA",
                "-validity",
                "3650",
            ],
            check=True,
        )
    subprocess.run(
        [
            str(tools / "apksigner"),
            "sign",
            "--ks",
            str(keystore),
            "--ks-pass",
            "pass:android",
            "--out",
            str(work / "hermes-test.apk"),
            str(aligned),
        ],
        check=True,
    )


def build(preset):
    work = ROOT / "build/hermes" / preset
    work.mkdir(parents=True, exist_ok=True)
    target_platform = "android" if preset.startswith("android-") else "apple"
    source = artifact("source") if target_platform == "android" else None
    hermes = artifact(target_platform)
    fbjni = artifact("fbjni") if target_platform == "android" else None
    packages = work / "npm"
    subprocess.run(
        ["node", str(BINDING / "scripts/pack.ts"), str(packages), preset],
        cwd=BINDING,
        check=True,
    )
    consumer = work / "consumer"
    package = consumer / "node_modules/@maplibre/native-ffi"
    shutil.rmtree(package, ignore_errors=True)
    package.mkdir(parents=True)
    with tarfile.open(packages / "maplibre-native-ffi-0.0.0.tgz") as bundle:
        bundle.extractall(work / "unpack", filter="data")
    shutil.copytree(work / "unpack/package", package, dirs_exist_ok=True)
    shutil.rmtree(work / "unpack")
    bundle_env = os.environ | {
        "MAPLIBRE_NATIVE_FFI_HERMES_BUNDLE": str(work / "bundle"),
        "MLN_HERMES_CONSUMER": str(consumer),
    }
    for smoke in (False, True):
        subprocess.run(
            ["pnpm", "exec", "vp", "pack"],
            cwd=FIXTURE,
            env=bundle_env | ({"MLN_HERMES_SMOKE": "1"} if smoke else {}),
            check=True,
        )
    compiler_package = BINDING / "node_modules/hermes-compiler"
    if (
        json.loads((compiler_package / "package.json").read_text())["version"]
        != ARTIFACTS["version"]
    ):
        raise RuntimeError("Hermes compiler and runtime versions must match")
    compiler_host = {
        "Linux": "linux64-bin",
        "Darwin": "osx-bin",
        "Windows": "win64-bin",
    }[platform.system()]
    compiler = (
        compiler_package
        / "hermesc"
        / compiler_host
        / ("hermesc.exe" if os.name == "nt" else "hermesc")
    )
    for entry in ("suite", "smoke"):
        # Optimized bytecode, as in a release host, avoids unoptimized suspended
        # generators retaining dead locals across the suite's forced collections.
        subprocess.run(
            [
                str(compiler),
                "-O",
                "-emit-binary",
                "-out",
                str(work / f"bundle/{entry}.hbc"),
                str(work / f"bundle/{entry}.iife.js"),
            ],
            check=True,
        )
    command = [
        "cmake",
        "--preset",
        preset,
        "-B",
        str(work),
        f"-DMLN_FFI_NATIVE_PREFIX={ROOT / 'build' / preset / 'install'}",
        f"-DMLN_HERMES_ARTIFACT={hermes}",
        f"-DMLN_TS_NATIVE={package / 'native'}",
        f"-DMLN_HERMES_TEST_suite={work / 'bundle/suite.hbc'}",
        f"-DMLN_HERMES_TEST_smoke={work / 'bundle/smoke.hbc'}",
    ]
    if fbjni:
        command.extend(
            [
                f"-DMLN_HERMES_SOURCE={source}",
                f"-DMLN_HERMES_FBJNI={fbjni}",
                "-DANDROID_STL=c++_shared",
            ]
        )
    else:
        command.append(
            f"-DMLN_HERMES_APPLE_FRAMEWORK={apple_framework(preset, hermes)}"
        )
        if not preset.startswith("visionos-"):
            # The released Hermes iOS, Catalyst, and tvOS slices require 15.1.
            command.append("-DCMAKE_OSX_DEPLOYMENT_TARGET=15.1")
    subprocess.run(command, cwd=FIXTURE, check=True)
    subprocess.run(
        ["cmake", "--build", str(work), "--target", "hermes-test"], check=True
    )
    if target_platform == "android":
        package_android(preset, work, hermes, fbjni)


def run_android(preset, work):
    abi = android_abi(preset)
    serial = subprocess.check_output(
        [
            str(ROOT / "tools/devices/android-serial"),
            abi,
            "--api",
            "26"
            if preset == "android-x64-egl"
            else os.environ["MLN_FFI_ANDROID_EMULATOR_API"],
        ],
        text=True,
    ).strip()
    device = [
        str(Path(os.environ["ANDROID_HOME"]) / "platform-tools/adb"),
        "-s",
        serial,
    ]
    package = "org.maplibre.nativeffi.hermestest"
    subprocess.run(
        [*device, "install", "-r", str(work / "hermes-test.apk")], check=True
    )
    subprocess.run([*device, "shell", "am", "force-stop", package], check=True)
    process = subprocess.Popen(
        [*device, "logcat", "-T", "1", "-s", "HermesTest:I", "AndroidRuntime:E"],
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )
    lines = queue.Queue()

    def read():
        for line in process.stdout:
            lines.put(line)
        lines.put(None)

    threading.Thread(target=read, daemon=True).start()
    token = uuid.uuid4().hex
    deadline = time.monotonic() + 240
    try:
        subprocess.run(
            [
                *device,
                "shell",
                "am",
                "start",
                "-n",
                f"{package}/.MainActivity",
                "--es",
                "runId",
                token,
            ],
            check=True,
        )
        with (work / "runtime.log").open("w") as output:
            while True:
                try:
                    line = lines.get(timeout=max(0, deadline - time.monotonic()))
                except queue.Empty as error:
                    raise RuntimeError("Hermes Android suite timed out") from error
                if line is None:
                    raise RuntimeError(
                        "Android log stream ended before the suite passed"
                    )
                output.write(line)
                output.flush()
                print(line, end="", flush=True)
                match = re.search(rf"HermesTest exit {token} (\d+)", line)
                if match:
                    if int(match[1]):
                        raise RuntimeError("Hermes Android suite failed")
                    break
                if "FATAL EXCEPTION" in line:
                    raise RuntimeError("Hermes Android host crashed")
    finally:
        process.terminate()
        process.wait(timeout=10)
        subprocess.run([*device, "shell", "am", "force-stop", package], check=True)


def test(preset):
    work = ROOT / "build/hermes" / preset
    if preset.startswith("android-"):
        run_android(preset, work)
    elif "-simulator-" in preset:
        subprocess.run(
            [
                "bash",
                str(ROOT / "scripts/run-ios-simulator-test.sh"),
                str(work / "hermes-test"),
                "240",
            ],
            env=os.environ | {"MLN_FFI_SIMULATOR_RUNTIME": preset.split("-")[0]},
            check=True,
        )
    elif preset.startswith("ios-maccatalyst-"):
        subprocess.run([str(work / "hermes-test")], check=True, timeout=240)
    else:
        raise ValueError(f"{preset} is a build-only device target")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=["build", "test"])
    parser.add_argument("preset")
    args = parser.parse_args()
    (build if args.action == "build" else test)(args.preset)
