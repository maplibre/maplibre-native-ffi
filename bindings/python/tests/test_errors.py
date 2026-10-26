"""How native statuses and binding failures reach Python as exceptions."""

import maplibre_native_ffi as mln
import pytest
from support import Harness, map_options, result


def test_native_statuses_carry_their_status_and_diagnostic(
    harness: Harness, map_handle: mln.MapHandle
) -> None:
    # An accepted call reports its status through the future; a rejected
    # submission raises before returning one.
    with pytest.raises(mln.MaplibreError) as not_found:
        result(map_handle.get_style_layer_property("missing", "visibility"))
    with pytest.raises(mln.MaplibreError) as invalid_state:
        harness.runtime.close()
    with pytest.raises(mln.MaplibreError) as invalid_argument:
        mln.set_network_status(mln.NetworkStatus(999_001))

    for raised, status in (
        (not_found, mln.Status.NOT_FOUND),
        (invalid_state, mln.Status.INVALID_STATE),
        (invalid_argument, mln.Status.INVALID_ARGUMENT),
    ):
        assert raised.value.status == status
        assert raised.value.diagnostic
        assert str(raised.value) == f"{status.name}: {raised.value.diagnostic}"
    assert "network status" in invalid_argument.value.diagnostic


def test_a_closed_handle_reports_the_native_status() -> None:
    runtime = mln.RuntimeHandle.create()
    result(runtime.close())

    with pytest.raises(mln.MaplibreError) as raised:
        runtime.create_map(map_options())

    assert raised.value.status == mln.Status.INVALID_STATE
    assert raised.value.diagnostic
