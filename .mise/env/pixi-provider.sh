repo_root="${MLN_FFI_REPO_ROOT:-$(pwd)}"
pixi_prefix="$repo_root/.pixi/envs/default"

windows_path() {
  if command -v cygpath >/dev/null 2>&1; then
    cygpath -m "$1"
  else
    printf '%s\n' "$1"
  fi
}

build_path() {
  case "$(uname -s)" in
    MINGW* | MSYS* | CYGWIN*)
      windows_path "$1"
      ;;
    *)
      printf '%s\n' "$1"
      ;;
  esac
}

append_path_value() {
  local name="$1"
  local value="$2"
  [[ -n "$value" ]] || return 0
  if [[ -n "${!name:-}" ]]; then
    export "$name=${!name};$value"
  else
    export "$name=$value"
  fi
}

for required_name in \
  MLN_FFI_REPO_ROOT \
  MLN_FFI_INCLUDE_DIR \
  MLN_FFI_VULKAN_INCLUDE_DIR \
  MLN_FFI_TARGET_TRIPLE \
  MLN_FFI_RENDER_BACKEND \
  MLN_FFI_BUILD_DIR \
  MLN_FFI_ZIG_TARGET; do
  if [[ -z "${!required_name:-}" ]]; then
    echo "$required_name is required before loading the pixi dependency provider" >&2
    return 1
  fi
done

case "$(uname -s)" in
  Darwin)
    export MLN_FFI_DEPS_PREFIX="$pixi_prefix"
    export MLN_FFI_DEPENDENCY_ROOT="$pixi_prefix"
    export MLN_FFI_DEPENDENCY_INCLUDE_DIR="$MLN_FFI_DEPS_PREFIX/include"
    export MLN_FFI_DEPENDENCY_LIBRARY_DIR="$MLN_FFI_DEPS_PREFIX/lib"
    export CMAKE_PREFIX_PATH="$MLN_FFI_DEPS_PREFIX${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
    export PKG_CONFIG_PATH="$MLN_FFI_DEPS_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    export LIBCLANG_PATH="$MLN_FFI_DEPS_PREFIX/lib"
    export RUSTFLAGS="-C link-arg=-Wl,-rpath,$MLN_FFI_DEPENDENCY_LIBRARY_DIR -C link-arg=-Wl,-rpath,$MLN_FFI_BUILD_DIR"
    export MLN_FFI_SYSTEM_ROOT="${SDKROOT:-$(xcrun --sdk macosx --show-sdk-path 2>/dev/null || true)}"
    export JDK_JAVA_OPTIONS="-Djava.library.path=$MLN_FFI_DEPENDENCY_LIBRARY_DIR -Dorg.lwjgl.vulkan.libname=libvulkan.1.dylib${JDK_JAVA_OPTIONS:+ $JDK_JAVA_OPTIONS}"
    export VK_ICD_FILENAMES="$MLN_FFI_DEPS_PREFIX/share/vulkan/icd.d/MoltenVK_icd.json"
    ;;
  Linux)
    export MLN_FFI_DEPS_PREFIX="$pixi_prefix"
    export MLN_FFI_DEPENDENCY_ROOT="$pixi_prefix"
    export MLN_FFI_DEPENDENCY_INCLUDE_DIR="$MLN_FFI_DEPS_PREFIX/include"
    export MLN_FFI_DEPENDENCY_LIBRARY_DIR="$MLN_FFI_DEPS_PREFIX/lib"
    export CMAKE_PREFIX_PATH="$MLN_FFI_DEPS_PREFIX${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"
    export CMAKE_LIBRARY_PATH="$MLN_FFI_DEPENDENCY_LIBRARY_DIR${CMAKE_LIBRARY_PATH:+:$CMAKE_LIBRARY_PATH}"
    export PKG_CONFIG_PATH="$MLN_FFI_DEPS_PREFIX/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
    export LIBRARY_PATH="$MLN_FFI_DEPENDENCY_LIBRARY_DIR${LIBRARY_PATH:+:$LIBRARY_PATH}"
    export LIBCLANG_PATH="$MLN_FFI_DEPS_PREFIX/lib"
    export LD_LIBRARY_PATH="$MLN_FFI_DEPENDENCY_LIBRARY_DIR${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

    case "${MLN_FFI_TARGET_TRIPLE:-}" in
      linux-x64)
        toolchain_triple="x86_64-conda-linux-gnu"
        ;;
      linux-arm64)
        toolchain_triple="aarch64-conda-linux-gnu"
        ;;
      *)
        toolchain_triple=""
        ;;
    esac

    if [[ -n "$toolchain_triple" ]]; then
      sysroot="$MLN_FFI_DEPS_PREFIX/$toolchain_triple/sysroot"
      export MLN_FFI_SYSTEM_ROOT="$sysroot"
      bindgen_args="--sysroot=$sysroot"
      gcc_root="$MLN_FFI_DEPS_PREFIX/lib/gcc/$toolchain_triple"
      if [[ -d "$gcc_root" ]]; then
        gcc_version="$(find "$gcc_root" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort -V | tail -n 1)"
        if [[ -n "$gcc_version" ]]; then
          gcc_include="$gcc_root/$gcc_version/include"
          gcc_include_fixed="$gcc_root/$gcc_version/include-fixed"
          [[ -d "$gcc_include" ]] && bindgen_args="$bindgen_args -isystem$gcc_include"
          [[ -d "$gcc_include_fixed" ]] && bindgen_args="$bindgen_args -isystem$gcc_include_fixed"
        fi
      fi
      export BINDGEN_EXTRA_CLANG_ARGS="$bindgen_args"
    fi
    export RUSTFLAGS="-C link-arg=-Wl,-rpath,$MLN_FFI_DEPENDENCY_LIBRARY_DIR -C link-arg=-Wl,-rpath-link,$MLN_FFI_DEPENDENCY_LIBRARY_DIR -C link-arg=-Wl,-rpath,$MLN_FFI_BUILD_DIR"
    ;;
  MINGW* | MSYS* | CYGWIN*)
    deps_prefix="$pixi_prefix/Library"
    deps_bin="$deps_prefix/bin"
    export MLN_FFI_DEPENDENCY_ROOT="$(windows_path "$pixi_prefix")"
    export MLN_FFI_DEPS_PREFIX="$(windows_path "$deps_prefix")"
    export MLN_FFI_DEPENDENCY_INCLUDE_DIR="$(windows_path "$deps_prefix/include")"
    export MLN_FFI_DEPENDENCY_LIBRARY_DIR="$(windows_path "$deps_prefix/lib")"
    export CMAKE_PREFIX_PATH="$MLN_FFI_DEPS_PREFIX${CMAKE_PREFIX_PATH:+;$CMAKE_PREFIX_PATH}"
    export CMAKE_LIBRARY_PATH="$MLN_FFI_DEPENDENCY_LIBRARY_DIR${CMAKE_LIBRARY_PATH:+;$CMAKE_LIBRARY_PATH}"
    export PKG_CONFIG_PATH="$(windows_path "$deps_prefix/lib/pkgconfig")${PKG_CONFIG_PATH:+;$PKG_CONFIG_PATH}"
    export LIBCLANG_PATH="$(windows_path "$deps_bin")"
    export RUSTFLAGS="-L native=$MLN_FFI_DEPENDENCY_LIBRARY_DIR"
    ;;
esac

export MLN_FFI_NATIVE_INCLUDE_DIRS=""
export MLN_FFI_NATIVE_LIBRARY_DIRS=""
append_path_value MLN_FFI_NATIVE_INCLUDE_DIRS "$(build_path "$MLN_FFI_INCLUDE_DIR")"
append_path_value MLN_FFI_NATIVE_INCLUDE_DIRS "$MLN_FFI_DEPENDENCY_INCLUDE_DIR"
append_path_value MLN_FFI_NATIVE_INCLUDE_DIRS "$(build_path "$MLN_FFI_VULKAN_INCLUDE_DIR")"
append_path_value MLN_FFI_NATIVE_LIBRARY_DIRS "$MLN_FFI_DEPENDENCY_LIBRARY_DIR"

if [[ -n "${MLN_FFI_EGL_ROOT:-}" ]]; then
  egl_root="$(build_path "$MLN_FFI_EGL_ROOT")"
  append_path_value MLN_FFI_NATIVE_INCLUDE_DIRS "$egl_root/include"
  append_path_value MLN_FFI_NATIVE_LIBRARY_DIRS "$egl_root"
fi

zig_build_args=(
  "-Dtarget=$MLN_FFI_ZIG_TARGET"
  "-Dcmake-artifact-dir=$(build_path "$MLN_FFI_BUILD_DIR")"
  "-Drender-backend=$MLN_FFI_RENDER_BACKEND"
)

IFS=";" read -ra native_include_dirs <<< "$MLN_FFI_NATIVE_INCLUDE_DIRS"
for native_include_dir in "${native_include_dirs[@]}"; do
  zig_build_args+=("-Dinclude-dir=$native_include_dir")
done

IFS=";" read -ra native_library_dirs <<< "$MLN_FFI_NATIVE_LIBRARY_DIRS"
for native_library_dir in "${native_library_dirs[@]}"; do
  zig_build_args+=("-Ddependency-library-dir=$native_library_dir")
done

printf -v MLN_FFI_ZIG_BUILD_ARGS "%q " "${zig_build_args[@]}"
export MLN_FFI_ZIG_BUILD_ARGS="${MLN_FFI_ZIG_BUILD_ARGS% }"
