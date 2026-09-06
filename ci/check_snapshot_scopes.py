import pathlib
import sys

from ci.snapshots import CONFIG, check_scopes

ROOT = pathlib.Path.cwd()


def main() -> int:
    problems = check_scopes(ROOT)
    if not problems:
        return 0
    for problem in problems:
        print(f"error: {problem}", file=sys.stderr)
    print(
        f"\nClassify every tracked path in {CONFIG.as_posix()}: add it to `shared` "
        "or to a component's `paths`, prefixed with `!` when it feeds no published "
        "artifact, and drop rules that match nothing.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
