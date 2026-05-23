#!/usr/bin/env python3
"""Print the local Cargo-built Java JNI bridge library path."""

from __future__ import annotations

import argparse
import os
import platform
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]


def library_filename() -> str:
    system = platform.system()
    if system == "Darwin":
        return "libmaplibre_native_jni.dylib"
    if system == "Windows":
        return "maplibre_native_jni.dll"
    return "libmaplibre_native_jni.so"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--profile", default=os.environ.get("PROFILE", "debug"))
    args = parser.parse_args()

    target_dir = Path(os.environ.get("CARGO_TARGET_DIR", ROOT / "target"))
    print(target_dir / args.profile / library_filename())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
