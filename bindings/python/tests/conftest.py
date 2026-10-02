from collections.abc import Iterator

import maplibre_native_ffi as mln
import pytest
from support import Harness


def pytest_configure(config: pytest.Config) -> None:
    # Registered here rather than in pyproject.toml, because the Android device
    # run copies only this directory.
    config.addinivalue_line(
        "markers",
        "global_state: changes process-wide native state and restores its default",
    )


@pytest.fixture
def harness() -> Iterator[Harness]:
    fixture = Harness()
    try:
        yield fixture
    finally:
        fixture.close()


@pytest.fixture
def map_handle(harness: Harness) -> mln.MapHandle:
    return harness.map_create()
