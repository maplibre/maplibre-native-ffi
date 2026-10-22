#!/usr/bin/env python3

from __future__ import annotations

import argparse
import datetime
import json
import pathlib
import re
import subprocess
from dataclasses import dataclass


@dataclass(frozen=True, order=True)
class ReleaseVersion:
    epoch: int
    month: int
    revision: int

    def __str__(self) -> str:
        return f"{self.epoch}.{self.month:06d}.{self.revision}"


VERSION_PATTERN = re.compile(
    r"^(?P<epoch>0|[1-9][0-9]*)\."
    r"(?P<year>[0-9]{4})(?P<month>0[1-9]|1[0-2])\."
    r"(?P<revision>0|[1-9][0-9]*)$"
)
TAG_PREFIX = "v"
# Before one tag released everything, the core and each binding had tags of
# their own. Those tags stay in the version history, so a release follows
# every version that any of them published.
LEGACY_TAG = re.compile(r"^(?:core|bindings/[a-z][a-z0-9-]*)/v")


def parse_version(value: str) -> ReleaseVersion:
    match = VERSION_PATTERN.fullmatch(value)
    if match is None:
        raise SystemExit(
            f"invalid release version {value!r}; expected API_EPOCH.YYYYMM.REVISION"
        )
    return ReleaseVersion(
        int(match.group("epoch")),
        int(match.group("year") + match.group("month")),
        int(match.group("revision")),
    )


def parse_tag(tag: str) -> tuple[ReleaseVersion, str]:
    if not tag.startswith(TAG_PREFIX):
        raise SystemExit(
            f"invalid release tag {tag!r}; expected vAPI_EPOCH.YYYYMM.REVISION"
        )
    version = tag.removeprefix(TAG_PREFIX)
    return parse_version(version), version


def require_current_month(
    version: str, *, now: datetime.datetime | None = None
) -> None:
    fields = parse_version(version)
    current = now or datetime.datetime.now(datetime.UTC)
    current_month = int(current.strftime("%Y%m"))
    if fields.month != current_month:
        raise SystemExit(
            f"release {version} names month {fields.month}; "
            f"the current UTC month is {current_month}"
        )


def release_tags(
    *, repository: pathlib.Path | None = None
) -> dict[str, ReleaseVersion]:
    """Every release tag in the checkout, legacy ones included, by version."""
    result = subprocess.run(
        ["git", "tag", "--list", "v*", "core/v*", "bindings/*/v*"],
        cwd=repository,
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    tags = {}
    for tag in filter(None, result.stdout.splitlines()):
        legacy = LEGACY_TAG.match(tag)
        version = tag[legacy.end() :] if legacy else tag.removeprefix(TAG_PREFIX)
        tags[tag] = parse_version(version)
    return tags


def validate_tag(
    tag: str,
    *,
    repository: pathlib.Path | None = None,
    now: datetime.datetime | None = None,
) -> tuple[str, str]:
    current, version = parse_tag(tag)
    require_current_month(version, now=now)
    parsed = release_tags(repository=repository)
    if tag not in parsed:
        raise SystemExit(f"release tag {tag} is not present in the checkout")
    previous = {other: fields for other, fields in parsed.items() if other != tag}
    if previous and current <= max(previous.values()):
        latest = max(previous, key=lambda other: (previous[other], other))
        raise SystemExit(f"{tag} is not newer than existing release tag {latest}")

    # A legacy core tag and a legacy binding tag may share a version.
    revisions = sorted(
        {
            fields.revision
            for fields in parsed.values()
            if (fields.epoch, fields.month) == (current.epoch, current.month)
        }
    )
    expected_revisions = list(range(current.revision + 1))
    if revisions != expected_revisions:
        raise SystemExit(
            f"release revisions for {current.epoch}.{current.month} are "
            f"{revisions}; expected {expected_revisions}"
        )

    previous_tag = (
        max(previous, key=lambda other: (previous[other], other)) if previous else ""
    )
    return version, previous_tag


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("tag")

    args = parser.parse_args()
    version, previous = validate_tag(args.tag)
    print(json.dumps({"previousTag": previous, "version": version}))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
