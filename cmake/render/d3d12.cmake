# The library loads D3D12 and DXGI from the system and the DXC compiler from the
# app at run time, so its consumers link nothing for it.
function(mln_ffi_configure_d3d12_dependencies target)
endfunction()
