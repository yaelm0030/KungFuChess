# syntax=docker/dockerfile:1.7
# Builds and runs the kfc_app binary (--serve-shard / --serve-gateway / smoke checks)
# from the repo-root CMakeLists.txt. Day-to-day local dev still uses Kung-Fu-Chess.sln;
# this image is for Linux (Compose/CI) deployment.

########## Build stage ##########
FROM debian:bookworm AS build

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    ninja-build \
    git \
    ca-certificates \
    curl \
    zip \
    unzip \
    tar \
    pkg-config \
    autoconf \
    automake \
    libtool \
    perl \
    python3 \
    bison \
    flex \
    && rm -rf /var/lib/apt/lists/*

# libpqxx 8.x requires <format>. libstdc++ only implements it from GCC 13, which isn't
# in bookworm-backports, and pulling GCC 13 from trixie drags in a newer glibc that
# bookworm-slim can't run. Use Clang + libc++ instead: libc++ has had <format> since
# LLVM 16, and it's staying self-contained within this build stage.
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget gnupg lsb-release software-properties-common \
    && wget -O /tmp/llvm.sh https://apt.llvm.org/llvm.sh \
    && chmod +x /tmp/llvm.sh \
    && /tmp/llvm.sh 18 \
    && apt-get install -y --no-install-recommends \
        libc++-18-dev libc++abi-18-dev clang-tools-18 \
    && rm -rf /var/lib/apt/lists/* /tmp/llvm.sh \
    && clang++-18 --version \
    && clang-scan-deps-18 --version

# Applies to vcpkg's own port builds (via the x64-linux triplet overlay in
# cmake/vcpkg-triplets, which adds -stdlib=libc++) and the kfc_app/kfc_tests build
# below alike, so nothing in this stage links libstdc++- and libc++-built objects
# together.
ENV CC=clang-18
ENV CXX=clang++-18

ENV VCPKG_ROOT=/opt/vcpkg
RUN git clone --depth 1 https://github.com/microsoft/vcpkg.git "$VCPKG_ROOT" \
    && "$VCPKG_ROOT/bootstrap-vcpkg.sh" -disableMetrics

WORKDIR /workspace
COPY . .

# x64-linux is vcpkg's default triplet on Linux, matching the vcpkg-configuration.json
# baseline already used by the Windows build (commit 3d66d00); VCPKG_OVERLAY_TRIPLETS
# only adds the Clang/libc++ flags on top, it doesn't change the triplet name.
# Cache mounts so a failed/retried configure doesn't re-download and re-build every
# vcpkg port from scratch each iteration.
RUN --mount=type=cache,target=/opt/vcpkg/downloads \
    --mount=type=cache,target=/opt/vcpkg/buildtrees \
    --mount=type=cache,target=/root/.cache/vcpkg \
    cmake -S . -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_OVERLAY_TRIPLETS="/workspace/cmake/vcpkg-triplets" \
    -DCMAKE_CXX_FLAGS="-stdlib=libc++" \
    -DCMAKE_EXE_LINKER_FLAGS="-stdlib=libc++" \
    && cmake --build build --target kfc_app kfc_tests --parallel

# Build-time sanity gate: catches a Linux-specific regression right here, before the
# runtime image is produced, at the cost of a bit of extra build time.
RUN ./build/kfc_tests

########## Runtime stage ##########
FROM debian:bookworm-slim AS runtime

# kung-fu-chess is linked against libc++ (see the build stage), which bookworm-slim
# doesn't ship by default. Add just the apt.llvm.org repo (not the full llvm.sh
# toolchain install — this stage only needs the two runtime .so packages) to pull
# a version-matched libc++/libc++abi.
RUN apt-get update && apt-get install -y --no-install-recommends \
        wget gnupg ca-certificates \
    && rm -rf /var/lib/apt/lists/*
RUN wget -qO /etc/apt/trusted.gpg.d/apt.llvm.org.asc https://apt.llvm.org/llvm-snapshot.gpg.key \
    && echo "deb http://apt.llvm.org/bookworm/ llvm-toolchain-bookworm-18 main" \
        > /etc/apt/sources.list.d/llvm.list \
    && apt-get update && apt-get install -y --no-install-recommends \
        libc++1-18 libc++abi1-18 libpq5 \
    && apt-get purge -y --auto-remove wget gnupg \
    && rm -f /etc/apt/sources.list.d/llvm.list /etc/apt/trusted.gpg.d/apt.llvm.org.asc \
    && rm -rf /var/lib/apt/lists/*

COPY --from=build /workspace/build/kung-fu-chess /usr/local/bin/kung-fu-chess

# No default flag: the same binary serves --serve-shard, --serve-gateway, etc.,
# so the mode is supplied at `docker run`/Compose time.
ENTRYPOINT ["/usr/local/bin/kung-fu-chess"]
