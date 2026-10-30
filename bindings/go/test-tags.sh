# Sourced by the Go test tasks with a native preset as $1. Sets go_test_tags
# to the build tags the suite compiles with: mlntest, which builds the test
# seams in testhook.go, and the tag of the preset's render backend, which
# builds the render tests and links tests/graphics.
# shellcheck shell=bash

go_test_tags=mlntest
case "$1" in
  *-metal) go_test_tags+=,mln_metal ;;
  *-vulkan) go_test_tags+=,mln_vulkan ;;
  *-egl) go_test_tags+=,mln_egl ;;
  *-wgl) go_test_tags+=,mln_wgl ;;
esac
