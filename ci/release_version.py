#!/usr/bin/env python3

from __future__ import annotations

import argparse
import datetime
import json
import pathlib
import re
import subprocess
from dataclasses import dataclass


@dataclass(frozen=True)
class ReleaseFamily:
    slug: str
    name: str
    prefix: str


@dataclass(frozen=True, order=True)
class ReleaseVersion:
    epoch: int
    month: int
    revision: int

    def __str__(self) -> str:
        return f"{self.epoch}.{self.month:06d}.{self.revision}"


BINDING_PATTERN = re.compile(r"^[a-z][a-z0-9-]*$")
VERSION_PATTERN = re.compile(
    r"^(?P<epoch>0|[1-9][0-9]*)\."
    r"(?P<year>[0-9]{4})(?P<month>0[1-9]|1[0-2])\."
    r"(?P<revision>0|[1-9][0-9]*)$"
)


def family(name: str) -> ReleaseFamily:
    if name == "core":
        return ReleaseFamily("core", "Core", "core")
    if not BINDING_PATTERN.fullmatch(name):
        raise SystemExit(
            f"invalid release family {name!r}; expected core or a binding name"
        )
    return ReleaseFamily(name, f"{name} binding", f"bindings/{name}")


def family_from_tag(tag: str) -> ReleaseFamily:
    if tag.startswith("core/v"):
        return family("core")
    match = re.match(r"^bindings/([^/]+)/v", tag)
    if match is not None:
        return family(match.group(1))
    raise SystemExit(
        f"invalid release tag {tag!r}; expected "
        "core/vAPI_EPOCH.YYYYMM.REVISION or "
        "bindings/<name>/vAPI_EPOCH.YYYYMM.REVISION"
    )


def parse_version(
    value: str, release_family: ReleaseFamily | None = None
) -> ReleaseVersion:
    match = VERSION_PATTERN.fullmatch(value)
    if match is None:
        subject = (
            "release" if release_family is None else f"{release_family.name} release"
        )
        raise SystemExit(
            f"invalid {subject} version {value!r}; expected API_EPOCH.YYYYMM.REVISION"
        )
    return ReleaseVersion(
        int(match.group("epoch")),
        int(match.group("year") + match.group("month")),
        int(match.group("revision")),
    )


def parse_tag(tag: str, release_family: ReleaseFamily) -> tuple[ReleaseVersion, str]:
    prefix = f"{release_family.prefix}/v"
    if not tag.startswith(prefix):
        raise SystemExit(
            f"invalid {release_family.name} release tag {tag!r}; expected "
            f"{release_family.prefix}/vAPI_EPOCH.YYYYMM.REVISION"
        )
    version = tag.removeprefix(prefix)
    return parse_version(version, release_family), version


def require_current_month(
    version: str,
    release_family: ReleaseFamily | None = None,
    *,
    now: datetime.datetime | None = None,
) -> None:
    fields = parse_version(version, release_family)
    current = now or datetime.datetime.now(datetime.UTC)
    current_month = int(current.strftime("%Y%m"))
    if fields.month != current_month:
        subject = (
            "release" if release_family is None else f"{release_family.name} release"
        )
        raise SystemExit(
            f"{subject} {version} names month {fields.month}; "
            f"the current UTC month is {current_month}"
        )


def release_tags(
    release_family: ReleaseFamily, *, repository: pathlib.Path | None = None
) -> dict[str, ReleaseVersion]:
    result = subprocess.run(
        ["git", "tag", "--list", f"{release_family.prefix}/v*"],
        cwd=repository,
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    tags = [line for line in result.stdout.splitlines() if line]
    return {tag: parse_tag(tag, release_family)[0] for tag in tags}


def validate_tag(
    tag: str,
    *,
    repository: pathlib.Path | None = None,
    now: datetime.datetime | None = None,
) -> tuple[ReleaseFamily, str, str]:
    release_family = family_from_tag(tag)
    current, version = parse_tag(tag, release_family)
    require_current_month(version, release_family, now=now)
    parsed = release_tags(release_family, repository=repository)
    if tag not in parsed:
        raise SystemExit(
            f"{release_family.name} release tag {tag} is not present in the checkout"
        )
    if current != max(parsed.values()):
        latest = max(parsed, key=parsed.__getitem__)
        raise SystemExit(
            f"{tag} is not newer than existing {release_family.name} tag {latest}"
        )

    revisions = sorted(
        fields.revision
        for fields in parsed.values()
        if (fields.epoch, fields.month) == (current.epoch, current.month)
    )
    expected_revisions = list(range(current.revision + 1))
    if revisions != expected_revisions:
        raise SystemExit(
            f"{release_family.name} release revisions for "
            f"{current.epoch}.{current.month} are {revisions}; "
            f"expected {expected_revisions}"
        )

    previous = [candidate for candidate in parsed if candidate != tag]
    previous_tag = max(previous, key=parsed.__getitem__) if previous else ""
    return release_family, version, previous_tag


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("tag")

    args = parser.parse_args()
    release_family, version, previous = validate_tag(args.tag)
    print(
        json.dumps(
            {
                "family": release_family.slug,
                "previousTag": previous,
                "version": version,
            }
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
