#!/usr/bin/env bash

# Sourced by the tasks that drive the Swift toolchain.
#
# The toolchain's libFoundationXML.so links libxml2.so.2, which SwiftPM loads.
# Distros from Ubuntu 26.04 on ship only libxml2.so.16 and drop the package that
# carried the older soname, so the Swift configs pin a conda-forge libxml2 and
# this points the loader at it.
#
# macOS builds with the system toolchain, where mise installs no conda:libxml2,
# so `mise where` fails there and the loader path is left alone.
#
# This runs in the task's own shell rather than a config-level [env] because
# that directory holds a whole conda dependency set — ICU, libstdc++, libgcc —
# and only the Swift toolchain should see it.

if mln_libxml2_dir="$(mise where conda:libxml2 2>/dev/null)"; then
  export LD_LIBRARY_PATH="$mln_libxml2_dir/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH}}"
fi
unset mln_libxml2_dir
