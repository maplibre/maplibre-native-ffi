# Sourced by Go tasks with a preset as $1 and a render backend as $2, such as
# vulkan or opengl. Selects the test tags of the backend under test and the
# platform's tags. The suite links the preset's install through pkg-config, not
# a preset module.
# shellcheck shell=bash

go_test_tags=mlntest,mln_pkgconfig
case "$2" in
  metal) go_test_tags+=,mln_metal ;;
  vulkan) go_test_tags+=,mln_vulkan ;;
  opengl)
    case "$1" in
      windows-* | mingw-*) go_test_tags+=,mln_wgl ;;
      *) go_test_tags+=,mln_egl ;;
    esac
    ;;
  *)
    echo "The Go suite has no tests of the $2 backend." >&2
    return 2
    ;;
esac
case "$1" in
  ios-maccatalyst-*) go_test_tags+=,macos,maccatalyst ;;
esac
