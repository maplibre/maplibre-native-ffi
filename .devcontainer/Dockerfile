FROM tools

LABEL org.opencontainers.image.source="https://github.com/maplibre/maplibre-native-ffi"

SHELL ["/bin/bash", "-euo", "pipefail", "-c"]

# Project build dependencies come from mise.linux.toml through the tools image.
# This layer contains only container-specific utilities.
RUN apt-get update \
  && apt-get install --no-install-recommends --yes \
    ca-certificates \
    curl \
    git \
    libgl1-mesa-dri \
    mesa-utils \
    mesa-vulkan-drivers \
    sudo \
  && rm -rf /var/lib/apt/lists/*

# mise's OCI v1 builder rejects plugin-backed tools, so install Emscripten in a
# regular image layer after the content-addressable tool image is assembled.
RUN mise install emsdk@4.0.10

RUN groupmod --new-name dev ubuntu \
  && usermod --login dev --home /home/dev --move-home --shell /bin/bash ubuntu \
  && echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev \
  && chmod 0440 /etc/sudoers.d/dev

# The tools image bakes the complete monorepo toolset. Mise OCI images use
# MISE_DATA_DIR=/mise (default mount_point); grant the runtime user write access
# so project tasks can update a tool before the published :main image catches up.
RUN mkdir -p /mise/migrations \
  && chown -R dev:dev /mise

USER dev

# Rustup stores toolchains outside MISE_DATA_DIR. The OCI tool layer is built
# as root, so let the runtime user materialize its own writable toolchain.
ENV CARGO_HOME=/home/dev/.cargo \
  RUSTUP_HOME=/home/dev/.rustup
