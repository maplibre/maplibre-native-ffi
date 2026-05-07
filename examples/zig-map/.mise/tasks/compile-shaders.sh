#!/usr/bin/env bash
#MISE hide=true
set -euo pipefail

backend="${MLN_FFI_RENDER_BACKEND:-}"
if [[ -z "$backend" ]]; then
  case "$(zig env target)" in
    *-linux*) backend="vulkan" ;;
    *) backend="metal" ;;
  esac
fi

if [[ "$backend" != "vulkan" ]]; then
  exit 0
fi

pixi run glslangValidator -V render/vulkan/shaders/fullscreen.vert -o render/vulkan/shaders/fullscreen.vert.spv
pixi run glslangValidator -V render/vulkan/shaders/sample.frag -o render/vulkan/shaders/sample.frag.spv
