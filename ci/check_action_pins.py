import pathlib
import sys

from ci.action_pins import CATALOG, check_pins

ROOT = pathlib.Path.cwd()


def main() -> int:
    problems = check_pins(ROOT)
    if not problems:
        return 0
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    print(
        f"\nUpdate {CATALOG.as_posix()} and the references above so every action "
        "resolves to one commit, then run `mise run ci:generate-workflow`.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
