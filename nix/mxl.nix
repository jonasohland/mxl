# SPDX-FileCopyrightText: 2026 Contributors to the Media eXchange Layer project.
# SPDX-License-Identifier: Apache-2.0

# The MXL C library, the fabrics library, the tools in tools/ and the
# looping_filesrc GStreamer plugin in utils/. Dependencies come from nixpkgs
# instead of vcpkg, so the CMake presets are not used.
{
  lib,
  stdenv,
  cmake,
  ninja,
  pkg-config,
  makeWrapper,
  ada,
  catch2_3,
  cli11,
  fmt,
  gst_all_1,
  libfabric,
  libuuid,
  microsoft-gsl,
  pcapplusplus,
  picojson,
  spdlog,
  stduuid,
  src,
  version,
  # Patches for this source, for example a release in releases.json.
  patches ? [ ],
  # Written into the version string as the commit ID. CMakeLists.txt reads it
  # from .git, which is not part of the source in a Nix build, and falls back
  # to "unknown".
  gitHash ? null,
  # The OFI fabrics backend needs libfabric, which is only packaged for Linux.
  enableFabrics ? stdenv.hostPlatform.isLinux,
  # null keeps the CMake default (x86-64-v3 or armv8.5-a). An empty string
  # builds for the baseline of the target platform.
  targetArch ? null,
}:

let
  # Elements the mxl-gst-* tools create at runtime, for example videotestsrc,
  # appsink, decodebin and the video sinks.
  gstRuntimePlugins = with gst_all_1; [
    gstreamer
    gst-plugins-base
    gst-plugins-good
    gst-plugins-bad
    gst-plugins-ugly
    gst-libav
  ];
in
stdenv.mkDerivation {
  pname = "mxl";
  inherit version src patches;

  postPatch = lib.optionalString (gitHash != null) ''
    substituteInPlace CMakeLists.txt \
      --replace-fail 'set(GIT_HASH "unknown")' 'set(GIT_HASH "${gitHash}")'
  '';

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    makeWrapper
  ];

  buildInputs = [
    ada
    cli11
    fmt
    microsoft-gsl
    picojson
    spdlog
    stduuid
    gst_all_1.gstreamer
    gst_all_1.gst-plugins-base
  ]
  ++ lib.optionals enableFabrics [
    libfabric
    # lib/fabrics/ofi includes <uuid/uuid.h> from the system.
    libuuid
  ];

  # The tests on release/v1.1 read pcap files with PcapPlusPlus on Linux. The
  # tests on main do not look for it.
  checkInputs = [ catch2_3 ] ++ lib.optional stdenv.hostPlatform.isLinux pcapplusplus;

  cmakeFlags = [
    (lib.cmakeBool "BUILD_DOCS" false)
    (lib.cmakeBool "BUILD_TESTS" true)
    # ON or OFF instead of lib.cmakeBool's TRUE or FALSE, because the
    # pkg-config consumer test only accepts ON or OFF for this value.
    (lib.cmakeFeature "MXL_ENABLE_FABRICS_OFI" (if enableFabrics then "ON" else "OFF"))
  ]
  ++ lib.optional (targetArch != null) (lib.cmakeFeature "MXL_TARGET_ARCH" targetArch);

  # ctest runs each Catch2 test case in its own process. The cases use the
  # same domain in /dev/shm, so they run one at a time.
  doCheck = true;
  enableParallelChecking = false;
  # Some tests create files in $HOME, which is not writable in the sandbox.
  checkPhase = ''
    runHook preCheck
    export HOME="$(mktemp -d)"
    ctest --output-on-failure --parallel 1
    runHook postCheck
  '';

  # The mxl-gst-* tools load GStreamer elements by name. Point them at the
  # plugins they need, including looping_filesrc from this package.
  postFixup = ''
    for tool in "$out"/bin/mxl-gst-*; do
      wrapProgram "$tool" \
        --prefix GST_PLUGIN_SYSTEM_PATH_1_0 : "$out/lib/gstreamer-1.0" \
        --prefix GST_PLUGIN_SYSTEM_PATH_1_0 : "${
          lib.makeSearchPathOutput "lib" "lib/gstreamer-1.0" gstRuntimePlugins
        }"
    done
  '';

  passthru = { inherit gstRuntimePlugins; };

  meta = {
    description = "Media eXchange Layer SDK and tools";
    homepage = "https://github.com/dmf-mxl/mxl";
    license = lib.licenses.asl20;
    platforms = lib.platforms.linux ++ lib.platforms.darwin;
    mainProgram = "mxl-info";
  };
}
