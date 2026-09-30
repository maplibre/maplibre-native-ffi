# Sourced by the Go test tasks with a native preset as $1. Sets go_test_tags
# to the build tags the suite compiles with: mlntest, which builds the test
# seams in testhook.go, and the tag of the preset's render backend, which
# builds the render tests and links tests/graphics. A variant preset, such as
# macos-arm64-metal-coverage, takes its backend's tag. WGL presets get no
# backend tag, because the Go wrapper of tests/graphics has no WGL context.
# shellcheck shell=bash

go_test_tags=mlntest
case "$1" in
  *-metal | *-metal-*) go_test_tags+=,mln_metal ;;
  *-vulkan | *-vulkan-*) go_test_tags+=,mln_vulkan ;;
  *-egl | *-egl-*) go_test_tags+=,mln_egl ;;
esac
