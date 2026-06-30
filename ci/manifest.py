import pathlib
import re
import tomllib
from typing import Annotated

from pydantic import (
    BaseModel,
    ConfigDict,
    Field,
    ValidationError,
    ValidationInfo,
    field_validator,
    model_validator,
)


REPO_ROOT = pathlib.Path(__file__).resolve().parents[1]
VARIANTS_PATH = REPO_ROOT / "ci" / "variants.toml"
SUBPROJECTS_DIR = REPO_ROOT / "ci" / "subprojects"

TASK_PATTERN = re.compile(r"^(?:[A-Za-z0-9_-]+|//[A-Za-z0-9_./-]+:[A-Za-z0-9_-]+)$")

NonEmptyString = Annotated[str, Field(min_length=1)]
NonEmptyStringList = Annotated[list[NonEmptyString], Field(min_length=1)]


class StrictManifestModel(BaseModel):
    model_config = ConfigDict(extra="forbid", strict=True)


class VariantConfig(StrictManifestModel):
    runner: NonEmptyString
    os: NonEmptyString
    arch: NonEmptyString
    backend: NonEmptyString
    supports_tests: bool = True


class Variant(VariantConfig):
    name: NonEmptyString


class VariantsManifest(StrictManifestModel):
    variants: Annotated[dict[NonEmptyString, VariantConfig], Field(min_length=1)]


class RequiresConfig(StrictManifestModel):
    os: NonEmptyStringList | None = None
    arch: NonEmptyStringList | None = None
    backend: NonEmptyStringList | None = None
    exclude: list[NonEmptyString] = Field(default_factory=list)

    @field_validator("os", "arch", "backend")
    @classmethod
    def require_known_values(
        cls, values: list[str] | None, info: ValidationInfo
    ) -> list[str] | None:
        if values is None:
            return values
        known_values = (info.context or {}).get("variant_values", {})
        unknown = set(values) - known_values.get(info.field_name, set())
        if unknown:
            raise ValueError(f"unknown values: {sorted(unknown)}")
        return values

    @field_validator("exclude")
    @classmethod
    def require_known_variants(
        cls, variants: list[str], info: ValidationInfo
    ) -> list[str]:
        known_variants = (info.context or {}).get("variants", set())
        unknown = set(variants) - known_variants
        if unknown:
            raise ValueError(f"unknown variants: {sorted(unknown)}")
        return variants


class CiConfig(StrictManifestModel):
    build: NonEmptyString | None = None
    test: NonEmptyString | None = None
    run: NonEmptyString | None = None
    check: NonEmptyString | None = None

    @model_validator(mode="after")
    def require_action(self) -> "CiConfig":
        if (
            self.build is None
            and self.test is None
            and self.run is None
            and self.check is None
        ):
            raise ValueError("[ci] must contain at least one action")
        return self

    @field_validator("build", "test", "run", "check")
    @classmethod
    def require_task_name(cls, task: str | None) -> str | None:
        if task is not None and not TASK_PATTERN.fullmatch(task):
            raise ValueError("must be a mise task name")
        return task


class SubprojectManifest(StrictManifestModel):
    label: NonEmptyString | None = None
    source_directory: NonEmptyString | None = None
    requires: RequiresConfig = Field(default_factory=RequiresConfig)
    ci: CiConfig | None = None


class Subproject(StrictManifestModel):
    name: NonEmptyString
    path: pathlib.Path
    manifest: SubprojectManifest


def parse_model[T: BaseModel](
    path: pathlib.Path, model: type[T], context: dict | None = None
) -> T:
    try:
        with path.open("rb") as file:
            return model.model_validate(tomllib.load(file), context=context)
    except ValidationError as error:
        raise SystemExit(f"error: {path}: {error}") from None


def load_variants() -> list[Variant]:
    manifest = parse_model(VARIANTS_PATH, VariantsManifest)
    return [
        Variant(name=name, **config.model_dump())
        for name, config in manifest.variants.items()
    ]


def manifest_context(variants: list[Variant]) -> dict[str, object]:
    return {
        "variants": {variant.name for variant in variants},
        "variant_values": {
            "os": {variant.os for variant in variants},
            "arch": {variant.arch for variant in variants},
            "backend": {variant.backend for variant in variants},
        },
    }


def load_subprojects(variants: list[Variant]) -> list[Subproject]:
    context = manifest_context(variants)
    paths = sorted(
        SUBPROJECTS_DIR.glob("*.toml"),
        key=lambda path: (path.name != "native.toml", path.name),
    )
    return [
        Subproject(
            name=path.stem,
            path=path,
            manifest=apply_default_ci(
                path, parse_model(path, SubprojectManifest, context)
            ),
        )
        for path in paths
    ]


def default_ci(name: str) -> CiConfig | None:
    if name.startswith("bindings-"):
        binding = name.removeprefix("bindings-")
        return CiConfig(
            build=f"//bindings/{binding}:build",
            test=f"//bindings/{binding}:test",
        )
    if name.startswith("examples-"):
        return CiConfig(build=f"//examples/{name.removeprefix('examples-')}:build")
    return None


def apply_default_ci(
    path: pathlib.Path, manifest: SubprojectManifest
) -> SubprojectManifest:
    if manifest.ci is not None:
        return manifest

    ci = default_ci(path.stem)
    if ci is None:
        raise SystemExit(f"error: {path}: [ci] is required")
    return manifest.model_copy(update={"ci": ci})


def matches(variant: Variant, requires: dict[str, list[str]]) -> bool:
    return all(getattr(variant, key) in values for key, values in requires.items())


def subproject_matches_variant(subproject: Subproject, variant: Variant) -> bool:
    requires = subproject.manifest.requires.model_dump(
        include={"os", "arch", "backend"}, exclude_none=True
    )
    excluded_variants = set(subproject.manifest.requires.exclude)
    return matches(variant, requires) and variant.name not in excluded_variants
