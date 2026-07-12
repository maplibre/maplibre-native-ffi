FROM tools

LABEL org.opencontainers.image.source="https://github.com/maplibre/maplibre-native-ffi"

SHELL ["/bin/bash", "-euo", "pipefail", "-c"]

# Project build dependencies come from the same Linux bootstrap config used by
# contributors and CI. The remaining APT layer contains container-only utilities.
RUN --mount=type=bind,source=mise.linux.toml,target=/tmp/mise/mise.toml,readonly \
    MISE_TRUSTED_CONFIG_PATHS=/tmp/mise \
    mise -C /tmp/mise bootstrap packages apply --manager apt --yes --update

RUN apt-get install --no-install-recommends --yes \
    ca-certificates \
    curl \
    git \
    libgl1-mesa-dri \
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
