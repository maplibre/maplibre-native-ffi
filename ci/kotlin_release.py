#!/usr/bin/env python3

import argparse
import base64
import datetime
import hashlib
import http.client
import json
import os
import pathlib
import re
import shutil
import subprocess
import tempfile
import time
import urllib.parse
import zipfile

GROUP_PATH = pathlib.PurePosixPath("org/maplibre/nativeffi")
MODULES = set(
    json.loads(
        pathlib.Path(__file__)
        .with_name("kotlin_maven_modules.json")
        .read_text(encoding="utf-8")
    )
)
ROOT_MODULES = {
    "maplibre-native-ffi",
    "maplibre-native-ffi-runtime-metal",
    "maplibre-native-ffi-runtime-opengl",
    "maplibre-native-ffi-runtime-vulkan",
}

TAG_PATTERN = re.compile(
    r"^bindings/kotlin/v(?P<epoch>0|[1-9][0-9]*)\."
    r"(?P<year>[0-9]{4})(?P<month>0[1-9]|1[0-2])\."
    r"(?P<revision>0|[1-9][0-9]*)$"
)
CHECKSUM_SUFFIXES = (".md5", ".sha1", ".sha256", ".sha512")
MAX_BUNDLE_BYTES = 1_000_000_000
SNAPSHOT_USER_AGENT = (
    "maplibre-native-ffi-publisher/1 (+https://github.com/maplibre/maplibre-native-ffi)"
)


def parse_tag(tag: str) -> tuple[tuple[int, int, int], str]:
    match = TAG_PATTERN.fullmatch(tag)
    if match is None:
        raise SystemExit(
            f"invalid Kotlin release tag {tag!r}; expected "
            "bindings/kotlin/vAPI_EPOCH.YYYYMM.REVISION"
        )
    version = tag.removeprefix("bindings/kotlin/v")
    fields = (
        int(match.group("epoch")),
        int(match.group("year") + match.group("month")),
        int(match.group("revision")),
    )
    return fields, version


def require_current_month(version: str) -> None:
    fields, parsed_version = parse_tag(f"bindings/kotlin/v{version}")
    current_month = int(datetime.datetime.now(datetime.UTC).strftime("%Y%m"))
    if fields[1] != current_month:
        raise SystemExit(
            f"Kotlin release {parsed_version} names month {fields[1]}; "
            f"the current UTC month is {current_month}"
        )


def validate_tag(tag: str) -> str:
    current, version = parse_tag(tag)
    require_current_month(version)
    result = subprocess.run(
        ["git", "tag", "--list", "bindings/kotlin/v*"],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    tags = [line for line in result.stdout.splitlines() if line]
    parsed = {candidate: parse_tag(candidate)[0] for candidate in tags}
    if tag not in parsed:
        raise SystemExit(f"Kotlin release tag {tag} is not present in the checkout")
    if current != max(parsed.values()):
        latest = max(parsed, key=lambda candidate: parsed[candidate])
        raise SystemExit(f"{tag} is not newer than existing Kotlin tag {latest}")

    epoch, month, revision = current
    revisions = sorted(
        fields[2] for fields in parsed.values() if fields[:2] == (epoch, month)
    )
    expected_revisions = list(range(revision + 1))
    if revisions != expected_revisions:
        raise SystemExit(
            f"Kotlin release revisions for {epoch}.{month} are {revisions}; "
            f"expected {expected_revisions}"
        )
    return version


def repository_files(root: pathlib.Path) -> list[pathlib.Path]:
    return sorted(path for path in root.rglob("*") if path.is_file())


def merge_repositories(output: pathlib.Path, inputs: list[pathlib.Path]) -> None:
    if output.exists():
        raise SystemExit(f"merge output already exists: {output}")
    if len(inputs) < 2:
        raise SystemExit("merge requires at least two Maven repositories")
    output.mkdir(parents=True)
    copied = 0
    for source in inputs:
        group_root = source / GROUP_PATH
        if not group_root.is_dir():
            raise SystemExit(f"Maven repository input is missing {group_root}")
        for source_file in repository_files(source):
            if source_file.is_symlink():
                raise SystemExit(
                    f"Maven repository contains a symbolic link: {source_file}"
                )
            relative = source_file.relative_to(source)
            destination = output / relative
            if destination.exists():
                raise SystemExit(
                    f"Maven repository inputs both contain {relative.as_posix()}"
                )
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source_file, destination)
            copied += 1
    if copied == 0:
        raise SystemExit("Maven repository inputs contain no files")


def secret_key_fingerprint(gnupg_home: pathlib.Path) -> str:
    result = subprocess.run(
        [
            "gpg",
            "--batch",
            "--homedir",
            os.fspath(gnupg_home),
            "--with-colons",
            "--list-secret-keys",
        ],
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    fingerprints: list[str] = []
    expect_primary_fingerprint = False
    for line in result.stdout.splitlines():
        fields = line.split(":")
        record_type = fields[0]
        if record_type == "sec":
            expect_primary_fingerprint = True
        elif record_type == "fpr" and expect_primary_fingerprint:
            fingerprints.append(fields[9])
            expect_primary_fingerprint = False
        elif record_type not in {"fpr", "grp", "uid", "uat"}:
            expect_primary_fingerprint = False
    if len(fingerprints) != 1:
        raise SystemExit(
            f"the release signing key contains {len(fingerprints)} primary secret keys; "
            "expected one"
        )
    return fingerprints[0]


def import_signing_key(gnupg_home: pathlib.Path, private_key: str) -> str:
    subprocess.run(
        ["gpg", "--batch", "--homedir", os.fspath(gnupg_home), "--import"],
        input=private_key.encode(),
        check=True,
    )
    return secret_key_fingerprint(gnupg_home)


def sign_file(
    path: pathlib.Path,
    gnupg_home: pathlib.Path,
    fingerprint: str,
    passphrase: str,
) -> None:
    signature = path.with_name(path.name + ".asc")
    subprocess.run(
        [
            "gpg",
            "--batch",
            "--yes",
            "--homedir",
            os.fspath(gnupg_home),
            "--pinentry-mode",
            "loopback",
            "--passphrase-fd",
            "0",
            "--local-user",
            fingerprint,
            "--armor",
            "--detach-sign",
            "--output",
            os.fspath(signature),
            os.fspath(path),
        ],
        input=(passphrase + "\n").encode(),
        check=True,
    )
    verification = subprocess.run(
        [
            "gpg",
            "--batch",
            "--homedir",
            os.fspath(gnupg_home),
            "--verify",
            os.fspath(signature),
            os.fspath(path),
        ],
        capture_output=True,
        check=False,
    )
    if verification.returncode != 0:
        message = verification.stderr.decode(errors="replace")
        raise SystemExit(f"signature verification failed for {path}: {message}")


def write_checksum(path: pathlib.Path, algorithm: str) -> None:
    digest = hashlib.new(algorithm)
    with path.open("rb") as stream:
        while chunk := stream.read(1024 * 1024):
            digest.update(chunk)
    path.with_name(path.name + f".{algorithm}").write_text(
        digest.hexdigest() + "\n", encoding="ascii"
    )


def copy_release_files(
    repository: pathlib.Path, bundle_root: pathlib.Path, version: str
) -> list[pathlib.Path]:
    if version.endswith("-SNAPSHOT"):
        raise SystemExit("a Maven Central release version cannot end in -SNAPSHOT")
    require_current_month(version)
    group_root = repository / GROUP_PATH
    actual_modules = {path.name for path in group_root.iterdir() if path.is_dir()}
    if actual_modules != MODULES:
        raise SystemExit(
            f"the Maven repository contains modules {sorted(actual_modules)}; "
            f"expected {sorted(MODULES)}"
        )

    copied: list[pathlib.Path] = []
    for module in sorted(MODULES):
        version_root = group_root / module / version
        if not version_root.is_dir():
            raise SystemExit(f"the Maven repository is missing {version_root}")
        for source in repository_files(version_root):
            if source.is_symlink():
                raise SystemExit(f"Maven repository contains a symbolic link: {source}")
            if source.name == "maven-metadata.xml" or source.name.endswith(
                (".asc", *CHECKSUM_SUFFIXES)
            ):
                continue
            relative = source.relative_to(repository)
            destination = bundle_root / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            copied.append(destination)
    if not copied:
        raise SystemExit("the Maven repository contains no release files")
    return copied


def write_deterministic_zip(root: pathlib.Path, output: pathlib.Path) -> None:
    with zipfile.ZipFile(
        output, mode="x", compression=zipfile.ZIP_DEFLATED, compresslevel=9
    ) as archive:
        for path in repository_files(root):
            relative = path.relative_to(root).as_posix()
            info = zipfile.ZipInfo(relative, date_time=(1980, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            info.external_attr = 0o100644 << 16
            with path.open("rb") as stream:
                archive.writestr(info, stream.read())
    size = output.stat().st_size
    if size >= MAX_BUNDLE_BYTES:
        output.unlink()
        raise SystemExit(
            f"the Maven Central bundle is {size} bytes; it must remain below "
            f"{MAX_BUNDLE_BYTES} bytes"
        )


def create_bundle(repository: pathlib.Path, output: pathlib.Path, version: str) -> None:
    if output.exists():
        raise SystemExit(f"bundle output already exists: {output}")
    private_key = os.environ.get("GPG_PRIVATE_KEY")
    passphrase = os.environ.get("GPG_PASSPHRASE")
    if private_key is None or passphrase is None:
        raise SystemExit("GPG_PRIVATE_KEY and GPG_PASSPHRASE are required")
    output.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as temporary_directory:
        temporary_root = pathlib.Path(temporary_directory)
        bundle_root = temporary_root / "repository"
        release_files = copy_release_files(repository, bundle_root, version)
        gnupg_home = temporary_root / "gnupg"
        gnupg_home.mkdir(mode=0o700)
        fingerprint = import_signing_key(gnupg_home, private_key)
        for path in release_files:
            sign_file(path, gnupg_home, fingerprint, passphrase)
            write_checksum(path, "md5")
            write_checksum(path, "sha1")
        write_deterministic_zip(bundle_root, output)


def sign_snapshot(repository: pathlib.Path, version: str) -> None:
    if not version.endswith("-SNAPSHOT"):
        raise SystemExit("snapshot signing requires a version ending in -SNAPSHOT")
    private_key = os.environ.get("GPG_PRIVATE_KEY")
    passphrase = os.environ.get("GPG_PASSPHRASE")
    if private_key is None or passphrase is None:
        raise SystemExit("GPG_PRIVATE_KEY and GPG_PASSPHRASE are required")

    release_files: list[pathlib.Path] = []
    for module in sorted(MODULES):
        version_root = repository / GROUP_PATH / module / version
        if not version_root.is_dir():
            raise SystemExit(f"the Maven repository is missing {version_root}")
        for path in repository_files(version_root):
            if path.name.startswith("maven-metadata.xml"):
                continue
            if path.name.endswith((".asc", *CHECKSUM_SUFFIXES)):
                path.unlink()
            else:
                release_files.append(path)

    with tempfile.TemporaryDirectory(dir=repository.parent) as temporary_directory:
        gnupg_home = pathlib.Path(temporary_directory) / "gnupg"
        gnupg_home.mkdir(mode=0o700)
        fingerprint = import_signing_key(gnupg_home, private_key)
        for path in release_files:
            sign_file(path, gnupg_home, fingerprint, passphrase)
            write_checksum(path, "md5")
            write_checksum(path, "sha1")


def snapshot_module_files(
    repository: pathlib.Path, module: str, version: str
) -> list[pathlib.Path]:
    module_root = repository / GROUP_PATH / module
    version_root = module_root / version
    if not version_root.is_dir():
        raise SystemExit(f"the Maven repository is missing {version_root}")

    version_files = repository_files(version_root)
    artifacts = [
        path for path in version_files if not path.name.startswith("maven-metadata.xml")
    ]
    version_metadata = [
        path for path in version_files if path.name.startswith("maven-metadata.xml")
    ]
    module_metadata = sorted(
        path
        for path in module_root.iterdir()
        if path.is_file() and path.name.startswith("maven-metadata.xml")
    )
    return artifacts + version_metadata + module_metadata


def snapshot_upload_order(
    repository: pathlib.Path, version: str
) -> list[tuple[str, list[pathlib.Path]]]:
    if not version.endswith("-SNAPSHOT"):
        raise SystemExit("snapshot publishing requires a version ending in -SNAPSHOT")
    group_root = repository / GROUP_PATH
    actual_modules = {path.name for path in group_root.iterdir() if path.is_dir()}
    if actual_modules != MODULES:
        raise SystemExit(
            f"the Maven repository contains modules {sorted(actual_modules)}; "
            f"expected {sorted(MODULES)}"
        )
    leaves = sorted(MODULES - ROOT_MODULES)
    roots = sorted(ROOT_MODULES)
    return [
        (module, snapshot_module_files(repository, module, version))
        for module in leaves + roots
    ]


def put_snapshot_file(
    connection: http.client.HTTPSConnection,
    repository: pathlib.Path,
    path: pathlib.Path,
    authorization: str,
) -> http.client.HTTPSConnection:
    relative = path.relative_to(repository).as_posix()
    request_path = "/repository/maven-snapshots/" + urllib.parse.quote(relative)
    for attempt in range(1, 4):
        try:
            connection.putrequest("PUT", request_path)
            connection.putheader("Authorization", authorization)
            connection.putheader("User-Agent", SNAPSHOT_USER_AGENT)
            connection.putheader("Content-Type", "application/octet-stream")
            connection.putheader("Content-Length", str(path.stat().st_size))
            connection.endheaders()
            with path.open("rb") as stream:
                while chunk := stream.read(1024 * 1024):
                    connection.send(chunk)
            response = connection.getresponse()
            response_body = response.read()
            if 200 <= response.status < 300:
                return connection
            if response.status < 500 and response.status != 429:
                message = response_body.decode(errors="replace")[:1000]
                raise SystemExit(
                    f"snapshot upload of {relative} returned HTTP "
                    f"{response.status}: {message}"
                )
        except (OSError, http.client.HTTPException) as error:
            if attempt == 3:
                raise SystemExit(
                    f"snapshot upload of {relative} failed: {error}"
                ) from error
        connection.close()
        if attempt < 3:
            time.sleep(attempt * 2)
            connection = http.client.HTTPSConnection(
                "central.sonatype.com", timeout=120
            )
    raise AssertionError("snapshot upload retry loop did not return")


def upload_snapshot(repository: pathlib.Path, version: str) -> None:
    username = os.environ.get("MAVEN_CENTRAL_USERNAME")
    password = os.environ.get("MAVEN_CENTRAL_PASSWORD")
    if username is None or password is None:
        raise SystemExit(
            "MAVEN_CENTRAL_USERNAME and MAVEN_CENTRAL_PASSWORD are required"
        )
    credentials = base64.b64encode(f"{username}:{password}".encode()).decode("ascii")
    authorization = f"Basic {credentials}"
    connection = http.client.HTTPSConnection("central.sonatype.com", timeout=120)
    try:
        for module, paths in snapshot_upload_order(repository, version):
            print(f"uploading snapshot module {module} ({len(paths)} files)")
            for path in paths:
                connection = put_snapshot_file(
                    connection, repository, path, authorization
                )
    finally:
        connection.close()


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="operation", required=True)

    validate_tag_parser = subparsers.add_parser("validate-tag")
    validate_tag_parser.add_argument("tag")

    check_month_parser = subparsers.add_parser("check-month")
    check_month_parser.add_argument("version")

    merge_parser = subparsers.add_parser("merge")
    merge_parser.add_argument("output", type=pathlib.Path)
    merge_parser.add_argument("inputs", type=pathlib.Path, nargs="+")

    bundle_parser = subparsers.add_parser("bundle")
    bundle_parser.add_argument("repository", type=pathlib.Path)
    bundle_parser.add_argument("output", type=pathlib.Path)
    bundle_parser.add_argument("version")

    sign_snapshot_parser = subparsers.add_parser("sign-snapshot")
    sign_snapshot_parser.add_argument("repository", type=pathlib.Path)
    sign_snapshot_parser.add_argument("version")

    upload_snapshot_parser = subparsers.add_parser("upload-snapshot")
    upload_snapshot_parser.add_argument("repository", type=pathlib.Path)
    upload_snapshot_parser.add_argument("version")

    args = parser.parse_args()
    if args.operation == "validate-tag":
        print(validate_tag(args.tag))
    elif args.operation == "check-month":
        require_current_month(args.version)
    elif args.operation == "merge":
        merge_repositories(args.output, args.inputs)
    elif args.operation == "bundle":
        create_bundle(args.repository, args.output, args.version)
    elif args.operation == "sign-snapshot":
        sign_snapshot(args.repository, args.version)
    elif args.operation == "upload-snapshot":
        upload_snapshot(args.repository, args.version)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
