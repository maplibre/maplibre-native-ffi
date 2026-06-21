FROM tools

LABEL org.opencontainers.image.source="https://github.com/maplibre/maplibre-native-ffi"

SHELL ["/bin/bash", "-euo", "pipefail", "-c"]

RUN apt-get update \
  && apt-get install --no-install-recommends --yes \
    bash \
    build-essential \
    ca-certificates \
    curl \
    git \
    libegl-mesa0 \
    libgl1-mesa-dri \
    libicu72 \
    libvulkan1 \
    mesa-utils \
    mesa-vulkan-drivers \
    sudo \
  && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --uid 1000 --shell /bin/bash dev \
  && echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev \
  && chmod 0440 /etc/sudoers.d/dev

# Mise OCI images use MISE_DATA_DIR=/mise (default mount_point). The tools layer is
# built as root; grant the runtime user write access so postCreate `mise install` can
# add or update tools before the published :main image catches up.
RUN mkdir -p /mise/migrations \
  && chown -R dev:dev /mise

USER dev
