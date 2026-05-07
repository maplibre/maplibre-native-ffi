#!/usr/bin/env python3
# [MISE] description="Generate GitHub Actions matrix JSON"
# [USAGE] arg "<matrix>" help="Matrix to generate" {
# [USAGE]   choices "native" "examples"
# [USAGE] }
# [USAGE] flag "--schema <schema>" help="Path to the GitHub matrix config"
# [USAGE] flag "--pretty" help="Print indented JSON for local inspection"

import json
import os
import sys
import tomllib
from pathlib import Path

type TomlValue = str | int | float | bool | list[TomlValue] | dict[str, TomlValue]
type TomlTable = dict[str, TomlValue]
type MatrixEntry = dict[str, str]
type Matrix = dict[str, list[MatrixEntry]]


def load_toml(path: Path) -> TomlTable:
    """Read a TOML file."""
    with path.open("rb") as toml_file:
        return tomllib.load(toml_file)


def as_table(value: TomlValue | None) -> TomlTable:
    """Return a TOML value as a table."""
    if isinstance(value, dict):
        return value
    return {}


def as_table_map(value: TomlValue | None) -> dict[str, TomlTable]:
    """Return a TOML value as a table of tables."""
    if not isinstance(value, dict):
        return {}

    return {key: nested for key, nested in value.items() if isinstance(nested, dict)}


def as_exclusion_list(value: TomlValue | None) -> list[TomlTable]:
    """Return a TOML value as a list of exclusion tables."""
    if not isinstance(value, list):
        return []

    return [item for item in value if isinstance(item, dict)]


def string_value(table: TomlTable, key: str) -> str:
    """Return a required TOML table string."""
    value = table[key]
    if isinstance(value, str):
        return value

    message = f"{key} must be a string"
    raise TypeError(message)


def value_matches(requirement: TomlValue, value: TomlValue | None) -> bool:
    """Return whether a variant value satisfies an example requirement."""
    if isinstance(requirement, list):
        return value in requirement
    return value == requirement


def exclusion_matches(exclusion: TomlTable, variant: TomlTable) -> bool:
    """Return whether a variant matches an explicit exclusion."""
    for key, value in exclusion.items():
        if key != "reason" and variant.get(key) != value:
            return False
    return True


def supports(example: TomlTable, variant: TomlTable) -> bool:
    """Return whether an example supports a variant."""
    requirements = as_table(example.get("requires"))
    for key, requirement in requirements.items():
        if key not in variant or not value_matches(requirement, variant[key]):
            return False

    for exclusion in as_exclusion_list(example.get("exclude")):
        if exclusion_matches(exclusion, variant):
            return False

    return True


def variant_from_profile(
    profile_path: Path,
    runners: TomlTable,
) -> tuple[str, TomlTable]:
    """Read variant metadata from a mise environment profile."""
    profile = load_toml(profile_path)
    env = as_table(profile.get("env"))
    variant_name = string_value(env, "MLN_FFI_VARIANT")
    target_triple = string_value(env, "MLN_FFI_TARGET_TRIPLE")
    backend = string_value(env, "MLN_FFI_RENDER_BACKEND")
    platform, arch = target_triple.split("-", maxsplit=1)

    return variant_name, {
        "platform": platform,
        "arch": arch,
        "backend": backend,
        "runner": string_value(runners, target_triple),
    }


def load_variants(repo_root: Path, schema: TomlTable) -> dict[str, TomlTable]:
    """Load variants from mise environment profiles."""
    runners = as_table(schema.get("runners"))
    variants: dict[str, TomlTable] = {}
    for profile_path in repo_root.glob(".mise/config.*.toml"):
        variant_name, variant = variant_from_profile(profile_path, runners)
        variants[variant_name] = variant
    return dict(sorted(variants.items()))


def native_matrix(variants: dict[str, TomlTable]) -> Matrix:
    """Generate the native build matrix."""
    return {
        "include": [
            {
                "variant": variant_name,
                "runner": string_value(variant, "runner"),
            }
            for variant_name, variant in sorted(variants.items())
        ]
    }


def examples_matrix(schema: TomlTable, variants: dict[str, TomlTable]) -> Matrix:
    """Generate the examples build matrix."""
    examples = as_table_map(schema.get("examples"))
    include: list[MatrixEntry] = []

    for example_name, example in sorted(examples.items()):
        for variant_name, variant in sorted(variants.items()):
            if supports(example, variant):
                include.append(
                    {
                        "example": example_name,
                        "task": string_value(example, "task"),
                        "variant": variant_name,
                        "runner": string_value(variant, "runner"),
                    }
                )

    return {"include": include}


def main() -> int:
    """Run the matrix generator CLI."""
    matrix_name = os.environ["usage_matrix"]
    schema_path = Path(
        os.environ.get("usage_schema", ".github/config/variants.toml"),
    )
    pretty = os.environ.get("usage_pretty") == "true"

    schema = load_toml(schema_path)
    variants = load_variants(Path.cwd(), schema)
    if matrix_name == "native":
        matrix = native_matrix(variants)
    else:
        matrix = examples_matrix(schema, variants)

    if pretty:
        matrix_json = json.dumps(matrix, indent=2, sort_keys=True)
    else:
        matrix_json = json.dumps(matrix, sort_keys=True, separators=(",", ":"))

    sys.stdout.write(matrix_json)
    sys.stdout.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
