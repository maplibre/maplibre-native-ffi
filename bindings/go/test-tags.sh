# Sourced by Go tasks with a preset as $1. Selects test and platform tags.
# shellcheck shell=bash

go_test_tags=mlntest
case "$1" in
  *-metal) go_test_tags+=,mln_metal ;;
  *-vulkan) go_test_tags+=,mln_vulkan ;;
  *-egl) go_test_tags+=,mln_egl ;;
  *-wgl) go_test_tags+=,mln_wgl ;;
esac
case "$1" in
  ios-maccatalyst-*) go_test_tags+=,macos,maccatalyst ;;
esac
