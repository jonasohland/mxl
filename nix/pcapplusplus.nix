# SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
# SPDX-License-Identifier: Apache-2.0

# PcapPlusPlus is not in nixpkgs. The tests on release/v1.1 read pcap files
# with it. MXL main has its own pcap reader and does not use it.
{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  ninja,
  libpcap,
}:

stdenv.mkDerivation (finalAttrs: {
  pname = "pcapplusplus";
  version = "25.05";

  src = fetchFromGitHub {
    owner = "seladb";
    repo = "PcapPlusPlus";
    tag = "v${finalAttrs.version}";
    hash = "sha256-rxX8VZRLXRtpFzMegDqXqmZ5ZpvjAoRqZ6hyhYTemII=";
  };

  nativeBuildInputs = [
    cmake
    ninja
  ];

  # The installed CMake package looks up libpcap when a consumer loads it.
  propagatedBuildInputs = [ libpcap ];

  cmakeFlags = [
    (lib.cmakeBool "PCAPPP_BUILD_EXAMPLES" false)
    (lib.cmakeBool "PCAPPP_BUILD_TESTS" false)
    (lib.cmakeBool "PCAPPP_BUILD_TUTORIALS" false)
    (lib.cmakeBool "PCAPPP_INSTALL" true)
    # PcapPlusPlusConfig.cmake prepends the install prefix to these paths.
    # Nix sets them to absolute paths, which would give the prefix twice.
    (lib.cmakeFeature "CMAKE_INSTALL_INCLUDEDIR" "include")
    (lib.cmakeFeature "CMAKE_INSTALL_LIBDIR" "lib")
  ];

  meta = {
    description = "C++ library for capturing, parsing and crafting network packets";
    homepage = "https://pcapplusplus.github.io";
    license = lib.licenses.unlicense;
    platforms = lib.platforms.linux;
  };
})
