# The library imports D3D12 and DXGI like any Windows DLL, and Rust bundles
# their
# import library entries into the static archive, so consumers add no linker
# flags for them. It loads DXC, which the app ships, at run time when it
# initializes D3D12.
function(mln_ffi_configure_d3d12_dependencies target)
endfunction()
