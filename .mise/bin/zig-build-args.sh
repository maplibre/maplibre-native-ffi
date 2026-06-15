source "$MLN_FFI_REPO_ROOT/.mise/bin/native-build-env.sh"

MLN_FFI_ZIG_INCLUDE_ARGS=()
for include_dir in "${MLN_FFI_NATIVE_INCLUDE_DIRS[@]}"; do
  MLN_FFI_ZIG_INCLUDE_ARGS+=(-Dinclude-dir="$include_dir")
done

for library_dir in "${MLN_FFI_NATIVE_LIBRARY_DIRS[@]}"; do
  MLN_FFI_ZIG_DEPENDENCY_LIBRARY_DIR="$library_dir"
done

MLN_FFI_ZIG_BUILD_ARGS=(
  -Dtarget="$MLN_FFI_ZIG_TARGET"
  -Dcmake-artifact-dir="$MLN_FFI_BUILD_DIR"
  "${MLN_FFI_ZIG_INCLUDE_ARGS[@]}"
  -Ddependency-library-dir="$MLN_FFI_ZIG_DEPENDENCY_LIBRARY_DIR"
  -Drender-backend="$MLN_FFI_RENDER_BACKEND"
)
