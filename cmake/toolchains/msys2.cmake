# Select tools by full path so a Visual Studio or standalone LLVM install
# on PATH cannot change the MSYS2 environment's ABI.
if(NOT CMAKE_HOST_WIN32)
  message(FATAL_ERROR "The MSYS2 toolchain requires a Windows host")
endif()

set(MLN_FFI_MSYS2_ROOT "$ENV{MLN_FFI_MSYS2_ROOT}"
    CACHE PATH "MSYS2 MINGW64 or CLANGARM64 prefix")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES MLN_FFI_MSYS2_ROOT)

set(CMAKE_C_COMPILER "${MLN_FFI_MSYS2_ROOT}/bin/clang.exe")
set(CMAKE_CXX_COMPILER "${MLN_FFI_MSYS2_ROOT}/bin/clang++.exe")
# The engine and platform archives refer back to each other; LLD resolves
# those references without depending on archive order.
set(CMAKE_LINKER_TYPE LLD)
set(CMAKE_AR "${MLN_FFI_MSYS2_ROOT}/bin/llvm-ar.exe" CACHE FILEPATH "" FORCE)
set(CMAKE_RANLIB "${MLN_FFI_MSYS2_ROOT}/bin/llvm-ranlib.exe"
    CACHE FILEPATH "" FORCE)
set(CMAKE_RC_COMPILER "${MLN_FFI_MSYS2_ROOT}/bin/windres.exe")
list(PREPEND CMAKE_PREFIX_PATH "${MLN_FFI_MSYS2_ROOT}")

# libstdc++ emits pthread calls from its inline synchronization helpers, so
# every C++ executable, including a test that links only the DLL, needs it.
if(MLN_FFI_TARGET_ARCHITECTURE STREQUAL "x64")
  set(CMAKE_CXX_FLAGS_INIT "-pthread")
endif()
