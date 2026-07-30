set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE static)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

# Overlay for the community x64-linux triplet: builds every port with
# Clang + libc++ (via CC/CXX env vars set in the Dockerfile) instead of the
# default GCC/libstdc++, so nothing ends up statically linking libstdc++ and
# libc++ objects together. Driven by libpqxx 8.x needing <format>, which
# libstdc++ only implements from GCC 13, which bookworm-slim can't run.
set(VCPKG_CXX_FLAGS "-stdlib=libc++")
set(VCPKG_C_FLAGS "")
set(VCPKG_LINKER_FLAGS "-stdlib=libc++")
