FROM tools

LABEL org.opencontainers.image.source="https://github.com/maplibre/maplibre-native-ffi"

SHELL ["/bin/bash", "-euo", "pipefail", "-c"]

RUN apt-get update \
  && apt-get install --no-install-recommends --yes \
    bash \
    ca-certificates \
    curl \
    git \
    libegl-mesa0 \
    libgl1-mesa-dri \
    libicu72 \
    mesa-utils \
    sudo \
  && rm -rf /var/lib/apt/lists/*

RUN useradd --create-home --uid 1000 --shell /bin/bash dev \
  && echo "dev ALL=(ALL) NOPASSWD:ALL" > /etc/sudoers.d/dev \
  && chmod 0440 /etc/sudoers.d/dev

RUN mkdir -p /mise/migrations \
  && chown dev:dev /mise/migrations

USER dev
