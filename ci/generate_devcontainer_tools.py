import argparse
import difflib
import pathlib
import sys

from ci.devcontainer_tools import render

ROOT = pathlib.Path.cwd()
OUTPUT = ROOT / ".devcontainer" / "mise.toml"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    expected = render(ROOT)
    actual = OUTPUT.read_text()
    if args.check:
        if actual == expected:
            return 0
        sys.stderr.writelines(
            difflib.unified_diff(
                actual.splitlines(keepends=True),
                expected.splitlines(keepends=True),
                fromfile=str(OUTPUT),
                tofile=f"{OUTPUT} (generated)",
            )
        )
        return 1
    OUTPUT.write_text(expected)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
